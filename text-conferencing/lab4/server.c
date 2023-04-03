/*
** selectserver.c -- a cheezy multiperson chat server
*/

#include "utils.h"

static struct client clients[MAXUSERS];
static struct sessions session_list[MAX_SESSIONS];

/************** primary command functions **************/
void process_login(struct message msg, int sockfd);
void process_createsession(struct message msg, int sockfd);
void process_joinsession(struct message msg, int sockfd);
void process_query(struct message msg, int sockfd);
void process_logout(struct message msg, int sockfd);
void process_exit(struct message msg, int sockfd);
void process_leavesession(struct message msg, int sockfd);

/************** helper functions **************/
bool command_handler(struct message msg, int sockfd);
void init_server();
int next_available_index();
int get_client_index_with_sockfd(int sockfd);
int get_client_index_with_name(struct message msg);
int find_session(char *session_id);
bool username_available(char *username);
bool user_registered(char *username, char *password);

int main(int argc, char *argv[])
{

    init_server(); // init clients list

    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number
    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[256];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];

    char *port = argv[1];

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
        fprintf(stderr, "server: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "[Failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // let the user know that we were successful with binding
    fprintf(stdout, "[Binding successful!]\n");

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // let the user know that the server is on
    fprintf(stdout, "[The server is listening...]\n");

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    int recvIndex, senderIndex;
    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {
            memset(&buf, sizeof(buf), 0);
            senderIndex = get_client_index_with_sockfd(i);

            //fprintf(stdout, "sender: %s\n", clients[senderIndex].username);
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        printf("New connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    //memset(&buf, sizeof(buf), 0);

                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("server: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {


                        struct message msg = deserialize(buf);
                        memset(&buf, sizeof(buf), 0);

                        /************** every other command **************/
                        if(command_handler(msg, i)){
                            continue;
                        /************** private message **************/
                        }else if (msg.type == DM) {
                            unsigned char localData[1000];
                            unsigned char dmUsername[1000];
                            unsigned char newMessage[100];
                            memcpy(localData, msg.data, sizeof(msg.data)); //make a local copy of the message data
                            unsigned char* colon_ptr = strchr(localData, ':');
                            if (colon_ptr != NULL) {
                                size_t len = colon_ptr - (localData + 4);
                                strncpy(dmUsername, (localData + 4), len);
                                dmUsername[len] = '\0';
                                strcpy(newMessage, colon_ptr + 2);
                                memmove(newMessage + 4, newMessage, strlen(newMessage) + 1);
                                memcpy(newMessage, "dm: ", 4);
                                printf("dm to: %s\n", dmUsername);
                            }
                            // we got some data from a client
                            for(j = 0; j <= fdmax; j++) {
                                // send to everyone!
                                if (FD_ISSET(j, &master)) {
                                    if (j != listener && j != i) {
                                        recvIndex = get_client_index_with_sockfd(j);
                                        if(recvIndex == -1) continue;
                                        // except the listener and ourselves
                                        // for private messaging, check for a match in username
                                        if ((strcmp(clients[recvIndex].username, dmUsername) == 0) && clients[recvIndex].is_logged_in){
                                            struct message msg_packet = make_message(MESSAGE, strlen(newMessage), clients[senderIndex].username, newMessage);
                                            char *msg_to_send = serialize(msg_packet);

                                            if (send(j, msg_to_send, nbytes, 0) == -1) {
                                                perror("send");
                                            }
                                        }
                                        memset(&buf, sizeof(buf), 0);
                                    }
                                }
                            }
                        /************** message **************/
                        }else{

                            // just basic text that needs to be sent out
                            if(strcmp(clients[senderIndex].session_id, "lobby") == 0){
                                continue;
                            }

                            // we got some data from a client
                            for(j = 0; j <= fdmax; j++) {
                                // send to everyone!
                                if (FD_ISSET(j, &master)) {
                                    if (j != listener && j != i) {
                                        recvIndex = get_client_index_with_sockfd(j);
                                        if(recvIndex == -1) continue;
                                        // except the listener and ourselves
                                        if ((strcmp(clients[recvIndex].session_id, clients[senderIndex].session_id) == 0) && clients[recvIndex].is_logged_in){
                                            struct message msg_packet = make_message(MESSAGE, strlen(msg.data), clients[senderIndex].username, msg.data);
                                            char *msg_to_send = serialize(msg_packet);

                                            if (send(j, msg_to_send, nbytes, 0) == -1) {
                                                perror("send");
                                            }
                                        }
                                        memset(&buf, sizeof(buf), 0);
                                    }
                                }
                            }
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

    return 0;
}

bool command_handler(struct message msg, int sockfd){
    switch(msg.type){
        case LOGIN:
            process_login(msg, sockfd);
            return true;
        case EXIT:
            process_exit(msg, sockfd);
            return true;
        case JOIN:
            process_joinsession(msg, sockfd);
            return true;
        case NEW_SESS:
            process_createsession(msg, sockfd);
            return true;
        case LEAVE_SESS:
            process_leavesession(msg, sockfd);
            return true;
        case QUERY:
            process_query(msg, sockfd);
            return true;
        case LOGOUT:
            process_logout(msg, sockfd);
            return true;
        default: // doesn't match anything
            return false;
    }
}

void init_server(){
    for(int i = 0; i < MAXUSERS; i++){
        strcpy(regClients[i].username, "-1");
        strcpy(regClients[i].password, "-1");
        regClients[i].sockfd = -1;
        strcpy(regClients[i].session_id, "-1");
        regClients[i].is_logged_in = false;
        // fprintf(stdout, "client %d: %s, %s\n", i, regClients[i].username, regClients[i].password);

    }
    
    read_users_from_file();
    // for(int i = 0; i < MAXUSERS; i++){
    //     fprintf(stdout, "client %d: %s\n", i, regClients[i].username);
    //     fprintf(stdout, "client %d: %s\n", i, regClients[i].password);
    // }
    for(int i = 0; i < MAXUSERS; i++){
        strcpy(clients[i].username, "-1");
        strcpy(clients[i].password, "-1");
        clients[i].sockfd = -1;
        strcpy(clients[i].session_id, "-1");
        clients[i].is_logged_in = false;
       // fprintf(stdout, "client %d: %s, %d, %s, %d\n", i, clients[i].username, clients[i].password, clients[i].sockfd, clients[i].session_id, clients[i].is_logged_in);

    }

    for(int i =0; i < MAX_SESSIONS; i++){
        strcpy(session_list[i].session_id, "-1");
    }

    strcpy(session_list[0].session_id, "lobby");
}

void process_query(struct message msg, int sockfd){


    char message_to_send[MAX_DATA];
    char message_to_send2[MAX_DATA];
    memset(&message_to_send, sizeof(message_to_send), 0);
    strcpy(message_to_send, "List of active users and sessions:\n");

    for(int i = 0; i < MAXUSERS; i++) {
        if((strcmp(clients[i].username,"-1") != 0) && (clients[i].is_logged_in == true)){
            fprintf(stdout, "index query: %d\n", i);
            strcat(message_to_send, clients[i].username);
            strcat(message_to_send, " in ");
            strcat(message_to_send, clients[i].session_id);
            strcat(message_to_send, "\n");
        }
    }

    struct message msg_packet = make_message(QU_ACK, strlen(message_to_send), msg.source, message_to_send);
    memset(&message_to_send, sizeof(message_to_send), 0);
    char *msg_to_send = serialize(msg_packet);

    // send the ack message packet back
    if (send(sockfd, msg_to_send, strlen(msg_to_send), 0) < 0){
        perror("QU_ACK");
    }

    memset(&msg_to_send, sizeof(msg_to_send), 0);
    memset(&msg_packet, sizeof(msg_packet), 0);
    memset(&message_to_send, sizeof(message_to_send), 0);
}

void process_createsession(struct message msg, int sockfd){
    // int index = get_client_index_with_sockfd(sockfd);
    // clients[index].session_id = msg.data;
    struct message msg_packet;

    int id = find_session(msg.data);

    if(id == -1){
        msg_packet =make_message(NS_ACK, strlen(msg.data), msg.source, msg.data);

        int i;
        for(i =0; i < MAX_SESSIONS; i++){
            if(strcmp(session_list[i].session_id,"-1") == 0){
                strcpy(session_list[i].session_id, msg.data);
                break;
            }
        }

    }else{
        char *error = "Session already exists";
        msg_packet = make_message(NS_NAK, strlen(error), msg.source, error);
    }

    char *msg_to_send = serialize(msg_packet);

    // send the ack message packet back
    if (send(sockfd, msg_to_send, strlen(msg_to_send), 0) < 0){
        perror("NS_ACK");
    }

    memset(&msg_to_send, sizeof(msg_to_send), 0);
    memset(&msg_packet, sizeof(msg_packet), 0);

}

void process_login(struct message msg, int sockfd){

    bool check = username_available(msg.source);
    struct message msg_packet;

    if(!check){
        char *error = msg.source;
        strcat(error, " already taken as a username");
        msg_packet = make_message(LO_NAK, strlen(error), msg.source, error);
    }else if(!user_registered(msg.source, msg.data)) {
        char *error = msg.source;
        strcat(error, " not a registered user");
        msg_packet = make_message(LO_NAK, strlen(error), msg.source, error);
    }else{
        int index = next_available_index();
        fprintf(stdout, "index: %d\n", index);

        strcpy(clients[index].username, msg.source);
        strcpy(clients[index].password, msg.data);
        clients[index].sockfd = sockfd;
        strcpy(clients[index].session_id, "lobby");
        clients[index].is_logged_in = true;
        msg_packet = make_message(LO_ACK, strlen(msg.data), msg.source, msg.data);
    }


    fprintf(stdout, "process_login: %s\n", msg.source);

    char *msg_to_send = serialize(msg_packet);

    // send the ack message packet back
    if (send(sockfd, msg_to_send, strlen(msg_to_send), 0) < 0){
        perror("LO_ACK");
    }

    memset(&msg_to_send, sizeof(msg_to_send), 0);
    memset(&msg_packet, sizeof(msg_packet), 0);

}

int get_client_index_with_sockfd(int sockfd){
    for (int i = 0; i < MAXUSERS; i++){
        if (clients[i].sockfd == sockfd){
            return i;
        }
    }
    return -1;
}

int next_available_index(){
    for(int i =0; i < MAXUSERS; i++){
        fprintf(stdout, "client %d: %s, %s, %d, %s, %d\n", i, clients[i].username, clients[i].password, clients[i].sockfd, clients[i].session_id, clients[i].is_logged_in);
        if(strcmp(clients[i].username,"-1") == 0 && clients[i].is_logged_in == 0){
            return i;
        }
    }
    return -1;
}

void process_joinsession(struct message msg, int sockfd){
    struct message msg_packet;

    int id = find_session(msg.data);

    if(id == -1){
        char *error = msg.data;
        strcat(error, " doesn't exist");
        msg_packet = make_message(JN_NAK, strlen(error), msg.source, error);
    }else{
        int index = get_client_index_with_name(msg);
        strcpy(clients[index].session_id, msg.data);
        msg_packet =make_message(JN_ACK, strlen(msg.data), msg.source, msg.data);
    }

    char *msg_to_send = serialize(msg_packet);

    // send the ack message packet back
    if (send(sockfd, msg_to_send, strlen(msg_to_send), 0) < 0){
        perror("JN_ACK");
    }

}

int get_client_index_with_name(struct message msg){
    for (int i = 0; i < MAXUSERS; i++){
        // find the index of the user with the same name
        if (strcmp(clients[i].username, msg.source)==0){
            return i;
        }
    }
    return -1;
}

void process_logout(struct message msg, int sockfd){
    int i = get_client_index_with_name(msg);
    // strcpy(clients[i].username, "-1");
    // strcpy(clients[i].password, "-1");
    // clients[i].sockfd = -1;
    // strcpy(clients[i].session_id, "-1");
    clients[i].is_logged_in = false;

    struct message msg_packet = make_message(LOGOUT_ACK, 0, msg.source, "");
    char *msg_to_send = serialize(msg_packet);

    // send the ack message packet back
    if (send(sockfd, msg_to_send, strlen(msg_to_send), 0) < 0){
        perror("LOGOUT_ACK");
    }
}

void process_exit(struct message msg, int sockfd){
    int i = get_client_index_with_name(msg);
    strcpy(clients[i].username, "-1");
    strcpy(clients[i].password, "-1");
    clients[i].sockfd = -1;
    strcpy(clients[i].session_id, "-1");
    clients[i].is_logged_in = false;
}

void process_leavesession(struct message msg, int sockfd){

    int index = get_client_index_with_name(msg);
    // char *original_session = clients[index].session_id;
    strcpy(clients[index].session_id, "lobby");

    struct message msg_packet = make_message(LV_SESS_ACK, 0, msg.source, "-1");
    char *msg_to_send = serialize(msg_packet);

    // send the ack message packet back
    if (send(sockfd, msg_to_send, strlen(msg_to_send), 0) < 0){
        perror("LV_SESS_ACK");
    }
}


int find_session(char *session_id){
    int i;
    for(i =0; i < MAX_SESSIONS; i++){
       if(strcmp(session_list[i].session_id,session_id) == 0){
        return i;
       }
    }
    return -1;
}

bool username_available(char *username){
    for (int i = 0; i < MAXUSERS; i++){
        // find the index of the user with the same name
        if (strcmp(clients[i].username, username)==0){
            return false;
        }
    }
    return true;
}

bool user_registered(char *username, char *password){
    for (int i = 0; i < MAXUSERS; i++){
        // find the index of the user with the same name
        if ((strcmp(regClients[i].username, username)==0) && (strcmp(regClients[i].password, password)==0)){
            return true;
        }
    }
    return false;
}

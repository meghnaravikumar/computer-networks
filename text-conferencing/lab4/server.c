/**
 * Based on: 7.3 select()—Synchronous I/O Multiplexing, Old School
*/

#include "utils.h"

typedef struct user_data{
    bool in_session;
    int fd;
    unsigned char user_name[MAX_NAME];
    unsigned char user_pwd[MAX_NAME];
    unsigned char user_sesh[MAX_NAME];
} user_data;

/* global variable declarations */
static user_data users_list[MAX_USERS];
int num_users = 0;

/* function declarations */
bool cmd_login(message* msg_serv, int fd);
bool cmd_refresh(message* msg_serv);
void cmd_list();
bool sesh_check();
int fd_get_index(int fd);
int name_get_index(message* msg_serv);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    for (int i = 0; i < MAX_USERS; ++i){
        strcpy(users_list[i].user_name, "user_def");
        strcpy(users_list[i].user_pwd, "pwd_def");
        strcpy(users_list[i].user_sesh, "sesh_def");
        users_list[i].fd = -1;
        users_list[i].in_session = false;
    }

    if(argc != 2){
        printf("Must be in this form: server <port number>.\n");
        exit(0);
    }

    char* port = argv[1];

    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[MAX_DATA];    // buffer for client data
    char copy_of_buf[MAX_DATA];
    int nbytes;

	char remoteIP[INET_ADDRSTRLEN]; // this used to be INET6_ADDSTRLEN

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
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
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

    fprintf(stdout, "Server binded successfully\n");

	// if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

	freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    // add the listener to the master set
    FD_SET(listener, &master);

    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    // for the message that we receive from the clients
    message msg_serv, ack, msg_to_be_sent;
    char send_ack[MAX_DATA];
    char send_list[MAX_DATA];
    char copy_of_send_list[MAX_DATA];

    // main loop
    for(;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        int sIdx;
        int recvIndex;
        for(i = 0; i <= fdmax; i++) {
            bzero(buf, sizeof buf);
            sIdx = fd_get_index(i);
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
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
							inet_ntop(remoteaddr.ss_family,
								get_in_addr((struct sockaddr*)&remoteaddr),
								remoteIP, INET6_ADDRSTRLEN),
							newfd);
                    }
                } else {

                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            users_list[sIdx].in_session = false;
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        msg_serv.type = -1;
                        msg_serv.size = MAX_DATA;
                        bzero(msg_serv.source, MAX_NAME);
                        bzero(msg_serv.data, MAX_DATA);

                        msg_serv = messagify(buf);

                        if (msg_serv.type == LOGIN){
                            fprintf(stdout,  "Server has recieved the login request at socket id %d.\n", i);
                            ack.type = LO_NAK;
                            if(cmd_login(&msg_serv, i)) ack.type = LO_ACK;
                            stringify(ack.type, 0, "", "", send_ack);
                            if (send(i, send_ack, strlen(send_ack), 0) < 0) perror("login");
                        }else if (msg_serv.type == JOIN){
                            fprintf(stdout, "Server has recieved the request to join %s.\n", msg_serv.data);
                            ack.type = JN_NAK;
                            if(sesh_check(msg_serv.data)){
                                if (cmd_refresh(&msg_serv)){
                                    fprintf(stdout, "Server successfuly updated session to %s.\n", msg_serv.data);
                                    ack.type = JN_ACK;
                                }
                            }
                            stringify(ack.type, 0, "", "", send_ack);
                            if (send(i, send_ack, strlen(send_ack), 0) < 0) perror("join");
                        } else if(msg_serv.type == LEAVE_SESS){
                            char sesh_exit[50];
                            strcpy(sesh_exit, users_list[sIdx].user_sesh);
                            strcpy(users_list[name_get_index(&msg_serv)].user_sesh, "lobby");
                            strcpy(users_list[sIdx].user_sesh, "lobby");
                        }else if(msg_serv.type == NEW_SESS){
                            fprintf(stdout, "Server has recieved the new session request.\n");
                            char sesh_exit[50];
                            strcpy(sesh_exit, users_list[sIdx].user_sesh);
                            strcpy(users_list[sIdx].user_sesh, "lobby");
                            ack.type = NS_NAK;
                            if(!sesh_check(msg_serv.data)){
                                if (cmd_refresh(&msg_serv)) ack.type = NS_ACK;
                            }
                            stringify(ack.type, 0, "", "", send_ack);
                            if (send(i, send_ack, strlen(send_ack), 0) < 0) perror("create");
                        } else if(msg_serv.type == QUERY){
                            cmd_list(send_list);
                            printf("Returned query string:\n %s", send_list);
                            stringify(QU_ACK, 0, "", send_list, copy_of_send_list);
                            if (send(i, copy_of_send_list, MAX_DATA, 0) == -1) perror("query");
                        }else {
                            if(strcmp(users_list[sIdx].user_sesh, "lobby") == 0) continue;
                            for(j = 0; j <= fdmax; j++) {
                                if (FD_ISSET(j, &master)) {
                                    if (j != listener && j != i) {
                                        recvIndex = fd_get_index(j);
                                        if (users_list[recvIndex].fd == -1) continue;
                                        if ((strcmp(users_list[recvIndex].user_sesh, users_list[sIdx].user_sesh) == 0) && users_list[recvIndex].in_session){
                                            msg_to_be_sent.type = MESSAGE;
                                            msg_to_be_sent.size = MAX_DATA;
                                            strcpy(msg_to_be_sent.source, users_list[sIdx].user_name);
                                            strcpy(msg_to_be_sent.data, buf);
                                            stringify(MESSAGE, MAX_DATA, users_list[sIdx].user_name, msg_serv.data, copy_of_buf);
                                            if (send(j, copy_of_buf, nbytes, 0) == -1) perror("send");
                                        }
                                    }
                                }
                            }
                            bzero(copy_of_buf, sizeof copy_of_buf);
                            bzero(buf, sizeof buf);
                        }
                    }
                    bzero(copy_of_buf, sizeof copy_of_buf);
                    bzero(buf, sizeof buf);
                } // END handle data from client
            } // END got new incoming connection
       } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

    return 0;
}

bool cmd_refresh(message* msg_serv){
    int clientIndex = name_get_index(msg_serv);
    if (users_list[clientIndex].in_session == false){
        fprintf(stdout, "User is not logged in.\n");
        return false;
    }else{
        strcpy(users_list[clientIndex].user_sesh, msg_serv->data);
        printf("User has been moved to %s.\n", users_list[clientIndex].user_sesh);
        return true;
    }
}

void cmd_list(char* query_list){
    int sessArrayInd = 0;
    char users_online[MAX_DATA];
    bzero(users_online, MAX_DATA);
    char avail_sesh[MAX_DATA];
    bzero(avail_sesh, MAX_DATA);
    strcpy(avail_sesh, "Available sessions: \n");
    strcpy(users_online, "Users online: \n");
    for(int i = 0; i < MAX_USERS; i++){
        if(strcmp(users_list[i].user_name, "user_def")==0){
            break;
        }else{
            if(users_list[i].in_session){
                strcat(users_online, users_list[i].user_name);
                strcat(users_online, "\n");
                int active_cnt = 0;
                if(strcmp(users_list[i].user_sesh, "lobby") !=0){
                    if (strstr(avail_sesh, users_list[i].user_sesh) == NULL){
                        strcat(avail_sesh,  users_list[i].user_sesh);
                        strcat(avail_sesh, "\n");
                    }
                }
            }
        }
    }
    strcat(users_online, avail_sesh);
    strcpy(query_list, users_online);
    printf("%s\n", query_list);
    return;
}

bool sesh_check(char *sessionID){
    for(int i = 0; i < MAX_USERS; i++){
        user_data current_client = users_list[i];
        if(strcmp(users_list[i].user_name, "user_def") == 0){
            return false;
        }else {
            if(!users_list[i].in_session) {
                continue;
            }else{
                if(strcmp(current_client.user_sesh, sessionID) == 0){
                    return true;
                 }
            }
        }
    }
    return false;
}

int fd_get_index(int fd){
    for (int recv_fd = 0; recv_fd < MAX_USERS; recv_fd++){
        if (users_list[recv_fd].fd == fd){
            return recv_fd;
        }
    }
    return -1;
}

int name_get_index(message* msg_serv){
    for (int i = 0; i < MAX_USERS; i++){
        if (strcmp(users_list[i].user_name, msg_serv->source)==0){
            return i;
        }
    }
    return -1;
}

/* note to self: change this before submitting */
bool cmd_login(message* msg_serv, int fd){
    user_data current_client;
    int i = 0;
    do{
        if(strcmp(users_list[i].user_name, "user_def")==0){
            strcpy(current_client.user_name, msg_serv->source);
            strcpy(current_client.user_pwd, msg_serv->data);
            current_client.in_session = true;
            strcpy(current_client.user_sesh, "lobby");
            current_client.fd = fd;
            users_list[i] = current_client;
            num_users = i;
            return true;
        }else{
            user_data temp_client = users_list[i];
            if (strcmp(temp_client.user_name, msg_serv->source) == 0){
                if (temp_client.in_session){
                    return false;
                }else {
                    strcpy(users_list[i].user_name, msg_serv->source);
                    users_list[i].in_session = true;
                    users_list[i].fd = fd;
                    return true;
                }
            }
        }
        i++;
    } while(i < MAX_USERS);
    return false;
}

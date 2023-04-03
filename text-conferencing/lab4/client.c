/*
** client.c -- a stream socket client demo
*/

#include "utils.h"

/************** function declarations **************/
void *recv_handler(void *);
void login(char *buf);
void createsession(char *buf);
void joinsession(char *buf);

/************** global variable declarations **************/
bool client_logged_in = false;
char username[100];
int sockfd;
pthread_mutex_t lock;

int main(int argc, char *argv[]){

    char buf[MAXBUFLEN];
    char msg_to_send[MAXBUFLEN];

    // keep recieving messages using this thread
    pthread_t recv_thread;

    while(1){

        memset(&buf, sizeof(buf), 0);
        fgets(buf, 100, stdin);

        /************** login **************/
        if((strncmp(buf, "/login", 6))== 0){
            login(buf);
            if(pthread_create(&recv_thread, NULL, recv_handler, (void*)(intptr_t) sockfd) < 0){
                perror("thread creation");
                return 2;
            }
        /************** quit **************/
        }else if((strncmp(buf, "/quit", 5))== 0){
            exit(0);
        /************** create session **************/
        }else if(strncmp(buf, "/createsession", 14) == 0){
            createsession(buf);
            if(pthread_create(&recv_thread, NULL, recv_handler, (void*)(intptr_t) sockfd) < 0){
                perror("thread creation");
                return 2;
            }
        /************** join session **************/
        }else if(strncmp(buf, "/joinsession", 12) == 0){
            joinsession(buf);
            if(pthread_create(&recv_thread, NULL, recv_handler, (void*)(intptr_t) sockfd) < 0){
                perror("thread creation");
                return 2;
            }
        /************** message **************/
        }else{
            // make the message packet
            memset(&msg_to_send, sizeof(msg_to_send), 0);
            sprintf(msg_to_send, "%d&%d&%s&%s&", MESSAGE, strlen(buf), username, buf);
            // sprintf(msg_to_send, "%s: %s", username, buf);

            if(send(sockfd, msg_to_send, strlen(msg_to_send), 0) < 0){
                fprintf(stderr, "Could not send message.");
                return 1;
            }
        }

        memset(&buf, sizeof(buf), 0);
    }

    pthread_join(recv_thread, NULL);
    close(sockfd);

    return 0;
}

void *recv_handler(void *sock_fd){
    char buf[MAXBUFLEN];
    int numbytes;
    memset(&buf, sizeof(buf), 0);
    while(1){
        if((numbytes = recv((intptr_t)sock_fd, buf, MAXBUFLEN-1, 0)) == -1){
            perror("recv");
            exit(1);
        }else{
            buf[numbytes] = '\0';
            struct message msg = deserialize(buf);
            if(msg.type == LO_ACK){
                client_logged_in = true;
                fprintf(stdout, "[%s is logged into to the server!]\n", msg.source);
            }else if(msg.type == LO_NAK){
                fprintf(stdout, "[%s could not be logged into the server.]\n", msg.source);
                fprintf(stdout, "[Reason: %s]\n", msg.data);
            }else if(msg.type == MESSAGE){
                fprintf(stdout, "%s: %s\n", msg.source, msg.data);
            }else if(msg.type == NS_ACK){
                fprintf(stdout, "%s successfully created %s\n", msg.source, msg.data);
            }else if(msg.type == JN_ACK){
                fprintf(stdout, "%s successfully joined %s\n", msg.source, msg.data);
            }else{
                fprintf(stdout, "%s", buf);
            }
        }
        memset(&buf, sizeof(buf), 0);
    }
}

void login(char *buf){
    char *port, *server_ip, *password;

    int nargs = 0;
    char *args[30];
    char copy[MAXBUFLEN];
    strcpy(copy, buf);
    copy[strcspn(copy, "\n")] = '\0';
    char *tok;

    tok = strtok(copy, " ");
    while (tok != NULL) {
        args[nargs] = tok;
        tok = strtok(NULL, " ");
        nargs++;
    }

    //if(nargs != 5) exit(0);

    memset(&username, sizeof(username), 0); // clean the username buffer
    strcpy(username, args[1]);

    port = args[4];
    server_ip = args[3];
    password = args[2];

    int numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(server_ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(1);
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(2);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    fprintf(stdout, "[client: connecting to %s]\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    pthread_mutex_lock(&lock);

    //fprintf(stdout, "%s\n", password);

    struct message msg_packet = make_message(LOGIN, strlen(password), username, password);
    char *msg_to_send = serialize(msg_packet);
    //fprintf(stdout, "%s", msg_to_send);
    if(send(sockfd,msg_to_send,strlen(msg_to_send), 0) < 0){
        printf("oof \n");
    }

    memset(&msg_to_send, sizeof(msg_to_send), 0);
    memset(&msg_packet, sizeof(msg_packet), 0);

    pthread_mutex_unlock(&lock);
    // confirmation output to the user
    //fprintf(stdout, "[%s is logged into to the server!]\n", username);
}

void createsession(char *buf){
    int nargs = 0;
    char *args[30];
    char copy[MAXBUFLEN];
    strcpy(copy, buf);
    copy[strcspn(copy, "\n")] = '\0';
    char *tok;

    tok = strtok(copy, " ");
    while (tok != NULL) {
        args[nargs] = tok;
        tok = strtok(NULL, " ");
        nargs++;
    }

    //if(nargs != 2) exit(0);

    char *session_id = args[1];

    // fprintf(stdout, "%s\n", session_id);

    struct message msg_packet = make_message(NEW_SESS, strlen(session_id), username, session_id);
    char *msg_to_send = serialize(msg_packet);

    if(send(sockfd,msg_to_send,strlen(msg_to_send), 0) < 0){
        printf("oof \n");
    }

    memset(&msg_to_send, sizeof(msg_to_send), 0);
    memset(&msg_packet, sizeof(msg_packet), 0);

    pthread_mutex_unlock(&lock);

}

void joinsession(char *buf){
    int nargs = 0;
    char *args[30];
    char copy[MAXBUFLEN];
    strcpy(copy, buf);
    copy[strcspn(copy, "\n")] = '\0';
    char *tok;

    tok = strtok(copy, " ");
    while (tok != NULL) {
        args[nargs] = tok;
        tok = strtok(NULL, " ");
        nargs++;
    }

    char *session_id = args[1];

    struct message msg_packet = make_message(JOIN, strlen(session_id), username, session_id);
    char *msg_to_send = serialize(msg_packet);

    if(send(sockfd,msg_to_send,strlen(msg_to_send), 0) < 0){
        printf("oof \n");
    }

    memset(&msg_to_send, sizeof(msg_to_send), 0);
    memset(&msg_packet, sizeof(msg_packet), 0);

    pthread_mutex_unlock(&lock);
}

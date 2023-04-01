/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <pthread.h>

#include "utils.h"

void *receive_handler(void *);

int main(int argc, char *argv[]){

    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char *port, *server_ip;
    char buf[MAXBUFLEN];
    char msg_to_send[MAXBUFLEN];
    char username[MAXBUFLEN];

    if (argc != 3) {
        fprintf(stderr,"usage: client server-ip-addr server-port\n");
        exit(1);
    }

    server_ip = argv[1];
    port = argv[2];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(server_ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
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
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    fprintf(stdout, "client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    // get the username
    fprintf(stdout, "Enter your username: ");
    memset(&username, sizeof(username), 0); // clean the username buffer
    fgets(username, MAXBUFLEN, stdin);
    username[strlen(username)-1] = '\0'; // get rid of the newline character

    // keep recieving messages using this thread
    pthread_t recv_thread;

    if(pthread_create(&recv_thread, NULL, receive_handler, (void*)(intptr_t) sockfd) < 0){
        perror("could not create thread");
        return 1;
    }

    // confirmation output to the user
    fprintf(stdout, "[%s is connected to the server!]\n", username);
    fprintf(stdout, "[Type '/logout' to logout]\n");

    while(1){

        memset(&buf, sizeof(buf), 0);
        fgets(buf, 100, stdin);
        if((strncmp(buf, "/logout", 7))== 0){
            exit(0);
        }

        // make the message packet
        memset(&msg_to_send, sizeof(msg_to_send), 0);
        sprintf(msg_to_send, "%s: %s", username, buf);

        if(send(sockfd, msg_to_send, strlen(msg_to_send), 0) < 0){
            fprintf(stderr, "Could not send message.");
            return 1;
        }

        memset(&buf, sizeof(buf), 0);
    }

    pthread_join(recv_thread, NULL);
    close(sockfd);

    return 0;
}

void *receive_handler(void *sock_fd){
    char buf[MAXBUFLEN];
    int numbytes;

    while(1){
        if((numbytes = recv((intptr_t)sock_fd, buf, MAXBUFLEN-1, 0)) == -1){
            perror("recv");
            exit(1);
        }else{
            buf[numbytes] = '\0';
        }
        printf("%s", buf);
    }
}

/**
 * @file server.c
 * @author Meghna Ravikumar & Ankur Samanta
 * @date March 13, 2023
 *
 * @cite
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>
#include "message.h"
#include "server_utils.h"

#define PORT "5000" // port we're listening on

int main(int argc, char *argv[])
{
    /* variable declarations */
    char cmd[100];
    int port_num;

    /* get initial server startup command */
    fgets(cmd, sizeof(cmd), stdin);
    port_num = server_cmd(cmd);

    if(port_num != 1){
        fprintf(stdout, "Starting server at port %d...\n", port_num);
    }

    // code from Beej's
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    int newfd;        // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;

    char buf[256];    // buffer for client data
    int nbytes;

    // added this in
    char message[MAX_DATA];
    char response[MAX_DATA];

    char remoteIP[INET6_ADDRSTRLEN];

    int yes = 1;        // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }

        // lose the pesky "address already in use" error message
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
            exit(1);
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

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

    /**
     * when a socket is ready to be read, we're either reading from a new
     * connection or from an already connected client.
    */
    while(1){ // accepts multiple connection requests

        read_fds = master;
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        // looping through all socket fds
        for(int sockfd = 0; sockfd < fdmax + 1; sockfd++){
            if(FD_ISSET(sockfd, &read_fds)){ // a socket is ready to be read
                if(sockfd == listener){ // make a new connection
                    // 3 way handshake
                    socklen_t sin = sizeof(remoteaddr);
                    sockfd2 = accept(listener, (struct sockaddr *)&remoteaddr, &sin);

                    FD_SET(sockfd2, &master); // add new socket fd to master
                    if(sockfd2 > fdmax) fdmax = sockfd2; // keep track of max fd

                }else{ // read from an existing connection
                    nbytes = recv(sockfd, message, MAX_DATA - 1, 0);
                    message[nbytes] = '\0';

                    if(nbytes > 0){
                        // decode the message
                        struct message *recv_msg;
                        recv_msg = decode_message(message);



                    }
                }
            }
        }
    }

    return 0;
}

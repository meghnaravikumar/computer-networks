/**
 * @file server.c
 * @author Meghna Ravikumar & Ankur Samanta
 * @version 1.0
 * @date 2023-01-22
 *
 * @cite Beej's Guide 6.3 (Datagram Sockets)
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

#define MAXBUFLEN 100

void error_msg(char *message){
    fprintf(stderr, message);
    exit(1);
}

int main(int argc, char *argv[]) {

    // variable declarations
    int port, numbytes, sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buf[MAXBUFLEN] = {0};
    char *rsp = "no";

    // check for correct execution command structure
    if(argc != 2) error_msg("Incorrect format. Must be: server -<UDP listen port>\n");

    // get port number
    port = atoi(argv[1]);

    // initialize socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sockfd == -1) error_msg("Failed to create socket\n");

    // get and store server info
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    // bind to socket
    if((bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr))) == -1){
        error_msg("Bind to socket error.\n");
    }

    // recieve message from client, store in client_addr
    if(recvfrom(sockfd, buf, MAXBUFLEN, 0, (struct sockaddr *) &client_addr, &client_len) == -1){
        error_msg("Failed to recieve.\n");
    }

    // send client "yes" if the message is ftp
    if(strcmp(buf, "ftp") == 0) rsp = "yes";

    if((sendto(sockfd, rsp, strlen(rsp), 0, (struct sockaddr *) &client_addr, sizeof(client_addr))) == -1){
        error_msg("sendto error\n");
    }

    close(sockfd);

    return 0;

}

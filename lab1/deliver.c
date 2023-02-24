/**
 * @file deliver.c
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

int main(int argc, char const *argv[]){

    // variable declarations
    int port;
    int numbytes;
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[MAXBUFLEN] = {0};
    char filename[MAXBUFLEN] = {0};
    char msg[MAXBUFLEN] = {0};
    socklen_t server_addr_size = sizeof(server_addr);

    // check for correct execution command structure
    if(argc != 3) error_msg("Format must be: deliver <server address> <server port number>.\n");

    // create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sockfd == -1) error_msg("Failed to create socket.\n");

    // get and store server info
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));
    server_addr.sin_family = AF_INET;
    port = atoi(argv[2]);
    server_addr.sin_port = htons(port);

    // convert IP address string to "network" form
    if(inet_aton(argv[1], &server_addr.sin_addr) == 0) error_msg("Failed to create socket: could not convert IP address string to 32-bit unsigned long.\n");

    // get input from the user
    fgets(buf, MAXBUFLEN, stdin);

    // check for valid input from user
    char *p = strtok(buf, " ");

    if(p[0] != 'f' || p[1] != 't'  || p[2] != 'p') error_msg("Incorrect format. Missing argument: 'ftp'.\n");

    //char *filename;
    p = strtok(NULL, " ");
    if(p) strcpy(filename, p);
    filename[strlen(filename)-1] = '\0';

    // check for file existence
    if(access(filename, F_OK) == -1) error_msg("File does not exist.\n");

    // send message to server server
    if ((numbytes = sendto(sockfd, "ftp", strlen("ftp"), 0, (struct sockaddr *) &server_addr, sizeof(server_addr))) == -1){
        error_msg("Failed to send message to the server.\n");
    }

    // recieve message from server
    if((numbytes = recvfrom(sockfd, msg, MAXBUFLEN, 0, (struct sockaddr *) &server_addr, &server_addr_size)) == -1){
        error_msg("Failed to recieve message from the server.\n");
    }

    if(strcmp(msg, "yes") == 0) {
        fprintf(stdout, "A file transfer can start.\n");
    }else{
        exit(1);
    }

    close(sockfd);

    return 0;

}

/**
 * @file deliver.c
 * @author Meghna Ravikumar & Ankur Samanta
 * @version 1.0
 * @date 2023-02-27
 *
 * @cite Beej's Guide
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
#include <time.h>
#include "packet.h"

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
    clock_t start, end, rtt;
    double n = 0;

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

    // measure the round-trip time (RTT) from the client to the server
    start = clock(); // start the clock

    // send message to server server
    if ((numbytes = sendto(sockfd, "ftp", strlen("ftp"), 0, (struct sockaddr *) &server_addr, sizeof(server_addr))) == -1){
        error_msg("Failed to send message to the server.\n");
    }

    // recieve message from server
    if((numbytes = recvfrom(sockfd, msg, MAXBUFLEN, 0, (struct sockaddr *) &server_addr, &server_addr_size)) == -1){
        error_msg("Failed to recieve message from the server.\n");
    }

    end = clock(); // end the clock
    fprintf(stdout, "Round-trip time (RTT): %f s.\n", (double)(end - start)/CLOCKS_PER_SEC);
    rtt = end - start; // store rtt

    /* SECTION 3 - config timeout */
    struct timeval to;
    // check below init values
    double tval = rtt / CLOCKS_PER_SEC;
    if (tval > 1) {
        tval = 1;
    }
    to.tv_sec = tval;
    to.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to)) < 0) {
		printf("Config timeout failure.\n");
		exit(1);
	}

    if(strcmp(msg, "yes") == 0) {
        fprintf(stdout, "A file transfer can start.\n");
    }else{
        exit(1);
    }

    /* SECTION 2.2 BEGINS HERE */

    // open file in binary mode
	FILE *fp = fopen(filename, "rb");

    // get size of file in bytes (https://www.techiedelight.com/find-size-of-file-c/)
    fseek(fp, 0, SEEK_END); // seek to the EOF
    int fsize = ftell(fp);  // get the current position
    rewind(fp);             // rewind to the beginning of file - equiv. to fseek(stream, 0, SEEK_SET)

    // split up the packet into fragments
    // size allows for the range of 0 to 1000, so the max # of fragments will be 1000
    int num_frags = (fsize/MAX_PACKET_LEN) + 1;

    char frag_data[MAX_PACKET_LEN];
    int packet_length;
    char* packet_content;
    int index;

    // init the array with the packet info and send each out
    for(int frag_no = 0; frag_no < num_frags; frag_no++){

        struct packet pk = {0};

        // fill in the struct data
        pk.total_frag = num_frags;
        pk.frag_no = frag_no + 1;
        pk.size = ((frag_no + 1) != num_frags) ? MAX_PACKET_LEN : (fsize - 1) % MAX_PACKET_LEN  + 1;
        pk.filename = filename;
        memset(pk.filedata, 0, sizeof(char) * (MAX_PACKET_LEN));
        fread((void*)pk.filedata, sizeof(char), MAX_PACKET_LEN, fp);

        // turn the packet into a string
        packet_length = get_packet_length(pk) + pk.size;
		packet_content = malloc(packet_length * sizeof(char));
		index = sprintf(packet_content, "%d:%d:%d:%s:", pk.total_frag, pk.frag_no, pk.size, pk.filename);
		memcpy(&packet_content[index], pk.filedata, pk.size);

        // sends packet to server
		if(sendto(sockfd, packet_content, packet_length, 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1){
            error_msg("Failed to send packet.\n");
        }

        if(recvfrom(sockfd, msg, MAXBUFLEN, 0, (struct sockaddr *)&server_addr, &server_addr_size) == -1){
            // check if a timeout has occurred
		    while((errno == EAGAIN || errno == EWOULDBLOCK)){
			    // SECTION 3 CODE CONTINUED BELOW:
                printf("(Timeout), trying to resend packet #%d\n", pk.frag_no);
                recvfrom(sockfd, msg, MAXBUFLEN, 0, (struct sockaddr *)&server_addr, &server_addr_size);
		    }
        }

        while(strcmp(msg, "dropped") == 0){
            printf("Trying to resend packet #%d\n", pk.frag_no);
            if(recvfrom(sockfd, msg, MAXBUFLEN, 0, (struct sockaddr *)&server_addr, &server_addr_size) == -1){
                    break;
            }
        }

        if(strcmp(msg, "ACK") == 0){
            printf("ACK recieved.\n");
        }

		free(packet_content);

    }

    // close socket connection
    close(sockfd);

    return 0;

}

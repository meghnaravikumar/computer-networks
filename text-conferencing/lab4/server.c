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

    return 0;
}

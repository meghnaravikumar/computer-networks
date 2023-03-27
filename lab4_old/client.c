/**
 * @file client.c
 * @author Meghna Ravikumar & Ankur Samanta
 * @date March 13, 2023
 *
 * @cite
 *
 */


#include "message.h"
#include "client_utils.h"

int main(int argc, char *argv[])
{
    /* variable declarations */
    char cmd[1000]; // user command
    char buf[100];
    int client = 0;

    /* get initial client startup command */
    while(client != 1){
        fprintf(stdout, "Run the program: ");
        fgets(buf, sizeof(buf), stdin);
        client = client_init(buf);
        if(client == 1){
            fprintf(stdout, "Welcome!\n");
            break;
        }else{
            fprintf(stdout, "Incorrect command. To run the program, type 'client'.\n");
        }
    }


    while(1){
        showmenu("Text conferencing menu:", opsmenu);
        fgets(cmd, sizeof(cmd), stdin);
        menu_execute(cmd, 1);
    }



    return 0;
}

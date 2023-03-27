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

#define MAX_NAME 50
#define MAX_DATA 1000
#define MAX_USERS 20

typedef struct message
{
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
} message;

enum
{
    LOGIN, // <password>, Login with the server
    LO_ACK, // Acknowledge successful login
    LO_NAK, // <reason for failure>, Negative acknowledgement of login
    EXIT, // Exit from the server
    JOIN, // <session ID>, Join a conference session
    JN_ACK, // <session ID>, Acknowledge successful conference session join
    JN_NAK, // <session ID, reason for failure>, Negative acknowledgement of joining the session
    LEAVE_SESS, // Leave a conference session
    NEW_SESS, // <session ID>, Create new conference session
    NS_ACK, // Acknowledge new conference session
    NS_NAK,
    MESSAGE, // <message data>, Send a message to the session or display the message if it is received
    QUERY, // Get a list of online users and available sessions
    QU_ACK // <users and sessions>, Reply followed by a list of users online
} message_type;

struct message messagify(char* msgString){

    struct message msgStruct;
    msgString[strcspn(msgString, "\n")] = '\0';

    char *tok;
    int nargs = 0;
    char *args[5];

    tok = strtok(msgString, ":");
    while (tok != NULL) {
        args[nargs] = tok;
        tok = strtok(NULL, ":");
        nargs++;
    }

    msgStruct.type = atoi(args[0]);
    msgStruct.size =  atoi(args[1]);
    if(args[2]) strcpy(msgStruct.source, args[2]);
    if(args[3]) strcpy(msgStruct.data, args[3]);

    return msgStruct;
}

void stringify(int type, int size, char* source, char* data, char* str){
    struct message msg;
    msg.type = type;
    msg.size = size;
    strncpy(msg.source, source, MAX_NAME);
    strncpy(msg.data, data, MAX_DATA);
    int str_temp = sprintf(str, "%u:%u:%s:%s",msg.type, msg.size, msg.source, msg.data);
    return;
}

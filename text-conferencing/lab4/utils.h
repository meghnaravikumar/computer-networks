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
#include <stdbool.h>

#define MAXBUFLEN 100
#define MAXUSERS 50
#define MAX_DATA 1000
#define MAX_NAME 50
#define MAX_SESSIONS 5

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

struct client{
    unsigned char username[MAX_NAME];
    unsigned char password[MAX_NAME];
    int sockfd;
    unsigned char session_id[MAX_NAME];
    bool is_logged_in;
};

struct message {
    unsigned int type;
    unsigned int size; // the length of the data
    unsigned char source[MAX_NAME]; // id of the client sending the message
    unsigned char data[MAX_DATA];
};

char *serialize(struct message msg){
    char *serialized_msg;
    memset(&serialized_msg, sizeof(serialized_msg), 0);

    // fprintf(stdout, "serialized %d\n", msg.type);
    // fprintf(stdout, "%d\n", msg.size);
    // fprintf(stdout, "%s\n", msg.source);
    // fprintf(stdout, "%s\n", msg.data);

    sprintf(serialized_msg, "%d&%d&%s&%s&", msg.type, msg.size, msg.source, msg.data);
    return serialized_msg;
}

struct message deserialize(char *msg){
    struct message deserialized_msg;
    memset(&deserialized_msg, sizeof(deserialized_msg), 0);

    int nargs = 0;
    char *args[5];
    char copy[MAXBUFLEN];
    strcpy(copy, msg);
    copy[strcspn(copy, "\n")] = '\0';
    char *tok;

    char* rest;

    tok = strtok(copy, "&");
    while (tok != NULL) {
        args[nargs] = tok;
        //fprintf(stdout, "check %s\n", tok);
        tok = strtok(NULL, "&");
        nargs++;
    }


    memset(&deserialized_msg, sizeof(deserialized_msg), 0);

    deserialized_msg.type = atoi(args[0]);
    deserialized_msg.size = atoi(args[1]);
    strcpy(deserialized_msg.source, args[2]);
    strcpy(deserialized_msg.data, args[3]);

    // fprintf(stdout, "deserialized %d\n", deserialized_msg.type);
    // fprintf(stdout, "%d\n", deserialized_msg.size);
    // fprintf(stdout, "%s\n", deserialized_msg.source);
    // fprintf(stdout, "%s\n", deserialized_msg.data);

    return deserialized_msg;
}

struct message make_message(int type, int size, char *source, char *data){
    struct message msg;
    memset(&msg, sizeof(msg), 0);
    msg.type = type;
    msg.size = size;
    strcpy(msg.source, source);
    strcpy(msg.data, data);
    return msg;
}

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
    MESSAGE, // <message data>, Send a message to the session or display the message if it is received
    QUERY, // Get a list of online users and available sessions
    QU_ACK, // <users and sessions>, Reply followed by a list of users online
    LOGOUT,
    LOGOUT_ACK,
    LV_SESS_ACK,
    DM
} message_type;

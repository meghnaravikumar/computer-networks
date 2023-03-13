#ifndef MESSAGE_H
#define MESSAGE_H

#define MAX_NAME 1000
#define MAX_DATA 1000

char buf[BUFSIZ];
//NOTE: is it ok to globally define like this? or should we redefine it each time we use?
int num_bytes;

struct message
{
    unsigned int type;
    unsigned int size; // length of the data
    unsigned char source[MAX_NAME]; // ID of the client sending the message
    unsigned char data[MAX_DATA];
} message;

/**
 * @brief Contains the control packet type.
 * Comments specify the <packet data type>, function.
*/
typedef enum
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
    QU_ACK // <users and sessions>, Reply followed by a list of users online
} message_type;

struct message create_message(unsigned int type, unsigned int size, const unsigned char *source, const unsigned char *data) {
    struct message msg;
    msg.type = type;
    msg.size = size;
    if (source) strncpy(msg.source, source, MAX_NAME);
    if (data) strncpy(msg.data, data, MAX_DATA);
    return msg;
}

void encode_message(const struct message msg, char *temp) {
    memset(temp, 0, sizeof(char) * BUFSIZ);
    sprintf(buf, "%d:%d:%s:", msg.type, msg.size, msg.source);
    strcat(temp, buf);
    strncat(temp, msg.data, BUFSIZ - strlen(temp) - 1);
}


#endif

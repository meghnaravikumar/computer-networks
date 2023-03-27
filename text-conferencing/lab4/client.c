/**
 * Based on: 6.2 A Simple Stream Client from Beej's Guide
*/

#include "utils.h"

/* variable declarations */
char msg[MAX_DATA];
char username[MAX_NAME];
int sockfd;
struct message msg_to_send;
struct sockaddr_in srvadr;
pthread_mutex_t lock;
bool in_session = false;
pthread_t recv_thread;

/* function declarations */
void cmd_login(char **args);
void stringify_and_send(int type, char *source, char *data);
void clear_message();
bool check_user_state();

void *recv_handler(void *sockfd)
{
	int sock = *((int *)sockfd);
	int len;
    struct message recv_message;
	while((len = recv(sock,msg,MAX_DATA,0)) > 0) {
        msg[len] = '\0';
        recv_message = messagify(msg);
        if (recv_message.type == LO_ACK){
            printf("Login was successful.\n");
            in_session = true;
        }else if (recv_message.type == LO_NAK){
            printf("Login was unsuccessful.\n");
            in_session = false;
        }else if (recv_message.type == JN_ACK){
            printf("Joined session successfully.\n");
        }else if (recv_message.type == JN_NAK){
            printf("Unable to join session.\n");
        }else if (recv_message.type == NS_ACK){
            printf("Created session succesfully.\n");
        }else if (recv_message.type == QU_ACK){
            printf("%s", recv_message.data);
        }else if (recv_message.type == MESSAGE){
            printf("%s: %s", recv_message.source, recv_message.data);
        }
        else{
            fputs(msg,stdout);
        }
	}
}

int main(void){

	int len;
	char send_msg[MAX_DATA];
	struct sockaddr_in ServerIp;

	while(fgets(msg, MAX_DATA, stdin) > 0) {

        int nargs = 0;
        char *args[30];
        char copy[MAX_DATA];
        strcpy(copy, msg);
        copy[strcspn(copy, "\n")] = '\0';
        char *tok;

        tok = strtok(copy, " ");
        while (tok != NULL) {
            args[nargs] = tok;
            tok = strtok(NULL, " ");
            nargs++;
        }

	    if(strcmp(args[0], "/login") == 0){ // login
            if (in_session){
                printf("Already logged in. \n");
                continue;
            }
            cmd_login(args);
			pthread_create(&recv_thread, NULL, (void *)recv_handler, &sockfd);
        }else if(strcmp(args[0],  "/logout") == 0){ // logout
            fprintf(stdout, "%s is now logged out.\n", username);
            close(sockfd);
            in_session = false;
            pthread_create(&recv_thread, NULL, (void *)recv_handler, &sockfd);
        }else if(strcmp(args[0],  "/joinsession") == 0){ // joinsession
            if(check_user_state()){
                char* seshid = args[1];
                seshid[strcspn(seshid, "\n")] = '\0';
                stringify_and_send(JOIN, username, seshid);
                clear_message();
            }
            pthread_create(&recv_thread, NULL, (void *)recv_handler, &sockfd);
        }else if(strcmp(args[0], "/leavesession") == 0){ // leavesession
            if(check_user_state()){
                printf("%s is leaving session. \n", username);
                stringify_and_send(LEAVE_SESS, username, "");
                clear_message();
            }
            pthread_create(&recv_thread, NULL, (void *)recv_handler, &sockfd);
        }else if(strcmp(args[0], "/list") == 0){ // list
            if(check_user_state()){
                printf("Listing users and available sessions:");
                stringify_and_send(QUERY, "", "");
                clear_message();
            }
            pthread_create(&recv_thread, NULL, (void *)recv_handler, &sockfd);
        }else if(strcmp(args[0], "/quit") == 0){ // quit
            fprintf(stdout, "%s is quitting the chat session.\n", username);
            close(sockfd);
            exit(0);
        }else if(strcmp(args[0], "/createsession") == 0){ // create session
            if(check_user_state()){
                char* seshid = args[1];
                seshid[strcspn(seshid, "\n")] = '\0';
                stringify_and_send(NEW_SESS, username, seshid);
                clear_message();
            }
            pthread_create(&recv_thread, NULL, (void *)recv_handler, &sockfd);
        }else { // send actual message
            stringify(MESSAGE, MAX_DATA, username, msg, send_msg);
            if(send(sockfd,send_msg,strlen(send_msg), 0) < 0){
                fprintf(stdout, "Was not able to send %s's message", username);
            }
        }
    }

	pthread_join(recv_thread, NULL);
	close(sockfd);
	return 0;
}

/* note to self: change this before submitting */
void stringify_and_send(int type, char *source, char *data){
	pthread_mutex_lock(&lock);
    struct message msg_to_send;
	msg_to_send.type = type;
    bzero(msg_to_send.data, MAX_DATA);
    strcpy(msg_to_send.data,data);
    msg_to_send.size = MAX_DATA;
    bzero(msg_to_send.source, MAX_NAME);
    strcpy(msg_to_send.source,source);

    char message_string[MAX_DATA];
	bzero(message_string, MAX_DATA);

    int message_string_temp = sprintf(message_string, "%u:%u:%s:%s",msg_to_send.type, msg_to_send.size, msg_to_send.source, msg_to_send.data);

    if(write(sockfd,message_string,strlen(message_string)) < 0){
        fprintf(stderr, "big time error");
    }
    bzero(message_string, MAX_DATA);

	pthread_mutex_unlock(&lock);

}

/* note to self: change this before submitting */
void cmd_login(char **args){

    char *username, *password, *server_ip, *server_port;
	username = args[1];
    strcpy(username, username);
	password = args[2];
	server_ip = args[3];
	server_port = args[4];

    sockfd = socket(AF_INET, SOCK_STREAM,0);

    srvadr.sin_family = AF_INET;
    srvadr.sin_port = htons(atoi(server_port));
    srvadr.sin_addr.s_addr = inet_addr(server_ip);
    memset(srvadr.sin_zero, 0, sizeof(srvadr.sin_zero));

    int connection = connect(sockfd, (struct sockaddr*)&srvadr, (socklen_t)sizeof(srvadr));
    if (connection == -1){
        fprintf(stderr, "big time error");
        exit(1);
    }

	stringify_and_send(LOGIN, username, password);
    bzero(args, sizeof(args));
    clear_message();
}

/* note to self: change this before submitting */
void clear_message(){
    msg_to_send.type = -1;
    bzero(msg_to_send.data, MAX_DATA);
}

bool check_user_state(){
    if(!in_session){
        fprintf(stdout, "Log in first. \n");
        return false;
    }
    return true;
}

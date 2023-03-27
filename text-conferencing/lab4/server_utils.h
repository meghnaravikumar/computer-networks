#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#define NUM_USERS 3
#define NUM_SESSIONS 2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "message.h"


void out_msg(char *message) {
	fprintf(stdout, message);
}

typedef struct user {
	char username[32];
	char password[32];

	bool logged_in;
	bool joined;
	int sockfd;
	int threadid;
} user;

// fix syntax
struct user user_database[NUM_USERS] = {
	{"user1", "password1"}, {"user2", "password2"}, {"user3", "password3"}
};

typedef struct session {
	int session_id;
	bool init;
	int active_users;
	struct user users_array[NUM_USERS];
} session;

struct user logged_in_users[NUM_USERS] =  {
	{ "", "", false, false, -1, -1 }, { "", "", false, false, -1, -1 }, { "", "", false, false, -1, -1 }
};

struct session servers[NUM_SESSIONS] = {
	{ 1, false, 0, { { "", "", false, false, -1, -1 }, { "", "", false, false, -1, -1 }, { "", "", false, false, -1, -1 } } },
    { 2, false, 0, { { "", "", false, false, -1, -1 }, { "", "", false, false, -1, -1 }, { "", "", false, false, -1, -1 } } }
};

bool user_logged_in(char *username, char *password) {
	for (int i = 0; i < NUM_USERS; i++) {
		if ((strcmp(logged_in_users[i].username, username) == 0 && strcmp(logged_in_users[i].password, password) == 0) && (servers[session_id].users_array[j].logged_in==true)) {
            return true;    
        }
	}
	return false;
}

bool add_user(int session_id, char *username, char *password, int sockfd, int threadid) {
    if (servers[session_id].active_users >= NUM_USERS) {
        out_msg("Error: Maximum number of users reached in session.\n");
        return false;
    }
	if (!user_logged_in(username, password)) {
		out_msg("Error: user not logged in\n");
		return false;
	}
	for (int i = 0; i < NUM_USERS; i++) {
        if (!servers[session_id].users_array[i].joined) {
            strncpy(servers[session_id].users_array[i].username, username, 32);
            strncpy(servers[session_id].users_array[i].password, password, 32);
            servers[session_id].users_array[i].joined = true;
			servers[session_id].users_array[i].sockfd = sockfd;
			servers[session_id].users_array[i].threadid = threadid;
            servers[session_id].active_users++;
            printf("User %s added to session %d.\n", username, session_id);
            return true;
        }
    }
    printf("Error: Maximum number of users reached.\n");
	return false;
}

bool login_user(char *username, char *password, int sockfd, int threadid) {
	if (user_logged_in(username, password)) {
		out_msg("User already logged in\n");
		return true; // or false?
	}
	for (int i = 0; i < NUM_USERS; i++) {
        strncpy(logged_in_users[i].username, username, 32);
        strncpy(logged_in_users[i].password, password, 32);
		logged_in_users[i].logged_in = true;
        logged_in_users[i].joined = false;
		logged_in_users[i].sockfd = sockfd;
		logged_in_users[i].threadid = threadid;
        printf("User %s successfully logged in.\n", username);
        return true;
    }
	printf("User %s failed logged in.\n", username);
	return false;
}

bool remove_user(int session_id, char *username) {
    int index = -1;
    for (int i = 0; i < servers[session_id].active_users; i++) {
        if (strcmp(servers[session_id].users_array[i].username, username) == 0) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        printf("Error: User not found.\n");
        return false;
    }
    servers[session_id].users_array[index].joined = false;
    memset(servers[session_id].users_array[index].username, 0, sizeof(servers[session_id].users_array[index].username));
    memset(servers[session_id].users_array[index].password, 0, sizeof(servers[session_id].users_array[index].password));
    servers[session_id].users_array[index].sockfd = -1;
	servers[session_id].users_array[index].threadid = -1;
	fprintf("User %s removed.\n", username);
    servers[session_id].active_users--;
	return true;
}

bool valid_user(char *username, char* password) {
	for (int i = 0; i < NUM_USERS; i++) {
		if(strcmp(user_database[i].username, username) == 0 && strcmp(user_database[i].password, password) == 0) {
            return true;    
        }
    }
	return false;
}

bool in_session(int session_id, char *username) {
	for (int j = 0; j < NUM_USERS; j++) {
		if ((strcmp(servers[session_id].users_array[j].username, username) == 0 && strcmp(servers[session_id].users_array[j].password, password) == 0) && (servers[session_id].users_array[j].joined==true)) {
            return true;    
        }
	}
	return false;
}

bool active_session(int session_id) {
	if (servers[session_id].init == true) {return true;}
	else {return false;}
}

void init_session(int session_id) {
	servers[session_id].session_id = session_id;
	servers[session_id].init = true;
	servers[session_id].active_users = 0;
}

bool join_session(int session_id, char *username, char *password, int sockfd, int threadid) {
	if (!active_session(session_id)) {return false;}
	if (!valid_user(username, password)) {return false;}
	return add_user(session_id, username, password, sockfd, threadid);
}

bool leave_session(int session_id, char *username) {
	if (!active_session(session_id)) {return false;}
	if (!valid_user(username, password)) {return false;}
	return remove_user(session_id, username);
}

void reset_session(int session_id) {
    // Reset active users count
    servers[session_id].active_users = 0;

    // Reset user array for the session
    for (int i = 0; i < NUM_USERS; i++) {
        memset(servers[session_id].users_array[i].username, 0, sizeof(servers[session_id].users_array[i].username));
        memset(servers[session_id].users_array[i].password, 0, sizeof(servers[session_id].users_array[i].password));
        servers[session_id].users_array[i].logged_in = false;
		servers[session_id].users_array[i].joined = false;
        servers[session_id].users_array[i].sockfd = -1;
        servers[session_id].users_array[i].threadid = -1;
    }

    printf("Session %d has been reset.\n", session_id);
}

struct message login_cmd(struct message msg) {
	bool success;
	unsigned int type;
	unsigned char data[MAX_DATA];
	if (!valid_user(msg.source, msg.data)) {
		out_msg("Invalid user\n");
		type = LO_NAK;
		// do we need to specify the type of error message
	}
	else {
		out_msg("Valid user\n");
		type = LO_ACK;
		// use locking here
		success = login_user(msg.source, msg.data, -1, -1);
	}
	// fix output message
}

struct message join_cmd(struct message msg) {
	bool success;
	int session_id = atoi((char*)(msg.data));
	unsigned int type;
	unsigned char data[MAX_DATA];
	if (!active_session(session_id) || in_session(session_id, msg.source)) {
		out_msg("invalid join request\n");
		type = LO_NAK;
	}
	else {
		out_msg("valid join request\n");
		type = JN_ACK;
		sprintf((char *)(data), "%d", session_id);

		//use locking here
		success = join_session(session_id, msg.source, msg.data, -1, -1);
	}
	//fix output message
}

struct message leave_cmd(struct message msg) {
	bool success;
	unsigned int type;
	unsigned char data[MAX_DATA];
	for (int i = 0; i < NUM_SESSIONS; i++) {
		if (in_session(i, msg.source)) {
			//use locking here
			success = leave_session(i, msg.source);
		}
	}
	//fix output message
}

struct message new_cmd(struct message msg) {
	bool success;
	unsigned int type;
	unsigned char data[MAX_DATA];
	int count;
	for (int i = 0; i < NUM_SESSIONS; i++) {
		if (servers[i].init == false) {
			init_session(i);
			type = NS_ACK;
			sprintf((char *)(data), "%d", i);
			count++;
		}
	}
	if (!count) {out_msg("error, max sessions reached\n");}
}

struct message message_cmd(struct message msg) {
	bool success;
	unsigned int type;
	char data[MAX_DATA];
}

struct message query_cmd(struct message msg) {
	bool success;
	unsigned int type;
	char data[MAX_DATA];
}



void client_handler(int session_id, char *username, char *password, int sockfd, int threadid) {
	struct message msg;
	struct message msg_send;
	if (msg.type == LOGIN) {msg_send = login_cmd(msg);}
	else if (msg.type == JOIN) {msg_send = join_cmd(msg);}
	else if (msg.type == LEAVE_SESS) {msg_send = leave_cmd(msg);}
	else if (msg.type == NEW_SESS) {msg_send = new_cmd(msg);}
	else if (msg.type == MESSAGE) {msg_send = message_cmd(msg);}
	else if (msg.type == QUERY) {msg_send = query_cmd(msg);}
}

int server_cmd(char *cmd)
{
	char *args[2];
	int nargs=0;
	char *word;
	int i, result;

    // get rid of newline character after the command
    // source: https://codereview.stackexchange.com/questions/67608/trimming-fgets-strlen-or-strtok
    size_t len = strlen(cmd);
    if (len && (cmd[len-1] == '\n')) {
        cmd[len-1] = '\0';
    }

	for (word = strtok(cmd, " ");
	     word != NULL;
	     word = strtok(NULL, " ")) {
		args[nargs++] = word;
	}

	if (nargs != 2) {
		fprintf(stdout, "Incorrect number of arguments.\n");
        return -1;
	}

	if(strcmp(args[0], "server") != 0) return -1;

    return atoi(args[1]);
}



#endif

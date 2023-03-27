#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include "message.h"
#include <pthread.h>

#define MAXMENUARGS 10
#define MAXMENUCMDS 8

struct session_details {
	bool active;
	int sockfd;
	pthread_t threadid;
};

struct session_details session;


void error_msg(char *message){
    fprintf(stderr, message);
    exit(1);
}

void out_msg(char *message) {
	fprintf(stdout, message);
}

/**
 * @brief Log into the server at the given address and port.
 * args[0]:
*/
int cmd_login(int nargs, char **args){
    if(nargs != 5) return -1;
    out_msg("cmd_login\n");

	// cmd: client_id, password, server_ip, server_port
	char *client_id, *password, *server_ip, *server_port;
	client_id = args[1];
	password = args[2];
	server_ip = args[3];
	server_port = args[4];

	// error checking

	// TCP connection

	// from Beej's guide
	int rv;
	int yes=1;
	char remoteIP[INET6_ADDRSTRLEN];
	struct addrinfo hints, *ai, *p;

	// get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(server_ip, server_port, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1); //exit or return?
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        if (session.sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol) < 0) {
			error_msg("client socket error\n");
			continue;
		}

        // lose the pesky "address already in use" error message - IS THIS NECESSARY
        setsockopt(session.sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (connect(session.sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(session.sockfd);
			session.sockfd = -1;
			error_msg("client connect error\n");
            continue;
        }
        break;
	}

	// if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
	}

	freeaddrinfo(ai); // all done with this

	// create message and convert to string for sending
    encode_message(create_message(LOGIN, strlen(password), client_id, password), buf);

	// send and error check
	if ((num_bytes = send(session.sockfd, buf, BUFSIZ - 1, 0)) < 0) {
		error_msg("client send error\n");
		close(session.sockfd);
		session.sockfd = -1;
		return -1;
	}

	// HANDLE THREAD MESSAGING FOR LO_ACK and LO_NACK




}

/**
 * @brief Log out of the server, but do not exit the client.
 * The client should return to the same state as when you
 * started running it.
 *
*/
int cmd_logout(int nargs, char **args){
    if(nargs != 1) return -1;
    out_msg("cmd_logout\n");

	// check session status
	if (!session.active) {return -1;}

	// create message and convert to string for sending
    encode_message(create_message(EXIT, 0, NULL, NULL), buf);

	// send and error check
	if ((num_bytes = send(session.sockfd, buf, BUFSIZ - 1, 0)) < 0) {
		error_msg("client send error\n");
		return -1;
	}

	// we are logging out so we send cancellation request to thread (success: 0, error is nonzero)
	if (pthread_cancel(session.threadid) == 0) {out_msg("logged out successfully\n");}
	else {error_msg("logout failure\n");}

	// toggle session status
	session.active = false;
	close(session.sockfd);
	session.sockfd = -1;

}

/**
 * @brief Join the conference session with the given session ID.
 * args[0]: sessionid
*/
int cmd_joinsession(int nargs, char **args){
    if(nargs != 2) return -1;
    fprintf(stdout, "cmd_joinsession\n");

	// check session status
	if (session.active) {return -1;}

	// if session id is valid
	if (args[0] != NULL) {
		// create message and convert to string for sending
		encode_message(create_message(JOIN, strlen(args[0]), NULL, args[0]), buf);

		// send and error check
		if ((num_bytes = send(session.sockfd, buf, BUFSIZ - 1, 0)) < 0) {
			error_msg("client send error\n");
			return -1;
		}
	}

	// HANDLE THREAD MESSAGING for JN_ACK and JN_NACK

	// toggle session status
	session.active = true;
}

/**
 * @brief Leave the currently established session.
 * args
*/
int cmd_leavesession(int nargs, char **args){
    if(nargs != 1) return -1;
    fprintf(stdout, "cmd_leavesession\n");

	// check session status
	if (!session.active) {return -1;}

	// create message and convert to string for sending
    encode_message(create_message(LEAVE_SESS, 0, NULL, NULL), buf);

	// send and error check
	if ((num_bytes = send(session.sockfd, buf, BUFSIZ - 1, 0)) < 0) {
		error_msg("client send error\n");
		return -1;
	}

	// toggle session status
	session.active = false;
}

/**
 * @brief Create a new conference session and join it.
 * args
*/
int cmd_createsession(int nargs, char **args){
    if(nargs != 2) return -1; // what is the second argument? shouldn't it be 1?
    fprintf(stdout, "cmd_createsession\n");

	// check session status
	if (session.active) {return -1;}

	// create message and convert to string for sending
	encode_message(create_message(NEW_SESS, 0, NULL, NULL), buf);

	// send and error check
	if ((num_bytes = send(session.sockfd, buf, BUFSIZ - 1, 0)) < 0) {
		error_msg("client send error\n");
		return -1;
	}

	// toggle session status
	session.active = true;
}

/**
 * @brief Get the list of the connected clients and available sessions.
*/
int cmd_list(int nargs, char **args){
    if(nargs != 1) return -1;
    fprintf(stdout, "cmd_list\n");

	// create message and convert to string for sending
	encode_message(create_message(QUERY, 0, NULL, NULL), buf);

	// send and error check
	if ((num_bytes = send(session.sockfd, buf, BUFSIZ - 1, 0)) < 0) {
		error_msg("client send error\n");
		return -1;
	}

}

/**
 * @brief Terminate the program.
*/
int cmd_quit(int nargs, char **args){
    if(nargs != 1) return -1;
    fprintf(stdout, "cmd_quit\n");
}

/**
 * @brief Send a message to the current conference session.
 * The message is sent after the newline.
*/
int cmd_text(int nargs, char **args){
    // we'll deal with this one last
    fprintf(stdout, "cmd_text\n");

	if (!session.sockfd) {return -1;}

	// create message and convert to string for sending - is the way i used strlen(buf) correct?
	encode_message(create_message(MESSAGE, strlen(buf), NULL, buf), buf);

	// send and error check
	if ((num_bytes = send(session.sockfd, buf, BUFSIZ - 1, 0)) < 0) {
		error_msg("client send error\n");
		return -1;
	}
}


static const char *opsmenu[] = {
	"1. /login <client ID> <password> <server-IP> <server-port>",
	"2. /logout",
	"3. /joinsession <session ID>",
	"4. /leavesession",
	"5. /createsession <session ID>",
	"6. /list",
	"7. /quit",
	"8. <text>",
	NULL
};

/**
 * Command table for control packet type.
 */
static struct {
	const char *name;
	int (*func)(int nargs, char **args);
} cmdtable[] = {
	{ "/login", cmd_login},
	{ "/logout", cmd_logout},
	{ "/joinsession", cmd_joinsession},
	{ "/leavesession", cmd_leavesession},
	{ "/createsession", cmd_createsession},
	{ "/list", cmd_list},
	{ "/quit", cmd_quit},
	{ "<text>",	cmd_text},
    { NULL, NULL }
};

/**
 * @param name: "Text conferencing menu:"
 * @param x: cmdtable or opsmenu?
*/
static void showmenu(const char *name, const char *x[])
{
	int ct, half, i;

	fprintf(stdout, "\n");
	fprintf(stdout, "%s\n", name);

	for (i=ct=0; x[i]; i++) {
		ct++;
	}
	half = (ct+1)/2;

	for (i=0; i<half; i++) {
		fprintf(stdout, "    %-65s", x[i]);
		if (i+half < ct) {
			fprintf(stdout, "%s", x[i+half]);
		}
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "\n");
}

/*
 * Process a single command.
 */
static int cmd_dispatch(char *cmd)
{
	char *args[MAXMENUARGS];
	int nargs=0;
	char *word;
	int i, result, cmd_num = MAXMENUCMDS;

    // get rid of newline character after the command
    // source: https://codereview.stackexchange.com/questions/67608/trimming-fgets-strlen-or-strtok
    size_t len = strlen(cmd);
    if (len && (cmd[len-1] == '\n')) {
        cmd[len-1] = '\0';
    }

	for (word = strtok(cmd, " ");
	     word != NULL;
	     word = strtok(NULL, " ")) {

		if (nargs >= MAXMENUARGS) {
			fprintf(stdout, "Command line has too many words\n");
			return -1;
		}
		args[nargs++] = word;
	}

	if (nargs==0) {
		return -1;
	}

	for (i=0; i < MAXMENUCMDS; i++) {
		if (strcmp(args[0], cmdtable[i].name) == 0) {
            cmd_num = i;
		}
	}

    result = cmdtable[cmd_num].func(nargs, args);

    return result;
}


static void menu_execute(char *line, int isargs)
{
	char *command;
	char *context;
	int result;

	for (command = strtok_r(line, ";", &context);
	     command != NULL;
	     command = strtok_r(NULL, ";", &context)) {

		if (isargs) {
			fprintf(stdout, "Text conferencing command: %s", command);
		}

		result = cmd_dispatch(command);
		if (result == -1) {
			fprintf(stdout, "Text conferencing command failed.\n");
		}
	}
}

int client_init(char *cmd)
{
	char *args[1];
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

	if (nargs != 1) {
		fprintf(stdout, "Incorrect number of arguments.\n");
        return -1;
	}

	if(strcmp(args[0], "client") != 0) return -1;

    return 1;
}

#endif

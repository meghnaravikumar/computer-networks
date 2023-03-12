#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include "message.h"

#define MAXMENUARGS 10
#define MAXMENUCMDS 8


/**
 * @brief Log into the server at the given address and port.
*/
int cmd_login(int nargs, char **args){
    if(nargs != 5) return -1;
    fprintf(stdout, "cmd_login\n");
}

/**
 * @brief Log out of the server, but do not exit the client.
 * The client should return to the same state as when you
 * started running it.
*/
int cmd_logout(int nargs, char **args){
    if(nargs != 1) return -1;
    fprintf(stdout, "cmd_logout\n");
}

/**
 * @brief Join the conference session with the given session ID.
*/
int cmd_joinsession(int nargs, char **args){
    if(nargs != 2) return -1;
    fprintf(stdout, "cmd_joinsession\n");
}

/**
 * @brief Leave the currently established session.
*/
int cmd_leavesession(int nargs, char **args){
    if(nargs != 1) return -1;
    fprintf(stdout, "cmd_leavesession\n");
}

/**
 * @brief Create a new conference session and join it.
*/
int cmd_createsession(int nargs, char **args){
    if(nargs != 2) return -1;
    fprintf(stdout, "cmd_createsession\n");
}

/**
 * @brief Get the list of the connected clients and available sessions.
*/
int cmd_list(int nargs, char **args){
    if(nargs != 1) return -1;
    fprintf(stdout, "cmd_list\n");
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
			return 0;
		}
		args[nargs++] = word;
	}

	if (nargs==0) {
		return 0;
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
		if (result) {
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

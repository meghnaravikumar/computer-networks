#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H


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

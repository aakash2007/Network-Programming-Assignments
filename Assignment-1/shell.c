#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#define EXIT_SUCCESS 0
#ifndef EXIT_FAILURE
#define EXIT_FAILURE -1
#endif
#define BUFFER_SIZE 64
#define SHELL_TOK_DELIM " \t\r\n\a"

typedef struct command {
	char *cmd;
	char *args[BUFFER_SIZE];
	int infd, outfd;
} Command;

char *shell_read_line(void) {
	char *line = NULL;
	ssize_t bufsize = 0; // have getline allocate a buffer for us
	getline(&line, &bufsize, stdin);
	return line;
}

char **shell_split_line(char *line) {
	int bufsize = BUFFER_SIZE, position = 0;
	char **tokens = (char**)malloc(bufsize * sizeof(char*));
	int length = strlen(line);
	if (!tokens) {
		fprintf(stderr, "shell: allocation error\n");
		exit(EXIT_FAILURE);
	}
	int pos = 0, token_no = 0;
	for(int i = 0; i < length; i++) {
		if(line[i] == ' ' || line[i] == '\t' || line[i] == '\n' || line[i] == '\r') {
			if(pos != 0)	token_no++;
			continue;
		}
		if(tokens[token_no] == NULL) {
			pos = 0;
			tokens[token_no] = (char*)malloc(bufsize * sizeof(char));
		}
		if(line[i] == '|' || line[i] == '<' || line[i] == '>') {
			if(pos > 0)	{
				pos = 0;
				tokens[++token_no] = (char*)malloc(bufsize * sizeof(char));
			}
			if(i+2 < length && line[i] == '|' && line[i+1] == '|' && line[i+2] == '|')
				strcpy(tokens[token_no++], "|||"), i += 2;
			else if(i+1 < length && line[i] == '|' && line[i+1] == '|')
				strcpy(tokens[token_no++], "||"), i++;
			else if(line[i] == '|')
				strcpy(tokens[token_no++], "|");
			else if(i+1 < length && line[i] == '>' && line[i+1] == '>')
				strcpy(tokens[token_no++], ">>"), i++;
			else if(line[i] == '<')
				strcpy(tokens[token_no++], "<");
			else if(line[i] == '>')
				strcpy(tokens[token_no++], ">");
			continue;
		}
		if(line[i] == ',') {
			if(pos > 0)	{
				pos = 0;
				tokens[++token_no] = (char*)malloc(bufsize * sizeof(char));
			}
			strcpy(tokens[token_no++], ",");
			continue;
		}
		if(line[i] == ';') {
			if(pos > 0)	{
				pos = 0;
				tokens[++token_no] = (char*)malloc(bufsize * sizeof(char));
			}
			strcpy(tokens[token_no++], ";");
			continue;
		}
		tokens[token_no][pos++] = line[i];
	}
	return tokens;
}

Command* shell_parse(char **args) {
	Command *cmds = (Command*)malloc(sizeof(Command)*BUFFER_SIZE);
	int cmd_no = 0, arg_no = 0, pos = 0;
	int fd[2]; pipe(fd);
	while(args[pos] != NULL) {
		cmds[cmd_no].cmd = args[pos];
		while(args[pos] != NULL && args[pos][0] != '|' && args[pos][0] != '<' && args[pos][0] != '>' && args[pos][0] != ',' && args[pos][0] != ';')
			cmds[cmd_no].args[arg_no++] = args[pos++];
		if(args[pos] != NULL)
			cmds[++cmd_no].cmd = args[pos++];
		cmd_no++;
		arg_no = 0;
	}
	return cmds;
}

int shell_execute(Command *cmds) {
	if(cmds == NULL)	return 1;
	for(int i = 0; i < BUFFER_SIZE && cmds[i].cmd != NULL; i++) {
		int pid = fork();
		if(pid == 0) {
			printf("%s\n", cmds[i].cmd);
			if(execvp(cmds[i].cmd, cmds[i].args) == -1) {
				printf("Shell: command not found: %s\n", cmds[i].cmd);
				exit(EXIT_FAILURE);
			}
		}
		else
			wait(NULL);
	}
	return 1;
}

void shell_loop(void) {
	char *line;
	char **args;
	Command *cmds;
	int status;
	do {
		printf("Shell> ");
		line = shell_read_line();
		args = shell_split_line(line);
		cmds = shell_parse(args);
		status = shell_execute(cmds);
	} while (status);
}

int main(int argc, char **argv) {
  shell_loop();
  return EXIT_SUCCESS;
}
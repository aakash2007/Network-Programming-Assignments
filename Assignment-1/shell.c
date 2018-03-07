#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#define EXIT_SUCCESS 0
#define BUFFER_SIZE 128
#define SHELL_TOK_DELIM " \t\r\n\a"

typedef struct command {
	char *cmd;
	char *args[BUFFER_SIZE];
	int read[2], write[2];
} Command;

char *shell_read_line() {
	int bufsize = BUFFER_SIZE;
	char *line = (char*)malloc(sizeof(char)*bufsize);
	int i = 0;
	char ch;
	while((ch = getchar()) != 10) {
		if(i == bufsize) {
			bufsize *= 2;
			line = realloc(line, bufsize);
		}
		line[i++] = ch;
		if(ch == -1)	exit(EXIT_SUCCESS);
	}
	return line;
}

char **shell_split_line(char *line) {
	int bufsize = BUFFER_SIZE, position = 0;
	char **tokens = (char**)malloc(bufsize * sizeof(char*));
	int length = strlen(line);
	if (!tokens) {
		fprintf(stderr, "Shell: allocation error\n");
		exit(EXIT_FAILURE);
	}
	int pos = 0, token_no = 0;
	for(int i = 0; i < length; i++) {
		if(token_no+5 >= bufsize) {
			bufsize *= 2;
			tokens = realloc(tokens, bufsize*sizeof(char*));
		}
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

Command* shell_parse(char **args, int *isPipeline) {
	int bufsize = BUFFER_SIZE;
	Command *cmds = (Command*)malloc(sizeof(Command)*bufsize);
	int cmd_no = 0, arg_no = 0, pos = 0;
	int isp = 0, isr = 0;
	while(args[pos] != NULL) {
		if(cmd_no + 5 >= bufsize) {
			bufsize *= 2;
			cmds = realloc(cmds, sizeof(Command)*bufsize);
		}
		cmds[cmd_no].cmd = args[pos];
		cmds[cmd_no].read[0] = -1;
		cmds[cmd_no].read[1] = -1;
		cmds[cmd_no].write[0] = -1;
		cmds[cmd_no].write[1] = -1;
		while(args[pos] != NULL && args[pos][0] != '|' && args[pos][0] != '<' && args[pos][0] != '>' && args[pos][0] != ',' && args[pos][0] != ';')
			cmds[cmd_no].args[arg_no++] = args[pos++];
		if(args[pos] != NULL) {
			if(args[pos][0] == '|')	isp = 1;
			if(args[pos][0] == '<' || args[pos][0] == '>')	isr = 1;
			cmds[++cmd_no].cmd = args[pos++];
		}
		cmd_no++;
		arg_no = 0;
	}
	if(pos == 0)	return NULL;
	if(isp == 1 && isr == 1) {
		fprintf(stderr, "Shell: Cannot use pipe and redirection at same time.\n");
		return NULL;
	}
	else if(isp == 1)	*isPipeline = 1;
	else if(isr == 1)	*isPipeline = 0;
	else	*isPipeline = 0;
	return cmds;
}


int shell_execute_pipeline(Command *cmds) {
	if(cmds == NULL)	return 1;
	int pid;
	int in = dup(STDIN_FILENO);
	int out = dup(STDOUT_FILENO);
	cmds[0].read[0] = STDIN_FILENO;
	int i;
	for(i = 0; i < BUFFER_SIZE && cmds[i].cmd != NULL; i++) {
		if(strcmp(cmds[i].cmd, "|") == 0) {
			if(i == 0 || cmds[i+1].cmd == NULL) {
				fprintf(stderr, "Shell: Cannot parse after %s\n", cmds[i].cmd);
				break;
			}
			pipe(cmds[i+1].read);
			char *buffer[BUFFER_SIZE];
			int size = 0;
			while((size = read(cmds[i].read[0], buffer, BUFFER_SIZE)) > 0)
				write(cmds[i+1].read[1], buffer, size);

			close(cmds[i].read[0]);
			close(cmds[i+1].read[1]);
			continue;
		}
		else if(strcmp(cmds[i].cmd, "||") == 0) {
			if(i == 0 || cmds[i+1].cmd == NULL || cmds[i+2].cmd == NULL || cmds[i+3].cmd == NULL 
					||  strcmp(cmds[i+2].cmd, ",") != 0) {
				fprintf(stderr, "Shell: Cannot parse after %s\n", cmds[i].cmd);
				break;
			}
			pipe(cmds[i+1].read);
			pipe(cmds[i+3].read);
			char *buffer[BUFFER_SIZE];
			int size = 0;
			while((size = read(cmds[i].read[0], buffer, BUFFER_SIZE)) > 0) {
				write(cmds[i+1].read[1], buffer, size);
				write(cmds[i+3].read[1], buffer, size);				
			}

			close(cmds[i].read[0]);
			close(cmds[i+1].read[1]);
			close(cmds[i+3].read[1]);
			cmds[i+1].write[1] = out;
			cmds[i+3].write[1] = out;
			continue;
		}
		else if(strcmp(cmds[i].cmd, "|||") == 0) {
			if(i == 0 || cmds[i+1].cmd == NULL || cmds[i+2].cmd == NULL || cmds[i+3].cmd == NULL 
					|| cmds[i+4].cmd == NULL || cmds[i+5].cmd == NULL
					|| strcmp(cmds[i+2].cmd, ",") != 0  || strcmp(cmds[i+4].cmd, ",") != 0) {
				fprintf(stderr, "Shell: Cannot parse after %s\n", cmds[i].cmd);
				break;
			}
			pipe(cmds[i+1].read);
			pipe(cmds[i+3].read);
			pipe(cmds[i+5].read);
			char *buffer[BUFFER_SIZE];
			int size = 0;
			while((size = read(cmds[i].read[0], buffer, BUFFER_SIZE)) > 0) {
				write(cmds[i+1].read[1], buffer, size);
				write(cmds[i+3].read[1], buffer, size);
				write(cmds[i+5].read[1], buffer, size);
			}

			close(cmds[i].read[0]);
			close(cmds[i+1].read[1]);
			close(cmds[i+3].read[1]);
			close(cmds[i+5].read[1]);

			cmds[i+1].write[1] = out;
			cmds[i+3].write[1] = out;
			cmds[i+5].write[1] = out;
			continue;
		}
		else if(strcmp(cmds[i].cmd, ",") == 0)	continue;
		else if(strcmp(cmds[i].cmd, ";") == 0) {
			char *buffer[BUFFER_SIZE];
			int size = 0;
			while((size = read(cmds[i].read[0], buffer, BUFFER_SIZE)) > 0)
				write(out, buffer, size);
			break;
		}

		if(cmds[i].write[1] == -1)	pipe(cmds[i].write);
		pid = fork();
		if(pid == 0) {
			dup2(cmds[i].read[0], STDIN_FILENO);
			dup2(cmds[i].write[1], STDOUT_FILENO);
			if(execvp(cmds[i].cmd, cmds[i].args) == -1) {
				fprintf(stderr, "Shell: Command not found: %s\n", cmds[i].cmd);
				exit(EXIT_FAILURE);
			}
		}
		else {
			wait(NULL);
			if(cmds[i+1].cmd == NULL)
				cmds[i+1].read[1] = out;
			else	pipe(cmds[i+1].read);

			close(cmds[i].read[0]);
			close(cmds[i].write[1]);

			char *buffer[BUFFER_SIZE];
			int size = 0;
			while((size = read(cmds[i].write[0], buffer, BUFFER_SIZE)) > 0)
				write(cmds[i+1].read[1], buffer, size);

			close(cmds[i].write[0]);
			close(cmds[i+1].read[1]);
		}
	}
	dup2(in, STDIN_FILENO);
	dup2(out, STDOUT_FILENO);
	return 1;
}

int shell_execute_redirected(Command *cmds) {
	if(cmds == NULL)	return 1;
	int in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);
	int input_fd = STDIN_FILENO, output_fd = STDOUT_FILENO;
	for(int i = 0; i < BUFFER_SIZE && cmds[i].cmd != NULL; i++) {
		if(strcmp(cmds[i].cmd, "<") == 0) {
			if(cmds[i+1].cmd == NULL) {
				fprintf(stderr, "Shell: Cannot Parse After '%s'\n", cmds[i].cmd);
				return 1;
			}
			input_fd = open(cmds[++i].cmd, O_RDONLY);
			if(input_fd == -1) {
				printf("Shell: No such file or directory: %s\n", cmds[i].cmd);
				return 1;
			}
		}
		else if(strcmp(cmds[i].cmd, ">") == 0) {
			if(cmds[i+1].cmd == NULL) {
				fprintf(stderr, "Shell: Cannot Parse After '%s'\n", cmds[i].cmd);
				return 1;
			}
			output_fd = fileno(fopen(cmds[++i].cmd, "w"));
		}
		else if(strcmp(cmds[i].cmd, ">>") == 0) {
			if(cmds[i+1].cmd == NULL) {
				fprintf(stderr, "Shell: Cannot Parse After '%s'\n", cmds[i].cmd);
				return 1;
			}
			output_fd = fileno(fopen(cmds[++i].cmd, "a"));
		}
	}
	int pid = fork();
	if(pid == 0) {
		dup2(input_fd, STDIN_FILENO);
		dup2(output_fd, STDOUT_FILENO);
		if(execvp(cmds[0].cmd, cmds[0].args) == -1) {
			fprintf(stderr, "Shell: Command not found: %s\n", cmds[0].cmd);
			exit(EXIT_FAILURE);
		}
	}
	else {
		wait(NULL);
		dup2(in, STDIN_FILENO);
		dup2(out, STDOUT_FILENO);
	}
}

void shell_loop() {
	char *line;
	char **args;
	Command *cmds;
	int status, isPipeline = 0;
	do {
		printf("Shell>");
		line = shell_read_line();
		args = shell_split_line(line);
		cmds = shell_parse(args, &isPipeline);
		if(isPipeline)	status = shell_execute_pipeline(cmds);
		else	status = shell_execute_redirected(cmds);
	} while (status);
}

void interruptHandler(int signo) {
	sleep(0.01);
	printf("\n\rShell>");
	fflush(stdout);
	return;
}

int main(int argc, char **argv) {
	signal(SIGINT, interruptHandler);
	shell_loop();
	return EXIT_SUCCESS;
}
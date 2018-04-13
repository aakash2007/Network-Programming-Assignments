#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>

#define BUFSIZE 1024

int main(int argc, char const *argv[])
{
	if(argc != 2){
		printf("Usage: ./server <portno> \n");
		exit(1);
	}

	int portno = atoi(argv[1]);

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portno);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int lis_fd;
	if((lis_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("Socket");
		exit(1);
	}

	if((bind(lis_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
		perror("Bind");
		exit(1);
	}
	int wait_size = 15;

	if((listen(lis_fd, wait_size)) < 0){
		perror("Listen");
	}

	struct sockaddr_in cli_addr;
	int cli_addr_len = 0;
	int conn_fd;

	printf("Server Ready to Accept Connections.....\n");

	pid_t child_pid;

	for(;;){
		if((conn_fd = accept(lis_fd, (struct sockaddr *)&cli_addr, &cli_addr_len)) < 0){
			perror("Accept");
			exit(1);
		}

		if((child_pid = fork()) == 0){
			close(lis_fd);
			break;
		}
		else{
			close(conn_fd);
			int status;
			waitpid(child_pid, &status, WNOHANG);
		}
	}

	char buf[BUFSIZE];
	if(child_pid == 0){

		while(1){
			bzero(&buf, BUFSIZE);
			int n = recv(conn_fd, buf, BUFSIZE,0);

			if(n < 0){
				perror("Read");
				exit(1);
			}
			if(n == 0){
				// printf("Client done\n");
				close(conn_fd);
				exit(0);
			}

			for (int i = 0; i < n; ++i)
			{
				buf[i] = toupper(buf[i]);
			}
			send(conn_fd, buf, n, 0);
		}
		close(conn_fd);
		exit(0);
	}
	else
		wait(NULL);
	return 0;
}
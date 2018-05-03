#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <unistd.h>

int main(int argc, char **argv){
	if (argc < 3) {
		fprintf(stderr, "Usage: ./server <server ip> <port>\n");
		exit(1);
	}
	const int server_port = ;

	char server_name[15];
	strcpy(server_name, "127.0.0.1");
	// printf("Enter Server IP Address: ");
	// scanf("%s", server_name);
	
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;

	int sock;
	
	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		exit(1);
	}

	inet_pton(AF_INET, server_name, &server_addr.sin_addr);
	server_addr.sin_port = htons(server_port);

	if((connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
		perror("connect");
		exit(1);
	}

	wait(NULL);
	close(sock);
	return 0;
}
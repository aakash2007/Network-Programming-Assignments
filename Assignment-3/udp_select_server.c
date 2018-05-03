#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAXLEN 4096

char* res(char* inp){
	char* snum = strtok(inp, ";");
	int sum = 0;
	if(snum != NULL){
		sum = atoi(snum);
		while(snum != NULL){
			snum = strtok(NULL, ";");
			if(snum != NULL){
				sum += atoi(snum);
			}
		}
	}
	char* ans = malloc(256*sizeof(char));
	sprintf(ans, "%d", sum);
	return ans;
}

int main(int argc, char **argv){
	fd_set rset, allset;
	int sockfd;
	struct sockaddr_in srv_addr, cli_addr;
	socklen_t clilen = sizeof(cli_addr);

	if (argc != 2) {
		fprintf(stderr, "Usage: ./server %s <port>\n", argv[1]);
		exit(1);
	}
	int portno = atoi(argv[1]);
	if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
		perror("socket");
		exit(1);
	}

	memset(&srv_addr, 0, sizeof(srv_addr));
	
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(portno);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     
	if(bind(sockfd, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0){
		perror("binding");
		close(sockfd);
		exit(1);
	}

	int n;
	printf("Server Ready....\n");
	while(1){

		char buffer[500];
				
		FD_ZERO(&rset);
		FD_SET(sockfd, &rset);

		if(select(sockfd+1, &rset, NULL, NULL, NULL) > 0){
			if(FD_ISSET(sockfd, &rset)){
				int len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &clilen);
				buffer[len] = '\0';
				printf("Received from: %s\n", inet_ntoa(cli_addr.sin_addr));
				char* ans = res(buffer);
				sendto(sockfd, ans, sizeof(ans), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
			}
		}
	}
}
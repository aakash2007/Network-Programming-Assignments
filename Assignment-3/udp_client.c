#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char **argv) {
	const char* server_name = "localhost";


	if (argc < 3) {
		fprintf(stderr, "Usage: ./server <server ip> <port>\n");
		exit(1);
	}
	const int server_port = atoi(argv[2]);

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;

	inet_pton(AF_INET, argv[1], &server_address.sin_addr);

	server_address.sin_port = htons(server_port);

	int sockfd;
	if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("could not create socket\n");
		return 1;
	}

	char data_to_send[4096];
	strcpy(data_to_send, "");
	srand(time(0));
	int k = rand()%101 + 1;
	printf("Generated Numbers: %d\n", k);

	for (int i = 0; i < k; ++i)
	{
		int num = rand()%1001;
		char snum[20];
		sprintf(snum, "%d", num);
		strcat(data_to_send, snum);
		strcat(data_to_send, ";");
	}

	printf("To Server: %s\n", data_to_send);

	int len = sendto(sockfd, data_to_send, strlen(data_to_send), 0, (struct sockaddr*)&server_address, sizeof(server_address));

	char buffer[100];
	recvfrom(sockfd, buffer, len, 0, NULL, NULL);
	buffer[len] = '\0';
	printf("\nSum: %s\n", buffer);

	close(sockfd);
	return 0;
}
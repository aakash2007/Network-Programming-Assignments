#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

char username[20];

typedef struct my_msg		// For Sending Over Network
{
	char msg_from[20];
	char msg_to[20];
	char msg_text[200];
} MESSAGE;

char* encode_msg(MESSAGE out_msg){
	char* en_msg = (char*)malloc(250*sizeof(char));
	strcpy(en_msg, username);
	strcat(en_msg, ",");
	strcat(en_msg, out_msg.msg_to);
	strcat(en_msg, ",");
	strcat(en_msg, out_msg.msg_text);

	return en_msg;
}

int main(){

	const char* server_name = "127.0.0.1";
	// const char* server_name = "192.168.43.55";
	// char server_name[20];
	const int server_port = 6789;

	// printf("Enter Server IP Address: ");
	// scanf("%s", server_name);

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;

	inet_pton(AF_INET, server_name, &server_addr.sin_addr);
	server_addr.sin_port = htons(server_port);

	int sock;

	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		exit(1);
	}

	if((connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
		perror("connect");
		exit(1);
	}

	int server_auth = 0;
	char data_to_send[100];
	char* password;

	// do{
		printf("Enter Username: ");
		scanf("%s", username);
		password = getpass("Enter Password: ");


		// Generating Verification String
		strcpy(data_to_send, username);
		strcat(data_to_send, ",");
		strcat(data_to_send, password);

		printf("%s 	%ld\n", data_to_send, strlen(data_to_send));

		// write(sock, data_to_send, strlen(data_to_send+1))
		send(sock, data_to_send, 100, 0);
	// }while(server_auth != 1);


	int maxlen = 100;
	char buffer[maxlen];
	char *pbuffer = buffer;

	recv(sock, pbuffer, maxlen, 0);

	printf("Response from Server: %s\n", buffer);

	char buffer2[maxlen];
	char *pbuffer2 = buffer2;

	recv(sock, pbuffer2, maxlen, 0);

	printf("Response from Server: %s  %ld\n", buffer2, strlen(buffer2));

	close(sock);
	return 0;
}
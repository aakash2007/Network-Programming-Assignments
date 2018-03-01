#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

char username[10];

struct my_msg		// For Sending Over Network
{
	// char msg_from[20];
	char msg_to[20];
	char msg_text[200];
};
typedef struct my_msg MESSAGE;

char* encode_msg(MESSAGE out_msg){
	char* en_msg = (char*)malloc(20*sizeof(char));
	strcpy(en_msg, username);
	strcat(en_msg, ",");
	strcat(en_msg, out_msg.msg_to);
	strcat(en_msg, ",");
	strcat(en_msg, out_msg.msg_text);

	return en_msg;
}

int main(){

	strcpy(username, "user0");
	MESSAGE mms;
	strcpy(mms.msg_to, "user1");
	strcpy(mms.msg_text, "Hello All");

	char *msg_to_send = encode_msg(mms);

	// const char* server_name = "172.17.2.55";
	const char* server_name = "192.168.43.55";
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

	const char* data_to_send = "Sockets Networking API";
	send(sock, data_to_send, strlen(data_to_send), 0);

	int n=0;
	int len=0, maxlen=200;
	char buffer[maxlen];
	char *pbuffer = buffer;

	while((n = recv(sock, pbuffer, maxlen, 0)) > 0){
		pbuffer += n;
		maxlen -= n;
		len += n;

		buffer[len] = '\0';
		printf("Recieved Message: %s\n", buffer);
	}

	close(sock);
	return 0;
}
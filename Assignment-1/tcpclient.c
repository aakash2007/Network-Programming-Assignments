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
	int maxlen = 100;
	char buffer[maxlen];
	char *pbuffer = buffer;
	int n;

	int md = 1;
	char inp[10];

	do{

		printf("1. New User?\n");
		printf("2. Existing User? Login\n");
		printf("3. Exit\n");

		scanf("%s", inp);
		md = atoi(inp);

		if(!(md == 1 || md == 2 || md == 3)){
			printf("Incorrect Choice. Please Try Again\n\n");
		}

	}while(!(md == 1 || md == 2 || md == 3));

	char mode[10];
	if(md == 1){
		strcpy(mode, "1");
		send(sock, mode, strlen(mode), 0);
		sleep(0.01);

	}
	else if(md == 2){
		strcpy(mode, "2");
		send(sock, mode, strlen(mode), 0);
		sleep(0.01);

		printf("Enter Username: ");
		scanf("%s", username);
		password = getpass("Enter Password: ");
		// Generating Verification String
		strcpy(data_to_send, username);
		strcat(data_to_send, ",");
		strcat(data_to_send, password);
		// printf("%s 	%ld\n", data_to_send, strlen(data_to_send));
		send(sock, data_to_send, 100, 0);
		n = recv(sock, pbuffer, maxlen, 0);
		buffer[n] = '\0';

		int usr_ver = atoi(buffer);
		
		if(usr_ver == 1){
			n = recv(sock, pbuffer, maxlen, 0);
			buffer[n] = '\0';

			printf("Response from Server: %s  %ld\n", buffer, strlen(buffer));
		}
		else{
			printf("Incorrect Username or Password!\nPlease Connect Again and Retry.\n");
			close(sock);
			return 0;
		}

	}
	else{
		close(sock);
		return 0;
	}

	close(sock);
	return 0;
}
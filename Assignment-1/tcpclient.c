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
	strcpy(en_msg, out_msg.msg_from);
	strcat(en_msg, ";");
	strcat(en_msg, out_msg.msg_to);
	strcat(en_msg, ";");
	strcat(en_msg, out_msg.msg_text);

	return en_msg;
}

void handle_output(char* inp_str){
	char tstr[256];
	strcpy(tstr, inp_str);
	// printf("\n\nssd %s\n\n", tstr);
	char *pt;
	pt = strtok(tstr, ";");
	if(strcmp(pt, "inc_msg") == 0){
		pt = strtok(NULL, ";");
		printf("\nMessage from %s: ", pt);
		pt = strtok(NULL, ";");
		printf("%s\n", pt);
	}
	else if(strcmp(pt, "msgack") == 0){
		pt = strtok(NULL, ";");
		int ack = atoi(pt);
		if(ack == 1){
			printf("\nMessage Sent\n");
		}
		else{
			printf("No Such User Exists!\n");
		}
	}
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
	int maxlen = 256;
	char buffer[maxlen];
	char *pbuffer = buffer;
	int n;

	int md = 1;
	char inp[10];

	do{

		printf("1. New User?\n");
		printf("2. Existing User? Login\n");
		printf("3. Exit\n");
		printf("Select an option: ");

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

		// printf("Create New User*\n");
		char usrnm[20], fname[50], lname[50];
		printf("\nEnter Details: \n");
		printf("Username: ");
		scanf("%s", usrnm);
		printf("First Name: ");
		scanf("%s", fname);
		printf("Last Name: ");
		scanf("%s", lname);
		password = getpass("Password: ");

		char new_user[maxlen];
		strcpy(new_user, usrnm);
		strcat(new_user, ";");
		strcat(new_user, password);
		strcat(new_user, ";");
		strcat(new_user, fname);
		strcat(new_user, ";");
		strcat(new_user, lname);

		send(sock, new_user, strlen(new_user), 0);
		sleep(0.01);

		n = recv(sock, pbuffer, maxlen, 0);
		buffer[n] = '\0';

		char tstr[20], *pt;
		strcpy(tstr, buffer);
		
		pt = strtok(tstr, ";");
		pt = strtok(NULL, ";");

		int scc = atoi(pt);

		if(scc == 1){
			printf("\nUser Successfully Created!\nConnect Again to Login.\n");
			close(sock);
			return 0;
		}
		else{
			printf("\nUsername Already Exists!\nPlease Try Again with different username.\n");
			close(sock);
			return 0;
		}

	}
	else if(md == 2){
		strcpy(mode, "2");
		send(sock, mode, strlen(mode), 0);
		sleep(0.01);

		printf("\nEnter Username: ");
		scanf("%s", username);
		password = getpass("Enter Password: ");
		// Generating Verification String
		strcpy(data_to_send, username);
		strcat(data_to_send, ";");
		strcat(data_to_send, password);
		// printf("%s 	%ld\n", data_to_send, strlen(data_to_send));
		send(sock, data_to_send, strlen(data_to_send), 0);
		sleep(0.1);
		n = recv(sock, pbuffer, maxlen, 0);
		buffer[n] = '\0';

		// printf("vv00 %s\n", buffer);
		char *pt, tstr[maxlen];
		strcpy(tstr, buffer);
		pt = strtok(tstr, ";");
		pt = strtok(NULL, ";");
		int usr_ver = atoi(pt);
		
		if(usr_ver == 1){
			n = recv(sock, pbuffer, maxlen, 0);
			buffer[n] = '\0';

			printf("\nMessage from Server: %s\n\n", buffer);

			pid_t lis_child;
			lis_child = fork();

			if(lis_child == 0){
				pid_t parent = getppid();
				while(1){
					if(kill(parent, 0) == 0){
						n = recv(sock, pbuffer, maxlen, 0);
						buffer[n] = '\0';
						sleep(1);
						handle_output(buffer);
					}
					else{
						close(sock);
						exit(0);
					}
				}
				exit(0);
			}

			while(1){
				sleep(1);
				int op;
				do{
					printf("\nWhat would you like to do?\n");
					printf("1. Send a Private Message\n");
					printf("2. Send a Broadcast Message\n");
					printf("3. Get Status of Other Users\n");
					printf("4. Block a User\n");
					printf("5. Unblock a User\n");
					printf("6. Exit\n");
					printf("Select an option: ");

					scanf("%s", inp);
					op = atoi(inp);

					if(!(op == 1 || op == 2 || op == 3 || op == 4 || op == 5 || op == 6)){
						printf("Incorrect Choice. Please Try Again\n\n");
					}
					fflush(stdin);
				}while(!(op == 1 || op == 2 || op == 3 || op == 4 || op == 5 || op == 6));

				if(op == 1){
					
					strcpy(mode, "1");
					send(sock, mode, strlen(mode), 0);
					sleep(0.01);

					MESSAGE snd_msg;
					strcpy(snd_msg.msg_from, username);
					printf("\nSend To: ");
					scanf("%s", snd_msg.msg_to);
					printf("Enter Message: ");
					fgets(snd_msg.msg_text, 100, stdin);
					fgets(snd_msg.msg_text, 100, stdin);

					char *msg_to_send = encode_msg(snd_msg);
					// printf("%s %ld\n", msg_to_send, strlen(msg_to_send));
					send(sock, msg_to_send, strlen(msg_to_send)-1, 0);
					sleep(0.01);

				}
				else if(op == 2){
					strcpy(mode, "2");
					send(sock, mode, strlen(mode), 0);
					sleep(0.01);

					MESSAGE snd_msg;
					strcpy(snd_msg.msg_from, username);
					strcpy(snd_msg.msg_to, "broadcast");
					printf("Enter Message: ");
					fgets(snd_msg.msg_text, 100, stdin);
					fgets(snd_msg.msg_text, 100, stdin);

					char *msg_to_send = encode_msg(snd_msg);
					// printf("%s %ld\n", msg_to_send, strlen(msg_to_send));
					send(sock, msg_to_send, strlen(msg_to_send)-1, 0);
					sleep(0.01);

				}
				else if(op == 3){
					strcpy(mode, "3");
					send(sock, mode, strlen(mode), 0);
					sleep(0.01);

				}
				else if(op == 4){
					strcpy(mode, "4");
					send(sock, mode, strlen(mode), 0);
					sleep(0.01);

					char bl_usr[20];
					printf("\n Enter Username to Block: ");
					scanf("%s", bl_usr);

					char block_str[maxlen];
					strcpy(block_str, "block;");
					strcat(block_str, bl_usr);
					send(sock, block_str, strlen(block_str), 0);
					sleep(0.01);

				}
				else if(op == 5){
					strcpy(mode, "5");
					send(sock, mode, strlen(mode), 0);
					sleep(0.01);

					char unbl_usr[20];
					printf("\nEnter Username to Unblock: ");
					scanf("%s", unbl_usr);

					char block_str[maxlen];
					strcpy(block_str, "unblock;");
					strcat(block_str, unbl_usr);
					send(sock, block_str, strlen(block_str), 0);
					sleep(0.01);

				}
				else if(op == 6){
					strcpy(mode, "6");
					send(sock, mode, strlen(mode), 0);
					sleep(0.01);
					printf("\nGoodbye!\n");
					close(sock);
					return 0;	
				}
				sleep(1);
			}
			return 0;

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

	wait(NULL);
	close(sock);
	return 0;
}
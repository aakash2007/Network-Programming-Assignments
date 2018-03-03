#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>

int MAX_USERS = 20;

typedef struct my_msg		// For Sending Over Network
{
	char msg_from[20];
	char msg_to[20];
	char msg_text[020];
} MESSAGE;

struct mymsg_buf	// For Message Queue
{
	long mtype;
	char msg_from[20];
	char msg_text[200];
};

MESSAGE decode_msg(char* en_msg){
	char *pt;
	MESSAGE rcvd_msg;
	pt = strtok(en_msg, ",");
	strcpy(rcvd_msg.msg_from, pt);
	pt = strtok(NULL, ",");
	strcpy(rcvd_msg.msg_to, pt);
	pt = strtok(NULL, ",");
	strcpy(rcvd_msg.msg_text, pt);

	return rcvd_msg;
}

typedef struct user_det{
	char username[20];
	char first_name[50];
	char last_name[50];
	char password[50];
	long user_id;
} USER;

typedef struct user_det* user_ptr;

user_ptr user_arr_begin;
int* registered_users;

void server_start_message(){
	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "enp2s0", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);

	printf("Server Running on %s, Port: %d\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), SERVER_PORT);
}

int verify_user(char *inp_str){

	char vstr[50];
	strcpy(vstr, inp_str);
	char* pt;
	char usrnm[20], pass[50];
	pt = strtok(vstr, ",");
	strcpy(usrnm, pt);
	pt = strtok(NULL, ",");
	strcpy(pass, pt);

	user_ptr tmp = user_arr_begin;

	for (int i = 0; i < registered_users; ++i)
	{
		if(strcmp(tmp->username, usrnm) == 0){		// User is Present
			if(strcmp(tmp->password, pass) == 0){	// Passwords Matched
				return 1;			// All OK, allow login
			}
			else{
				return 2;			// User Exists, Password incorrect, retry
			}
		}

		tmp++;
	}

	return 0;		// User doesn't exist, ask to create new.
}

void create_new_user(char* usr_str){
	
}

void handle_client(int conn_sockfd, struct sockaddr_in client_addr){
	int maxlen = 100;
	char buffer[maxlen];
	char *pbuffer = buffer;

	recv(conn_sockfd, pbuffer, maxlen, 0);

	// printf("Recieved String: %s\n", buffer);
	int resp = verify_user(buffer);
	printf("Verification: %d\n", resp);

	char res[3];
	if(resp == 1){

	}
	else if(resp == 2){

	}
	else if(resp == 0){
		strcpy(res, "0");
		recv(conn_sockfd, pbuffer, maxlen, 0);
		create_new_user(buffer);
	}
}

int main(){
	const int SERVER_PORT = 6789;

	server_start_message();	

	// char srv_ip[20];
	// printf("Input the IP Address of Machine: ");
	// scanf("%s", srv_ip);

	// Array in Shared Memory for User Details
	const int arr_sz = MAX_USERS*sizeof(USER) + sizeof(int);

	// First 4 bytes to store no. of registered users and rest user_det array

	int shmid = shmget(IPC_PRIVATE, arr_sz, 0666);
	int semid = semget(IPC_PRIVATE, 2, 0666);
	
	void* shm_ptr = shmat(shmid, NULL, 0);
	registered_users = (int*)shm_ptr;
	*registered_users = 0;
	user_arr_begin = (user_ptr)(shm_ptr + sizeof(int));



	// dummy users
	USER u1, u2;
	strcpy(u1.username, "aakash");
	strcpy(u1.password, "bajaj1234");
	u1.user_id = 100;

	strcpy(u2.username, "deepak");
	strcpy(u2.password, "kar987kar");
	u2.user_id = 200;

	user_ptr ptr = user_arr_begin;
	*ptr = u1;
	ptr++;
	*ptr = u2;
	*registered_users = 2;



	// Message Queue for IPC
	int msqid;
	if((msqid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT)) == -1){
		perror("msgget");
		exit(1);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);


	// Open Stream Socket
	int lis_sockfd;
	if((lis_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("listening socket");
		exit(1);
	}

	if((bind(lis_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
		perror("bind");
		exit(1);
	}

	int wait_size = 15;

	if((listen(lis_sockfd, wait_size)) <  0){
		perror("listen");
		exit(1);
	}

	struct sockaddr_in client_addr;
	int client_addr_len = 0;
	int conn_sockfd;

	printf("Server Ready to Accept Connections....\n\n");

	pid_t child_pid;
	for(;;){		// to accept connections and create child processes
		if((conn_sockfd = accept(lis_sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0){
			perror("accept");
			exit(1);
		}

		if((child_pid = fork()) == 0){		// Child Process
			close(lis_sockfd);
			break;							// Child Process exits the loop
		}									// Parent keeps listening
		else{
			// close(conn_sockfd);			// Causing Connection Issues
			wait(NULL);
		}
	}

	if(child_pid == 0){
		printf("Connected with IP: %s\n", inet_ntoa(client_addr.sin_addr));
		handle_client(conn_sockfd, client_addr);
		exit(0);
	}

	return 0;
}
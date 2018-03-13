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
const int SERVER_PORT = 6789;

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
	pid_t child_pid;
	char first_name[50];
	char last_name[50];
	char password[50];
	long user_id;
	int online_status;
} USER;

typedef struct user_det* user_ptr;

user_ptr user_arr_begin;
int* registered_users;

int shmid, msqid, semid;		// System V IPC identifiers

void server_start_message(){
	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "enp2s0", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);

	printf("Server Starting on %s, Port: %d\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), SERVER_PORT);
}

user_ptr find_user(char* usrnm){
	
	user_ptr tmp = user_arr_begin;

	for (int i = 0; i < *registered_users; ++i)
	{
		if(strcmp(tmp->username, usrnm) == 0){		// User is Present
			return tmp;
		}
		tmp++;
	}
	return NULL;		// User doesn't exist, ask to create new.
}


user_ptr verify_user(char *inp_str){

	char vstr[50];
	strcpy(vstr, inp_str);
	char* pt;
	char usrnm[20], pass[50];
	pt = strtok(vstr, ",");
	strcpy(usrnm, pt);
	pt = strtok(NULL, ",");
	strcpy(pass, pt);

	user_ptr tmp = user_arr_begin;

	for (int i = 0; i < *registered_users; ++i)
	{
		if(strcmp(tmp->username, usrnm) == 0){		// User is Present
			if(strcmp(tmp->password, pass) == 0){	// Passwords Matched
				return tmp;			// All OK, allow login
			}
			else{
				return NULL;			// User Exists, Password incorrect, retry
			}
		}

		tmp++;
	}
	return NULL;		// User doesn't exist, ask to create new.
}

int create_new_user(char* usr_str){
	char tstr[200];
	strcpy(tstr, usr_str);
	// printf("in: %s\n", tstr);
	char usrnm[20], fname[50], lname[50], pass[50];
	char* pt;
	pt = strtok(tstr, ",");
	strcpy(usrnm, pt);
	pt = strtok(tstr, ",");
	strcpy(pass, pt);
	pt = strtok(tstr, ",");
	strcpy(fname, pt);
	pt = strtok(tstr, ",");
	strcpy(lname, pt);

	user_ptr ptr = find_user(usrnm);

	if(ptr != NULL){
		return 2;
	}

	USER new_usr;
	strcpy(new_usr.username, usrnm);
	strcpy(new_usr.password, pass);
	strcpy(new_usr.first_name, fname);
	strcpy(new_usr.last_name, lname);

	// *************semaphore ACCESSING RESOURCE  todo
	ptr = user_arr_begin;

	for (int i = 0; i < *registered_users; ++i)
	{
		ptr++;
	}
	*ptr = new_usr;
	(*registered_users)++;
	// *************semaphore RELEASING RESOURCE  todo


	return 1;
}

void handle_client(int conn_sockfd, struct sockaddr_in client_addr){
	int maxlen = 256;
	char buffer[maxlen];
	char *pbuffer = buffer;
	int n;

	int mode;
	n = recv(conn_sockfd, pbuffer, maxlen, 0);	
	buffer[n] = '\0';
	mode = atoi(buffer);

	if(mode == 1){
		n = recv(conn_sockfd, pbuffer, maxlen, 0);	
		buffer[n] = '\0';
		int scc = create_new_user(buffer);
		char scc_str[10];
		if(scc == 1){
			strcpy(scc_str, "1");
			send(conn_sockfd, scc_str, strlen(scc_str), 0);
			sleep(0.01);
		}
		else{
			strcpy(scc_str, "2");
			send(conn_sockfd, scc_str, strlen(scc_str), 0);
			sleep(0.01);
		}
		close(conn_sockfd);
		return;
	}
	else if(mode == 2){

		n = recv(conn_sockfd, pbuffer, maxlen, 0);
		buffer[n] = '\0';
		// printf("%s %ld\n", buffer, strlen(buffer));
		user_ptr conn_user = verify_user(buffer);
		// printf("Verification: %d\n", ver_usr);

		char ver_usr[3];

		if(conn_user != NULL){		// Verification OK
			conn_user->child_pid = getpid();
			conn_user->online_status = 1;

			strcpy(ver_usr, "1");
			send(conn_sockfd, ver_usr, strlen(ver_usr), 0);
			sleep(0.01);

			// Welcome User
			char welcome[100];
			strcpy(welcome, "Welcome ");
			strcat(welcome, conn_user->first_name);
			strcat(welcome, "!");
			// printf("%s %ld\n", welcome, strlen(welcome));
			send(conn_sockfd, welcome, strlen(welcome), 0);
			sleep(0.01);

			pid_t lis_child;
			lis_child = fork();

			if(lis_child == 0){			// Child Process to listen to incoming message for client
				while(1){
					// MESSAGE inc_msg = msgrcv();
				}
				exit(0);
			}

			while(1){
				int oper;
				n = recv(conn_sockfd, pbuffer, maxlen, 0);
				buffer[n] = '\0';
				oper = atoi(buffer);
				if(oper == 1){

				}
				else if(oper == 2){

				}
				else if(oper == 3){

				}
			}



		}
		else{
			strcpy(ver_usr, "2");
			send(conn_sockfd, ver_usr, strlen(ver_usr), 0);
			sleep(0.01);
		}
	}
	// else if(ver_usr == 2){		// Password Incorrect
	// 	strcpy(res, "2");
	// 	send(conn_sockfd, res, sizeof(res), 0);
	// }
	// else if(ver_usr == 0){		// Create New User?
	// 	strcpy(res, "0");
	// 	send(conn_sockfd, res, sizeof(res), 0);
	// 	recv(conn_sockfd, pbuffer, maxlen, 0);
	// 	create_new_user(buffer);
	// }
}

int main(){

	server_start_message();	

	// char srv_ip[20];
	// printf("Input the IP Address of Machine: ");
	// scanf("%s", srv_ip);

	// Array in Shared Memory for User Details
	const int arr_sz = MAX_USERS*sizeof(USER) + sizeof(int);

	// First 4 bytes to store no. of registered users and rest user_det array

	shmid = shmget(IPC_PRIVATE, arr_sz, 0666);
	semid = semget(IPC_PRIVATE, 2, 0666);
	
	void* shm_ptr = shmat(shmid, NULL, 0);
	registered_users = (int*)shm_ptr;
	*registered_users = 0;
	user_arr_begin = (user_ptr)(shm_ptr + sizeof(int));



	// dummy users
	USER u1, u2;
	strcpy(u1.username, "aakash");
	strcpy(u1.password, "bajaj1234");
	strcpy(u1.first_name, "Aakash");
	strcpy(u1.last_name, "Bajaj");
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

	// **To test user creation

	// pid_t temp;
	// temp = fork();
	// if(temp == 0){
	// 	while(1){
	// 		sleep(2);
	// 		printf("%d \n", *registered_users);
	// 	}
	// }


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
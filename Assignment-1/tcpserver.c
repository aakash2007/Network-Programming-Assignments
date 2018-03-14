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
#include <sys/signal.h>

int MAX_USERS = 20;
const int SERVER_PORT = 6789;

typedef struct my_msg		// For Sending Over Network
{
	char msg_from[20];
	char msg_to[20];
	char msg_text[256];
} MESSAGE;

struct mymsg_buf	// For Message Queue
{
	long mtype;
	char msg_from[50];
	char msg_text[256];
};

typedef struct user_det{
	char username[20];
	pid_t child_pid;
	char first_name[50];
	char last_name[50];
	char password[50];
	long user_id;
	long blocked_id[100];
	int online_status;
} USER;

MESSAGE decode_msg(char* en_msg){
	char *pt;
	MESSAGE rcvd_msg;
	pt = strtok(en_msg, ";");
	strcpy(rcvd_msg.msg_from, pt);
	pt = strtok(NULL, ";");
	strcpy(rcvd_msg.msg_to, pt);
	pt = strtok(NULL, ";");
	strcpy(rcvd_msg.msg_text, pt);

	return rcvd_msg;
}

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
	pt = strtok(vstr, ";");
	strcpy(usrnm, pt);
	pt = strtok(NULL, ";");
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
	pt = strtok(tstr, ";");
	strcpy(usrnm, pt);
	pt = strtok(NULL, ";");
	strcpy(pass, pt);
	pt = strtok(NULL, ";");
	strcpy(fname, pt);
	pt = strtok(NULL, ";");
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
	new_usr.online_status = 0;

	// *************semaphore ACCESSING RESOURCE  todo
	ptr = user_arr_begin;
	for (int i = 0; i < *registered_users; ++i)
	{
		ptr++;
	}
	(*registered_users)++;
	int usrid = (*registered_users)+1;
	new_usr.user_id = usrid;

	*ptr = new_usr;
	// *************semaphore RELEASING RESOURCE  todo


	return 1;
}

void update_user_status(){
	user_ptr ptr = user_arr_begin;
	for (int i = 0; i < *registered_users; ++i)
	{
		if(kill(ptr->child_pid, 0) == 0){
			ptr->online_status = 1;
		}
		else{
			ptr->online_status = 0;
		}
		ptr++;
	}
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
		char scc_str[20];
		if(scc == 1){
			strcpy(scc_str, "createuser;1");
			send(conn_sockfd, scc_str, strlen(scc_str), 0);
			sleep(0.01);
		}
		else{
			strcpy(scc_str, "createuser;2");
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
		// printf("Verification:\n");

		char ver_usr[15];

		if(conn_user != NULL){		// Verification OK

			strcpy(ver_usr, "verusr;1");
			// printf("%s\n", ver_usr);
			send(conn_sockfd, ver_usr, strlen(ver_usr), 0);
			sleep(1);

			conn_user->child_pid = getpid();
			conn_user->online_status = 1;
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
				pid_t parent = getppid();
				while(1){
					sleep(1);
					if(kill(parent, 0) == 0){
						struct mymsg_buf inc_msg;
						msgrcv(msqid, &inc_msg, sizeof(inc_msg), conn_user->user_id, 0);
						printf("lls %s\n", inc_msg.msg_text);
						char send_msg_str[maxlen];
						strcpy(send_msg_str, "inc_msg;");
						strcat(send_msg_str, inc_msg.msg_from);
						strcat(send_msg_str, ";");
						strcat(send_msg_str, inc_msg.msg_text);
						sleep(0.5);
						printf("send_msg_str %s\n", send_msg_str);
						send(conn_sockfd, send_msg_str, strlen(send_msg_str), 0);
					}
					else{
						conn_user->online_status = 0;
						close(conn_sockfd);
						exit(0);
					}
				}
				exit(0);
			}

			while(1){
				int oper;
				n = recv(conn_sockfd, pbuffer, maxlen, 0);
				buffer[n] = '\0';
				oper = atoi(buffer);
				// printf("%d\n", oper);
				if(oper == 1){
					n = recv(conn_sockfd, pbuffer, maxlen, 0);
					buffer[n] = '\0';
					printf("%s %ld\n", buffer, strlen(buffer));
					MESSAGE rcvd_msg = decode_msg(buffer);
					printf("from user %s %s %s\n", rcvd_msg.msg_from, rcvd_msg.msg_to, rcvd_msg.msg_text);

					user_ptr frm_usr = find_user(rcvd_msg.msg_from);
					user_ptr targ_usr = find_user(rcvd_msg.msg_to);

					char msg_ack[15];
					strcpy(msg_ack, "msgack;");
					if(targ_usr != NULL){
						struct mymsg_buf q_msg;
						q_msg.mtype = targ_usr->user_id;
						strcpy(q_msg.msg_from, frm_usr->first_name);
						strcpy(q_msg.msg_text, rcvd_msg.msg_text);

						printf("to msq %s %ld %s\n", q_msg.msg_from, q_msg.mtype, q_msg.msg_text);
						msgsnd(msqid, &q_msg, sizeof(q_msg), 0);
						
						strcat(msg_ack, "1");
						send(conn_sockfd, msg_ack, strlen(msg_ack), 0);
						sleep(0.01);
					}
					else{
						strcat(msg_ack, "2");
						send(conn_sockfd, msg_ack, strlen(msg_ack), 0);
						sleep(0.01);
					}
				}
				else if(oper == 2){
					n = recv(conn_sockfd, pbuffer, maxlen, 0);
					buffer[n] = '\0';
					printf("%s %ld\n", buffer, strlen(buffer));
					MESSAGE rcvd_msg = decode_msg(buffer);
					printf("from user %s %s %s\n", rcvd_msg.msg_from, rcvd_msg.msg_to, rcvd_msg.msg_text);

					user_ptr frm_usr = find_user(rcvd_msg.msg_from);

					char msg_ack[15];
					strcpy(msg_ack, "msgack;");
					struct mymsg_buf q_msg;
					q_msg.mtype = 1;
					strcpy(q_msg.msg_from, frm_usr->first_name);
					strcpy(q_msg.msg_text, rcvd_msg.msg_text);

					printf("to msq %s %ld %s\n", q_msg.msg_from, q_msg.mtype, q_msg.msg_text);
					msgsnd(msqid, &q_msg, sizeof(q_msg), 0);
						
					strcat(msg_ack, "1");
					send(conn_sockfd, msg_ack, strlen(msg_ack), 0);
					sleep(0.01);

				}
				else if(oper == 3){

				}
				else if(oper == 4){

				}
				else if(oper == 5){

				}
				else if(oper == 6){
					conn_user->online_status = 0;
					kill(lis_child, SIGINT);
					close(conn_sockfd);
					// kill(lis_child, SIGINT);
					// wait(NULL);
					exit(0);
				}
			}



		}
		else{
			strcpy(ver_usr, "2");
			send(conn_sockfd, ver_usr, strlen(ver_usr), 0);
			sleep(0.01);
		}
	}
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
	USER u1, u2, u3;
	strcpy(u1.username, "aakash");
	strcpy(u1.password, "bajaj");
	strcpy(u1.first_name, "Aakash");
	strcpy(u1.last_name, "Bajaj");
	u1.user_id = 2;

	strcpy(u2.username, "deepak");
	strcpy(u2.password, "kar");
	strcpy(u2.first_name, "Deepak");
	strcpy(u2.last_name, "Kar");
	u2.user_id = 3;

	strcpy(u3.username, "abhi");
	strcpy(u3.password, "abhi");
	strcpy(u3.first_name, "Abhi");
	strcpy(u3.last_name, "Bajaj");
	u3.user_id = 4;

	user_ptr ptr = user_arr_begin;
	*ptr = u1;
	ptr++;
	*ptr = u2;
	ptr++;
	*ptr = u3;
	*registered_users = 3;



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

	pid_t temp;
	temp = fork();
	if(temp == 0){
		while(1){
			sleep(2);
			// update_user_status();
			user_ptr ppp = user_arr_begin;
			for (int i = 0; i < *registered_users; ++i)
			{
				printf("KS %ld %s %s %s %s %d\n", ppp->user_id, ppp->username, ppp->password, ppp->first_name, ppp->last_name, ppp->online_status);
				ppp++;
			}
			printf("%d \n", *registered_users);
		}
	}

	pid_t broad_child;
	broad_child = fork();
	if(broad_child == 0){
		while(1){
			sleep(1);
			if(kill(getppid(), 0) == 0){
					struct mymsg_buf inc_msg;
					msgrcv(msqid, &inc_msg, sizeof(inc_msg), 1, 0);
					// printf("broad %s\n", inc_msg.msg_text);
					user_ptr ptr = user_arr_begin;
					for (int i = 0; i < *registered_users; ++i)
					{
						inc_msg.mtype = ptr->user_id;
						if(!(strcmp(inc_msg.msg_from, ptr->first_name)==0))
							msgsnd(msqid, &inc_msg, sizeof(inc_msg), 0);
						ptr++;
					}
				}
				else{
					close(conn_sockfd);
					exit(0);
				}
		}
	}

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
			close(conn_sockfd);			// Causing Connection Issues
		}
	}

	if(child_pid == 0){
		printf("Connected with IP: %s\n", inet_ntoa(client_addr.sin_addr));
		handle_client(conn_sockfd, client_addr);
		exit(0);
	}

	wait(NULL);
	return 0;
}
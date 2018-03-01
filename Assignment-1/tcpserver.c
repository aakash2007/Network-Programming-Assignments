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

#define MAX_USERS 20

int users[MAX_USERS][3];	

typedef struct my_msg		// For Sending Over Network
{
	char msg_from[20];
	char msg_to[20];
	char msg_text[200];
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
	char password[20];
	long user_id;
} USER;

typedef struct user_det* user_ptr;

int main(){
	const int SERVER_PORT = 6789;

	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "enp2s0", IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);

	// printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	
	// char srv_ip[20];
	// printf("Input the IP Address of Machine: ");
	// scanf("%s", srv_ip);

	// Array in Shared Memory for User Details
	const int arr_sz = MAX_USERS*sizeof(USER);

	int shmid = shmget(IPC_PRIVATE, arr_sz, 0666);
	int semid = semget(IPC_PRIVATE, 2, 0666);
	user_ptr user_arr_begin = (user_ptr)shmat(shmid, NULL, 0);

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

	printf("Server Running on %s, Port: %d\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr), SERVER_PORT);

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

	printf("Server Ready to Accept Connections\n");

	pid_t child_pid;
	for(;;){
		if((conn_sockfd = accept(lis_sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0){
			perror("accept");
			exit(1);
		}



		if((child_pid = fork()) == 0){		// Child Process
			close(lis_sockfd);
			break;							// Child Process exits the loop
		}									// Parent keeps listening
		else{
			close(conn_sockfd);
			wait(NULL);
		}
	}

	if(child_pid == 0){
		int n=0;
		int len=0, maxlen=200;
		char buffer[maxlen];
		char *pbuffer = buffer;
		
		printf("Connected with IP: %s\n", inet_ntoa(client_addr.sin_addr));
		
		while ((n = recv(conn_sockfd, pbuffer, maxlen, 0)) > 0) {
			pbuffer += n;
			maxlen -= n;
			len += n;

			printf("received: '%s'\n", buffer);

			// echo received content back
			send(conn_sockfd, buffer, len, 0);
		}
		exit(0);
	}

	return 0;
}
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
int SERVER_PORT = 6789;

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

void handler(){
	pause();
}

int main(int argc, char **argv){

	if (argc < 2) {
		fprintf(stderr, "Usage: ./tcpserver <port>\n");
		exit(1);
	}
	SERVER_PORT = atoi(argv[1]);
	// server_start_message();	

	// char srv_ip[20];
	// printf("Input the IP Address of Machine: ");
	// scanf("%s", srv_ip);

	// Array in Shared Memory for User Details
	// printf("Enter Port No. to Start Chat Server: ");
	// scanf("%d", &SERVER_PORT);


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
			close(conn_sockfd);			// Causing Connection Issues
		}
	}

	if(child_pid == 0){
		printf("Connected with IP: %s %d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
		// handle_client(conn_sockfd, client_addr);
		exit(0);
	}

	wait(NULL);
	return 0;
}
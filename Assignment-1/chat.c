#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct my_msg
{
	char msg_from[20];
	char msg_to[20];
	char msg_text[200];
};

struct mymsg_buf
{
	long mtype;
	char msg_from[20];
	char msg_text[200];
};

int main(){
	const int SERVER_PORT = 6789;

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

	struct sockaddr_in client_addr;
	int client_addr_len = 0;

	for(;;){
		int conn_sockfd;
		if((conn_sockfd = accept(lis_sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0){
			perror("accept");
			exit(1);
		}
		pid_t child_pid;
		if((child_pid = fork()) == 0){		// Child Process
			close(lis_sockfd);
			break;
		}
		else{
			close(conn_sockfd);
		}
	}

	if(child_pid == 0){

	}

	return 0;
}
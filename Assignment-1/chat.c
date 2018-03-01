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
		perror("Listening Socket");
		exit(1);
	}

	if((bind(lis_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
		perror("Socket Binding");
		exit(1);
	}

	struct sockaddr_in client_addr;
	int client_addr_len = 0;

	for(;;){
		
	}

} 
#include "nmb.h"

void client_facing();
void network_facing();

int msqid;
pid_t child_pid;

int main(int argc, char const *argv[])
{
	// printf("ss %ld\n", sizeof(struct loc_msg));

	if((msqid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT)) == -1){
		perror("msgget");
		exit(1);
	}

	child_pid = fork();
	if(child_pid == 0){
		network_facing();
	}
	else{
		client_facing();
	}
	return 0;
}

void client_facing(){
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(TCP_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// printf("server: %s  \n", inet_ntoa(server_addr.sin_addr));


	// Open Stream Socket
	int lis_sockfd;
	if((lis_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("listening socket");
		exit(1);
	}

	if((bind(lis_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
		perror("bind");
		kill(child_pid, SIGINT);
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

	fd_set allset, rset, wset;
	FD_ZERO(&rset);
	FD_ZERO(&allset);
	FD_ZERO(&wset);

	FD_SET(lis_sockfd, &allset);

	struct sockaddr_in addr;
	int addrlen, sock, cnt;
	struct ip_mreq mreq;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0){
		perror("Socket Error");
		exit(1);
	}
	bzero((char *)&addr, sizeof(addr));
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(BUS_PORT);
	addrlen = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr(BUS_GROUP);

	int maxfd = lis_sockfd;
	struct loc_msg out_msg;

	int maxlen = 256;
	char buffer[maxlen];
	// char *pbuffer = buffer;
	void* pbuffer;
	pbuffer = (void *)malloc(sizeof(out_msg));
	int n;

	while(1){
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		memset(pbuffer, 0, sizeof(out_msg));
		bzero(pbuffer, sizeof(out_msg));

		rset = allset;
		wset = allset;

		FD_SET(lis_sockfd, &rset);

		if(select(maxfd+1, &rset, NULL, NULL, NULL) > 0){
			if(FD_ISSET(lis_sockfd, &rset)){
				if((conn_sockfd = accept(lis_sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0){
					perror("accept");
					exit(1);
				}
				FD_SET(conn_sockfd, &allset);
				if(conn_sockfd > maxfd){
					maxfd = conn_sockfd;
				}
			}

			for (int i = 0; i < maxfd+1; ++i)
			{
				if(FD_ISSET(i, &rset)){
					n = recv(i, pbuffer, sizeof(out_msg), 0);
					if(n <= 0){
						FD_CLR(i, &allset);
						shutdown(i, SHUT_WR);
					}

					struct loc_msg out_msg = *((struct loc_msg *)pbuffer);
					struct mymsg_buf temp;
					temp = *((struct mymsg_buf *)&out_msg);

					if(strcmp(out_msg.msg, "mreq;") == 0){
						struct mymsg_buf send_msg;
						struct mymsg_buf *send_msg_ptr = (struct mymsg_buf*)&out_msg;
						msgrcv(msqid, &send_msg, sizeof(send_msg), send_msg_ptr->mtype, IPC_NOWAIT);
						send(i, (void *)&send_msg, sizeof(send_msg), 0);
					}
					else if(n > 0){
						cnt = sendto(sock, (void *)&out_msg, sizeof(out_msg), 0, (struct sockaddr *)&addr, addrlen);
					}
				}
			}
		}



		// sleep(2);
	}

}

void network_facing(){
	struct sockaddr_in addr;
	int addrlen, sock, cnt;
	struct ip_mreq mreq;
	char msgstr[50];
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0){
		perror("Socket");
		exit(1);
	}
	bzero((char *)&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	// addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	addr.sin_port = htons(BUS_PORT);
	addrlen = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr(BUS_GROUP);

	if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		perror("Bind");
		kill(getppid(), SIGINT);
		exit(1);
	}
	mreq.imr_multiaddr.s_addr = inet_addr(BUS_GROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0){
		perror("setsockopt mreq");
		exit(1);
	}
	while(1){
		cnt = recvfrom(sock, msgstr, sizeof(msgstr), 0, (struct sockaddr *)&addr, &addrlen);
		struct mymsg_buf *inc_msg_ptr;
		struct loc_msg *in_msg_ptr;

		in_msg_ptr = (struct loc_msg *)&msgstr;
		inc_msg_ptr = (struct mymsg_buf *)in_msg_ptr;

		struct mymsg_buf inc_msg = *inc_msg_ptr;
		struct loc_msg in_msg = *in_msg_ptr;

		struct in_addr ip_addr;
    	ip_addr.s_addr = in_msg.ip_addr;

		printf("%s: message: \"%s\"  type: %ld \n", inet_ntoa(addr.sin_addr), inc_msg.message, inc_msg.mtype);
		printf("aa: %s %d %s\n", inet_ntoa(ip_addr), (in_msg.port), in_msg.msg);

    	// **********Check Condition for IP Matching**************
		msgsnd(msqid, &inc_msg, sizeof(inc_msg), 0);
	}
}
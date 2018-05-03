#include "nmb.h"

int msgget_nmb(){

	char server_name[15];
	strcpy(server_name, "127.0.0.1");
	
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;

	int sock;
	
	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket");
		return -1;
	}

	inet_pton(AF_INET, server_name, &server_addr.sin_addr);
	server_addr.sin_port = htons(TCP_PORT);

	if((connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
		perror("connect");
		return -1;
	}

	return sock;
}

int msgsnd_nmb(int nmbid, const void * msgp, size_t msgsz){
	send(nmbid, msgp, msgsz, 0);
}

size_t msgrcv_nmb(int nmbid, void * msgp, size_t maxmsgsz, long mtype){
	struct mymsg_buf mreq_msg;
	mreq_msg.mtype = mtype;
	printf("sent: %ld\n", mreq_msg.mtype);
	strcpy(mreq_msg.message, "mreq;");
	send(nmbid, (void *)&mreq_msg, maxmsgsz, 0);
	int n;
	n = recv(nmbid, msgp, sizeof(msgp), 0);
	return n;
}
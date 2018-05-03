#include "nmb.h"

int main(int argc, char const *argv[])
{
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
		printf("%s: message: \"%s\"\n", inet_ntoa(addr.sin_addr), msgstr);
	}
	return 0;
}
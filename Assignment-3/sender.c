#include "nmb.h"

int main(int argc, char const *argv[])
{
	struct sockaddr_in addr;
	int addrlen, sock, cnt;
	struct ip_mreq mreq;
	char msgstr[50];
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0){
		perror("Socket Error");
		exit(1);
	}
	bzero((char *)&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	// addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(BUS_PORT);
	
	addrlen = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr(BUS_GROUP);

	while(1){
		time_t t = time(0);
		sprintf(msgstr, "time is %-24.24s", ctime(&t));
		printf("sending: %s\n", msgstr);
		cnt = sendto(sock, msgstr, sizeof(msgstr), 0, (struct sockaddr *)&addr, addrlen);
		sleep(5);
	}

	return 0;
}
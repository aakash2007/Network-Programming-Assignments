#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>

#define TCP_PORT 1111
#define BUS_PORT 1112

#define BUS_GROUP "239.0.0.1"

struct loc_msg
{
	uint32_t ip_addr;
	uint16_t port;
	uint16_t padding;
	char msg[202];
};
typedef struct loc_msg * MSG;

struct mymsg_buf	// For Message Queue
{
	long mtype;
	char message[50];
};

int msgget_nmb();
int msgsnd_nmb(int nmbid, const void * msgp, size_t msgsz);
size_t msgrcv_nmb(int nmbid, void * msgp, size_t maxmsgsz, long mtype);
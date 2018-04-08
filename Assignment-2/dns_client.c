/*gethostbyname.c*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>

#define CLASS_IN 1
#define T_A 1
#define T_CNAME 5
#define T_PTR 12
#define T_MX 15	
#define T_AAAA 28
#define MAX_SIZE 1024

int questions[] = {T_A, T_CNAME, T_PTR, T_MX, T_AAAA};
char* types[] = {"A", "CNAME", "PTR", "MX", "AAAA"};

struct DNSHeader {
	unsigned short id;	// identification
	
	/* Allocates memory starting from RHS of a byte */
	unsigned char rd:1;	// recursion desired
	unsigned char tc:1;	// answer authoritative
	unsigned char aa:1;	// answer authoritative
	unsigned char qt:4;	// query type
	unsigned char op:1;	// operation type

	unsigned char rt:4;	// response type
	unsigned char z:3;	// reserved
	unsigned char ra:1; // recursion available

	unsigned short nques;	// number of questions
	unsigned short nans;	// number of answers
	unsigned short nauth;	// number of authority
	unsigned short nadd;	// number of additional
};

struct QuestionType {
	unsigned short qt;	// query type
	unsigned short qc;	// query class
};

void initDNSHeader(struct DNSHeader* header) {
	header->id = htons(getpid());
	header->op = 0; // query
	header->qt = 0; // standard
	header->aa = 0;
	header->tc = 0;
	header->rd = 1;	// recursion required
	header->ra = 0;
	header->z = 0;
	header->rt = 0; // no error
	header->nques = htons(1); // five questions of type A, AAAA, CNAME, PTR & MX
	header->nans = 0;
	header->nauth = 0;
	header->nadd = 0;
}

unsigned int changeHostNameToDNSFormat(char *name, char *buf) {
	unsigned int len = strlen(name), pos = 0;
	for(int i = 0; i < len; i++) {
		int j = i;
		while(i < len && name[i] != '.')	i++;
		buf[pos++] = (i-j);
		while(j < i)	buf[pos++] = name[j++];
	}
	buf[pos++] = 0;
	return pos;
}

unsigned int prepareQuery(char* buf, char *domain, int type) {
	unsigned int size = 0;

	initDNSHeader((struct DNSHeader*)(buf+size));
	size += sizeof(struct DNSHeader);

	char domain_query[MAX_SIZE];
	unsigned int dqlen = changeHostNameToDNSFormat(domain, domain_query);

	memcpy(buf+size, domain_query, dqlen);
	size += dqlen;
	struct QuestionType *quesType = (struct QuestionType *)(buf+size);
	quesType->qt = htons(type);
	quesType->qc = htons(CLASS_IN);
	size += sizeof(struct QuestionType);
	return size;
}

char *getType(int type) {
	for(int i = 0; i < (sizeof(questions)/(sizeof(int))); i++)
		if(type == questions[i])	return types[i];
	return NULL;
}

int getResponse(unsigned char *domain, unsigned char *dns_server, unsigned char *recvline, int type) {
	unsigned char sendline[MAX_SIZE];
	bzero(sendline, MAX_SIZE);
	unsigned int size = prepareQuery(sendline, domain, type);

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0), recvsize;
	struct sockaddr_in servaddr;
	if(sockfd == -1) {
		perror("Socket Error");
		exit(-1);
	}
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(53);
	inet_aton(dns_server, &servaddr.sin_addr);
	if(connect(sockfd,(struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
		perror("Connect Error");
		exit(-1);
	}

	if(send(sockfd, sendline, size, 0) == -1) {
		perror("Write Error");
		exit(-1);
	}
	if((recvsize = recv(sockfd, recvline, MAX_SIZE, 0)) == -1) {
		perror("Read Error");
		exit(-1);
	}
	return recvsize;
}

int main(int argc, char *argv[]) {
	if(argc < 3) {
		printf("Please Enter Domain Name & DNS Server.\n");
		exit(-1);
	}
	unsigned char response[MAX_SIZE];
	int size;
	for(int i = 0; i < (sizeof(questions)/sizeof(int)); i++) {
		bzero(response, MAX_SIZE);
		size = getResponse(argv[1], argv[2], response, questions[i]);
		unsigned char *data, dest[MAX_SIZE];
		bzero(dest, MAX_SIZE);
		switch(questions[i]) {
			case T_A:
				data = (response+size-(sizeof(struct in_addr)));
				inet_ntop(AF_INET, data, dest, MAX_SIZE);
				printf("%s has A type record of %s\n", argv[1], dest);
				break;
			case T_CNAME:
			case T_PTR:
			case T_MX:
				printf("%s has %s type record of ", argv[1], getType(questions[i]));
				data = response + 12 + (strlen(argv[1]) + 2) + 4 + 12;
				size = size - (12 + (strlen(argv[1]) + 2) + 4 + 12);
				if(questions[i] == T_MX && response[7] != 0) {
					data += 2;
					size -= 2;
				}
				for(int i = 0; i < size;) {
					int len = data[i];
					if(len == 192 && data[i+1] == 12) {
						printf("%s", argv[1]);
						break;
					}
					else if(len == 192 && data[i+1] == 16) {
						int j = strlen(argv[1]);
						while(argv[1][j] != '.' && j > 1)	j--; j--;
						while(argv[1][j] != '.' && j > 0)	j--;
						printf("%s", argv[1]+j+1);
						break;
					}
					else {
						for(int j = 1; j <= len; j++)	printf("%c", data[i+j]);
						i += len+1;
						if(data[i] == 0)	break;
						printf(".");
					}	
				}
				printf("\n");
				break;
			// case T_MX:
			// 	for(int j = 0; j < size; j++) {
			// 		printf("%02x ", response[j]);
			// 		if((j+1)%4 == 0)	printf("\n");
			// 	} printf("\n");
			// 	break;
			case T_AAAA:
				inet_ntop(AF_INET6, data, dest, MAX_SIZE);
				printf("%s has AAAA type record of %s\n", argv[1], dest);
				break;
			default:
				printf("Unknown Format\n");
				break;
		}
	}
}
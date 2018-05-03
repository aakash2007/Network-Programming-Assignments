#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>

#define BUFSIZE 500

int main(int argc, char const *argv[])
{
	if(argc != 2){
		printf("Usage: ./server <portno> \n");
		exit(1);
	}

	int portno = atoi(argv[1]);

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portno);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int lis_fd;
	if((lis_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("Socket");
		exit(1);
	}

	if((bind(lis_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
		perror("Bind");
		exit(1);
	}
	int wait_size = 15;

	if((listen(lis_fd, wait_size)) < 0){
		perror("Listen");
	}

	struct sockaddr_in cli_addr;
	int cli_addr_len = 0;
	int conn_fd;

	double data_gb = 1.0;
	double data_mb = data_gb*1024.0;
	double data_kb = data_mb*1024.0;
	double data_b = data_kb*1024.0;

	char data[300], temp[300];
	int n;
	strcpy(data, "e");
	for (int i = 0; i < 8; ++i)
	{
		strcpy(temp, data);
		strcat(data, temp);
	}

	FILE *fl;
	fl = fopen("output", "wb");

	printf("Creating File: %lf GB\n", data_gb);

	for (int i = 0; i < (data_b/256); ++i)
    {
    	fprintf(fl, "%s", data);
        fflush(fl);
    }
    fclose(fl);

	char buf[BUFSIZE];
    
    // printf("aa %ld\n", ftell(fl));
    // int kk = fread(buf, 1, sizeof(buf), fl);
    // printf("aa %ld\n", ftell(fl));

	printf("Server Ready to Accept Connections.....\n");

		if((conn_fd = accept(lis_fd, (struct sockaddr *)&cli_addr, &cli_addr_len)) < 0){
			perror("Accept");
			exit(1);
		}

		int nread;
		fl = fopen("output", "rb");
		nread = recv(conn_fd, buf, BUFSIZE,0);
		if(nread < 0){
			perror("Socket Read");
			exit(1);
		}
		char *pt;
		pt = strtok(buf, ":;");
		pt = strtok(NULL, ":;");
		int no_conn;
		sscanf(pt, "%d", &no_conn);
		printf("No. of Connections: %d\n", no_conn);

		char data_gb_str[10];
		strcpy(buf, "data:");
		sprintf(data_gb_str, "%0.4lf", data_gb);
		strcat(buf, data_gb_str);
		strcat(buf, ";");
		
		send(conn_fd, buf, sizeof(buf)+1, 0);

		int cli_sock[no_conn];

		for (int i = 0; i < no_conn; ++i)
		{
			if((cli_sock[i] = accept(lis_fd, (struct sockaddr *)&cli_addr, &cli_addr_len)) < 0){
				perror("Accept");
				exit(1);
			}
		}
		printf("All Connections Accepted... Sending Data\n");
		
		bzero(&buf, BUFSIZE);
		int i=0;
		while(1){
			long int pos = ftell(fl);
			nread = fread(buf, 1, sizeof(buf), fl);
			if(nread < 0){
				perror("Read");
				exit(1);
			}
			if(nread == 0){
				printf("Client done\n");
				close(conn_fd);
				exit(0);
			}
			buf[nread] = '\0';

			char posstr[10];
			sprintf(posstr, "%d", pos);
			char data_send[512];
			strcpy(data_send, posstr);
			strcat(data_send, ";");
			strcat(data_send, buf);
			strcat(data_send, ";");
			// printf("%s\n", pos	str);
			// printf("ll %s %d\n", posstr, pos);
			// printf("a %d\n", i);
			send(cli_sock[i%no_conn], data_send, sizeof(data_send)+1, 0);
			// printf("b %d\n", i);
			i++;
		}
		for (int i = 0; i < no_conn; ++i)
		{
			close(cli_sock[i]);
		}
		close(conn_fd);
		exit(0);
	// }
	return 0;
}

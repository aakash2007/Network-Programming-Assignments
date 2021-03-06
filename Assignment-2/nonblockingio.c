#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAXLINE 256

int max(int a, int b){
	if(a>b)
		return a;
	else
		return b;
}

void strcli(FILE *fl, int sockfd){
  int mxfdp, val;
  fd_set rset, wset;
  FD_ZERO(&rset);
  FD_ZERO(&wset);
  
  char to[MAXLINE], fr[MAXLINE];
  char *toiptr, *tooptr, *friptr, *froptr;

  toiptr = tooptr = to;
  friptr = froptr = fr;

  int dataeof = 0;

  int flfd = fileno(fl);
  
  val = fcntl(sockfd, F_GETFL, 0);
  fcntl(sockfd, F_SETFL, val | O_NONBLOCK);

  val = fcntl(flfd, F_GETFL, 0);
  fcntl(flfd, F_SETFL, val | O_NONBLOCK);

  mxfdp = max(flfd, sockfd) + 1;
  double data_gb = 1.0;
  double data_mb = data_gb*1024.0;
  double data_kb = data_mb*1024.0;
  double data_b = data_kb*1024.0;
  double tt = 1024*1024*1024;
  
  char data[300], temp[300];
  int n;
  strcpy(data, "e");
  for (int i = 0; i < 8; ++i)
  {
	strcpy(temp, data);
	strcat(data, temp);
  }

  char buf[1024];
  double recv_data = 0.0;

  struct timeval t1, t2;
  double elapsedTime;
  int flag = 0;
  int errno;
  int i=0;

  for(;;){
		FD_ZERO(&rset);
		FD_ZERO(&wset);

		FD_SET(sockfd, &wset);

		if(friptr < &fr[MAXLINE]){
			FD_SET(sockfd, &rset);
		}
		if(froptr != friptr){
			FD_SET(flfd, &wset);
		}

		int nready;

		if((nready=select(mxfdp, &rset, &wset, NULL, NULL)) < 0){
		  perror("select");
		  exit(1);
		}

		if(FD_ISSET(sockfd, &rset)){
		  
		  if((n = read(sockfd, friptr, &fr[MAXLINE]-friptr)) < 0){
		  	if(errno != EWOULDBLOCK)
		  		perror("read error on socket");
		  }
		  else if(n == 0){
		  	close(sockfd);
		  	break;
		  }
		  else{

		  	double perc = (recv_data*100.0)/data_b;
		  	if(perc == 100.00)	break;
		  	printf("\rData Recieved: %0.0lf/%0.0lf kB            %0.2lf %%", recv_data/(1024), data_kb, perc);

		  	if(flag == 0){
					gettimeofday(&t1, NULL);
					flag = 1;
			  }

		  	friptr += n;
		  	fprintf(fl, "%s", buf);
				fflush(fl);
				recv_data += (double)n;
		  }


		}

		if(FD_ISSET(sockfd, &wset) && i<(data_b/256)){
		  n = send(sockfd, data, strlen(data), 0);
		  i++;
		}
		int nwritten;
		if(FD_ISSET(flfd, &wset) && ((n = friptr - froptr) > 0)){
			if((nwritten = write(flfd, froptr, n)) < 0){
				if(errno != EWOULDBLOCK)
		  		perror("write error to file");
			}
			else{
				froptr += nwritten;
				if(froptr == friptr){
					froptr = friptr = fr;
				}
			}
		}
		if( !(i < (data_b/256)) ){
		  shutdown(sockfd, SHUT_WR);
		  // exit(0);
		  // break;
		}
  }
  gettimeofday(&t2, NULL);
  elapsedTime = (t2.tv_sec - t1.tv_sec)*1000.0;
  elapsedTime += (t2.tv_usec - t1.tv_usec)/1000.0;
  printf("\nTotal time : %0.3f ms.\n", elapsedTime);
  double thrg = data_mb/(elapsedTime/1000.0);
  printf("Throughput: %0.5lf MB/s\n", thrg);

  double res_time;
  res_time = elapsedTime/(data_b/256);
  printf("Response Time Per Request: %lf ms.\n", res_time);
  close(sockfd);
}


int main(int argc, char const *argv[])
{
  if(argc != 3){
	printf("Usage: ./twoprocesses <server_ip> <portno> \n");
	exit(1);
  }

  int portno = atoi(argv[2]);

  char server_name[15];
  strcpy(server_name, argv[1]);
  
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;

  inet_pton(AF_INET, server_name, &server_addr.sin_addr);
  server_addr.sin_port = htons(portno);

  int sock;

  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
	perror("socket");
	exit(1);
  }

  if((connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
	perror("connect");
	exit(1);
  }
  FILE *fl = fopen("output", "w");
  strcli(fl, sock);
  fclose(fl);
  return 0;
}
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

FILE *fl;
double data_gb, recv_data, data_mb, data_kb, data_b;

struct timeval t1, t2;
double elapsedTime;
int flag;

void * getdata(void * arg){
	int sock;
	sock = *((int *) arg);
	free(arg);

	char buf[520], temp[520];
	char* pt;
	int nread, pos;
	int tid = pthread_self();

	while(1){
		if(flag == 0){
		   	gettimeofday(&t1, NULL);
		   	flag = 1;
		}
		// printf("a %d\n", tid);
		nread = recv(sock, buf, 513,0);
		// printf("%d %d\n", i, nread);

		if(nread < 0){
			perror("conn");
			break;
		}
		if(nread == 0){
			printf("\nend\n");
			close(sock);
			break;
		}

		buf[nread] = '\0';
		// printf("\n%s\n", buf);
		strcpy(temp, buf);
		pt = strtok(temp, ";");
		pos = atoi(pt);
		pt = strtok(NULL, ";");
		strcpy(temp, pt);
		// printf("b %d\n", tid);
		// ******Use Mutex
		pthread_mutex_lock(&mtx);
		// printf("c %d\n", tid);
		fseek(fl, pos, SEEK_SET);
		// printf("d %d %d\n", tid, pos);
		// printf("%d %d\n", nread, pos);
		fwrite(pt, strlen(pt)+1, 1, fl);
		// fprintf(fl, "%s", temp);
		// printf("e %d\n", tid);
		recv_data += (double)(strlen(pt));
		// printf("f %d\n", tid);

		// ******Unlock Mutex
		pthread_mutex_unlock(&mtx);
		printf("g %d\n", tid);
	}
}

int main(int argc, char const *argv[])
{
	if(argc != 4){
		printf("Usage: ./twoprocesses <server_ip> <portno> <no_connections>\n");
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

	int no_conn = atoi(argv[3]);
	int sock[no_conn];

	int par_sock;
	if((par_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
			perror("socket");
			exit(1);
	}
	if((connect(par_sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
		perror("connect");
		exit(1);
	}

	char data_req[32];
	char no_conn_str[10];
	sprintf(no_conn_str, "%d", no_conn);

	strcpy(data_req, "no_conn:");
	strcat(data_req, no_conn_str);
	strcat(data_req, ";");

	char buf[520], temp[520];
	char* pt;
	int kn = send(par_sock, data_req, strlen(data_req)+1, 0);

	recv(par_sock, buf, 513, 0);

	pt = strtok(buf, ":;");
	pt = strtok(NULL, ":;");

	sscanf(pt, "%lf", &data_gb);
	printf("data: %0.4lf GB\n", data_gb);
  
	fl = fopen("download", "w");
	int maxsock=0;

	for (int i = 0; i < no_conn; ++i)
	{
		if((sock[i] = socket(PF_INET, SOCK_STREAM, 0)) < 0){
			perror("socket");
			exit(1);
		}
		if((connect(sock[i], (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0){
			perror("connect");
			exit(1);
		}
	}

	data_mb = data_gb*1024.0;
	data_kb = data_mb*1024.0;
	data_b = data_kb*1024.0;

	int n, pos=0;
	recv_data = 0.0;

	int flag = 0;
	int i=0, nread;

	pthread_t tid[no_conn];

	for (int q = 0; q < no_conn; ++q)
	{
		int* iptr = (int*)malloc(sizeof(int));
		*iptr = sock[q];
		pthread_create(&(tid[q]), NULL, &getdata, iptr);
	}

	while(1){
		double perc = (recv_data*100.0)/data_b;
		printf("\rData Recieved: %0.0lf/%0.0lf kB            %0.2lf %%", recv_data/(1024), data_kb, perc);
		if(perc >= 100.00)
			break;
	}

	for (int q = 0; q < no_conn; ++q)
	{
		pthread_join(tid[q], NULL);
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
	
	// while(1){
	// 	rset = allset;
	// 	n = select(maxsock + 1, &rset, NULL, NULL, NULL);
	// 	if(n>0){
	// 		if(flag == 0){
	// 	    	gettimeofday(&t1, NULL);
	// 	    	flag = 1;
	// 		}
	// 		for (int q = 0; q < no_conn; ++q)
	// 		{
	// 			if(FD_ISSET(sock[q], &rset)){		
	// 				nread = recv(sock[q], buf, 513,0);
	// 				// printf("%d %d\n", i, nread);
	// 				i++;
	// 				if(nread == 0){
	// 					close(sock[q]);
	// 					sock[q] = -1;
	// 					break;
	// 				}
	// 				if(nread < 0){
	// 					perror("conn");
	// 					break;
	// 				}
	// 				buf[nread] = '\0';
	// 				// printf("\n%s\n", buf);
	// 				strcpy(temp, buf);
	// 				pt = strtok(temp, ";");
	// 				pos = atoi(pt);
	// 				pt = strtok(NULL, ";");

	// 				fseek(fl, pos, SEEK_SET);
	// 				// printf("%d %d\n", nread, pos);
	// 				fwrite(pt, strlen(pt)+1, 1, fl);
	// 				// fprintf(fl, "%s", pt);
	// 				recv_data += (double)(strlen(pt));
	// 			}
	// 		}
	// 	}
	// 	double perc = (recv_data*100.0)/data_b;
	//   	printf("\rData Recieved: %0.0lf/%0.0lf kB            %0.2lf %%", recv_data/(1024), data_kb, perc);
	// 	if(nread < 0){
	// 		perror("conn");
	// 		break;
	// 	}
	// 	if(nread == 0){
	// 		printf("\nend\n");
	// 		gettimeofday(&t2, NULL);
	// 		elapsedTime = (t2.tv_sec - t1.tv_sec)*1000.0;
	// 		elapsedTime += (t2.tv_usec - t1.tv_usec)/1000.0;
	// 		printf("\nTotal time : %0.3f ms.\n", elapsedTime);
	// 		break;
	// 	}
	// }


	fclose(fl);
	close(par_sock);
	return 0;
}
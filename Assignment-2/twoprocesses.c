#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <unistd.h>

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

  pid_t child_pid;
  double data_gb = 1.0;
  double data_mb = data_gb*1024.0;
  double data_kb = data_mb*1024.0;
  double data_b = data_kb*1024.0;
  child_pid = fork();
  double tt = 1024*1024*1024;
  // printf("%lf\n", tt);
  if(child_pid == 0){

    char data[300], temp[300];
    int n;
    strcpy(data, "e");
    for (int i = 0; i < 8; ++i)
    {
      strcpy(temp, data);
      strcat(data, temp);
    }
    for (int i = 0; i < (data_b/256); ++i)
    {
      n = send(sock, data, strlen(data), 0);
      // double perc = ((((double)i + 1.0)*25600)/data_b);
      // printf("\rData Sent: %d/%0.0lf kB                  %lf %%", (i+1), data_kb,perc);
    }
    shutdown(sock, SHUT_WR);
    exit(0);
  }
  else{
    FILE *fl = fopen("output", "w");
    int n;
    char buf[1024];
    double recv_data = 0.0;

    struct timeval t1, t2;
    double elapsedTime;
    int flag = 0;

    while(1)
    {
      double perc = (recv_data*100.0)/data_b;
      printf("\rData Recieved: %0.0lf/%0.0lf kB            %0.2lf %%", recv_data/(1024), data_kb, perc);
      n = recv(sock, buf, 1024,0);
      if(flag == 0){
        gettimeofday(&t1, NULL);
        flag = 1;
      }
      if(n > 0){
        buf[n] = '\0';
        fprintf(fl, "%s", buf);
        fflush(fl);
        recv_data += (double)n;
      }
      else if(n == 0){
        gettimeofday(&t2, NULL);
        elapsedTime = (t2.tv_sec - t1.tv_sec)*1000.0;
        elapsedTime += (t2.tv_usec - t1.tv_usec)/1000.0;
        printf("\nTotal time : %0.3f ms.\n", elapsedTime);      

        close(sock);
        break;
      }
      else{
        exit(1);
      }
    }

    double thrg = data_mb/(elapsedTime/1000.0);
    printf("Throughput: %0.5lf MB/s\n", thrg);

    double res_time;
    res_time = elapsedTime/(data_b/256);
    printf("Response Time Per Request: %lf ms.\n", res_time);

    
    fclose(fl);
    close(sock);    
    wait(NULL);
  }

  return 0;
}
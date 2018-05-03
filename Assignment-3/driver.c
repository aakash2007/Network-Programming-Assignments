#include "nmb.h"

int main(int argc, char const *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: ./driver <your_ip> <your_port>\n");
		exit(1);
	}
	const int my_port = atoi(argv[2]);

	char my_ip[15];
	strcpy(my_ip, argv[1]);

	struct loc_msg my_iden;
	my_iden.ip_addr = inet_addr(my_ip);
	my_iden.port = (my_port);

	struct mymsg_buf * my_iden_ptr;
	my_iden_ptr = (struct mymsg_buf *)&my_iden;

	long my_mtype = my_iden_ptr->mtype;
	printf("MY TYPE: %ld\n", my_mtype);

	struct mymsg_buf msg;
	int nmbid = msgget_nmb();
	if(nmbid < 0){
		printf("Error Connecting to Local Server.....\n");
		exit(0);
	}
	printf("Connection to Local Server Succesful..\n");

	while(1){
		printf("1. Send Message?\n");
		printf("2. Recieve Message?\n");
		printf("3. Exit\n");
		printf("Enter Your Choice: ");
		int choice;
		scanf("%d", &choice);
		if(choice == 1){
			printf("\nEnter IP: ");
			char dest_ip[15];
			scanf("%s", dest_ip);
			printf("Enter Port: ");
			uint16_t dest_port;
			scanf("%hd", &dest_port);
			// printf("Enter Message:\n");
			// char dest_msg[202];
			// scanf("%s", dest_msg);

			struct loc_msg out_msg;
			out_msg.ip_addr = inet_addr(dest_ip);
			time_t t = time(0);
			out_msg.port = dest_port;
			out_msg.padding = 0;

			struct mymsg_buf temp;
			sprintf(out_msg.msg, "ABCD time is %-24.24s", ctime(&t));
			temp = *((struct mymsg_buf *)&out_msg);
			printf("\nsending: %s\n\n", out_msg.msg);
			// cnt = sendto(sock, (void *)&out_msg, sizeof(out_msg), 0, (struct sockaddr *)&addr, addrlen);
			msgsnd_nmb(nmbid, (void *)&out_msg, sizeof(out_msg));
		}
		else if(choice == 2){
			struct mymsg_buf recv_msg;
			msgrcv_nmb(nmbid, &recv_msg, sizeof(struct loc_msg), my_mtype);
			printf("received: %s\n", recv_msg.message);
		}
		else if(choice == 3){
			printf("Good Bye!\n");
			return 0;
		}
		else{
			printf("Wrong Choice!\n\n");
		}
		// sleep(2);
	}
}
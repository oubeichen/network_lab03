#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<time.h>

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 6666 /*port*/
#define LISTENNQ 8 /*maximum number of client connections*/
#define MAXLEN_CITY 29 /*max length of city name*/
#define MAXCITY_NUM 30 /*maximum number of cities*/
#define SEND_SIZE 137/*length of receiving package's data*/

int main(int argc, char **argv)
{
	int listenfd, connfd, n, citynum = 3, i;
	pid_t childpid;
	socklen_t clilen;
	unsigned char recvline[MAXLINE],sendline[MAXLINE];
	char cities[MAXCITY_NUM][MAXLEN_CITY + 1] = {"nanjing", "beijing", "shanghai"};
	struct sockaddr_in cliaddr, servaddr;
	time_t now;
	struct tm *date;

	//Create a socket for the soclet
	//If sockfd<0 there was error in the creation of the socket
	if((listenfd = socket (AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Problem in creating the socket");
		exit(1);
	}

	//preparation of the socket address
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	//bind the socket
	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	//listen to the socket by creating a connection queue, then wait for clients
	listen(listenfd, LISTENNQ);
	printf("Server running...waiting for connections.\n");

	while(1){
		clilen = sizeof(cliaddr);
		//accpet a connection
		connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);

		printf("Received request...\n");

		if((childpid = fork()) == 0){//if it's zero, it's child process
			printf("Child created for dealing with client requests.\n");
			
			//close listening socket
			close(listenfd);

			srand(time(NULL));/*Generate a random seed*/

			for(; (n = recv(connfd, recvline, MAXLINE,0)) > 32;
					send(connfd, sendline, SEND_SIZE,0), 
					memset(recvline, 0, MAXLINE),memset(sendline, 0, MAXLINE)){//valid length
				printf("Received a line.\n");

				strncpy(sendline + 2, recvline + 2, MAXLEN_CITY);
				
				//always check city, in order to some unsafe query
				printf("A client is querying city: %s.\n", sendline + 2);
				for(i = 0;i < citynum;i++){
					if(strcmp(sendline + 2, cities[i]) == 0)
						break;
				}
				if(recvline[0] == 1)//validate city
				{
					if(i == citynum)//city not found
					{
						printf("City not found.\n");
						sendline[0] = 4;
					}else{
						sendline[0] = 3;
					}

				}else if(recvline[0] == 2){//second menu
					time(&now);
					date = localtime(&now);
					sendline[32] = (date->tm_year + 1900) / 0x100;
					sendline[33] = (date->tm_year + 1900) % 0x100;
					sendline[34] = date->tm_mon + 1;
					sendline[35] = date->tm_mday;

					if(i == citynum)//city not found,reply no such weather
					{
						printf("City not found.\n");
						sendline[0] = 2;
					}else{//valid city
						if(recvline[1] == 1){//one day
							int query_day = recvline[32];
							sendline[1] = 0x41;
							//generate a random weather
							if(query_day >= 0 && query_day <= 5)//allowing 0~5
							{
								sendline[36] = recvline[32];
								sendline[37] = rand() % 5;//weather
								sendline[38] = rand() % 35;//temp 0~35
								printf("Day%d 's weather generated.\n", query_day);
								sendline[0] = 1;
							}else{//invalid query or weather not found
								printf("Weather not found.\n");
								sendline[0] = 2;
							}
						}else if(recvline[1] == 2){//three days
							sendline[1] = 0x42;
							if(recvline[32] == 3){//only 3 is allowed
								sendline[36] = 3;
								sendline[37] = rand() % 5;//weather1
								sendline[38] = rand() % 35;//temp1
								sendline[39] = rand() % 5;//weather1
								sendline[40] = rand() % 35;//temp1
								sendline[41] = rand() % 5;//weather1
								sendline[42] = rand() % 35;//temp1
								printf("Three days' weather generated.\n");
								sendline[0] = 1;
							}else{//invalid query
								sendline[0] = 2;
							}
						}else{//invalid query
							sendline[0] = 2;
						}
					}
				}else{
					sendline[0] = 5;
				}

			}

			if(n < 0)
				printf("Read error!\n");
			printf("A client disconnected..,\n");
			exit(0);
		}
		close(connfd);
	}
}


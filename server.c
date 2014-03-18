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

int main(int argc, char **argv)
{
	int listenfd, connfd, n;
	pid_t childpid;
	socklen_t clilen;
	unsigned char recvline[MAXLINE],sendline[MAXLINE];
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

			for(; (n = recv(connfd, recvline, MAXLINE,0)) > 32;

			}

			if(n < 0)
				printf("Read error!\n");
			printf("A client disconnected..,\n");
			exit(0);
		}
		close(connfd);
	}
}


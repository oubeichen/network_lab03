#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<time.h>
#include<pthread.h>

#include"message.h"

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 6566 /*port*/
#define LISTENNQ 8 /*maximum number of client connections*/
#define MAX_ONLINE 255 /*maximum number of online clients*/

#define USER_UNUSED 0
#define USER_USED 1
int usernum;

struct users{
	pthread_t thread;
	unsigned char name[20];
	unsigned char used;//whether this thread is used or not
}users[MAX_ONLINE + 1];//one for sending error message.

void *user_work(void *arg)
{
	int connfd = (int)arg, length;
	unsigned char recvline[MAXLINE],sendline[MAXLINE];
	struct msg_client_to_server *msg_recv;
	struct msg_server_to_client *msg_send;
	printf("Thread created..\n");
	while((length = recv(connfd, recvline, MAXLINE, 0)) > 0){
		memset(sendline, 0, MAXLINE);
		printf("String received from a client.\n");
		msg_recv = (struct msg_client_to_server *)recvline;
		msg_send = (struct msg_server_to_client *)sendline;
		//test begin
		msg_send->flags = MSG_ANNOUNCE;
		sprintf(msg_send->content, "%s is accepted, connfd = %d.", msg_recv->name, connfd);
		send(connfd, msg_send, length, 0);
		//testend
	}
	//close socket of the server
	close(connfd);
	
	pthread_exit(NULL);
}

int main(int argc, char **argv)
{
	int listenfd, connfd, rc;
	int i;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;

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

		printf("Received a request, creating thread...\n");
		
		if(usernum < MAX_ONLINE){
			for(i = 0;i <= MAX_ONLINE && users[i].used != USER_UNUSED;i++);//find the first unused thread
			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			rc = pthread_create(&(users[usernum].thread), &attr, user_work, (void *)connfd);
			if(rc){
				printf("ERROR: return code from pthread_create() is %d\n", rc);
			}else{
				users[i].used = USER_USED;
				usernum++;
			}
		}
		//pthread_t tt;
		//pthread_create(&tt, NULL, user_work, (void *)connfd);

	}
	pthread_exit(NULL);
}


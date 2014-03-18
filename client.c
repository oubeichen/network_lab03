#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 6666 /*server port*/

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;
	unsigned char sendline[MAXLINE],recvline[MAXLINE],buffer[MAXLINE];

	//Create a socket for the client
	//If sockfd<0 there was an error in the creation of the socket
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Problem in creating the socket.");
		exit(1);
	}

	//Creation of the socket
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	
	if(argc == 2){
	     servaddr.sin_addr.s_addr = inet_addr(argv[1]);
	}else{
	     servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	}
	
	if(argc == 3){
	     servaddr.sin_port = htons(SERV_PORT);//convert to big-endian order
	}else{
	     servaddr.sin_port = htons(atoi(argv[2]));//convert to big-endian order
	}
	//Connection of the client to the socket
	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
		perror("Problem in connecting to the server");
		exit(2);
	}
	system("clear");

	while(1){//main loop

	}
	exit(0);
}


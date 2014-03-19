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
#define SERV_PORT 6566 /*server port*/

void *recv_print(void *arg)
{
    unsigned char recvline[MAXLINE];
    struct msg_server_to_client *msg_recv;
    int sockfd = (int)arg;
    while(1)
    {
        if(recv(sockfd, recvline, MAXLINE, 0) == 0){
            perror("The server terminated prematurely.\n");
            exit(3);
        }
        msg_recv = (struct msg_server_to_client *)recvline;
        printf("Received a line from server: %s\n", msg_recv->content);
    }
    pthread_exit(NULL);
} 

int main(int argc, char **argv)
{
    pthread_t recv_thread;
    int sockfd;
    struct sockaddr_in servaddr;
    unsigned char sendline[MAXLINE],buf[MAXLINE],name[MSG_MAX_NAME_LENGTH + 1];
    struct msg_client_to_server *msg_send;

    srand((unsigned)time(NULL));//generate a random seed

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
         servaddr.sin_port = htons(atoi(argv[2]));//convert to big-endian order
    }else{
         servaddr.sin_port = htons(SERV_PORT);//convert to big-endian order
    }
    //Connection of the client to the socket
    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("Problem in connecting to the server");
        exit(2);
    }
    system("clear");

    while(1){//main loop
        printf("Welcome to the chatroom of oubeichen!\n");
        printf("Please enter a name(leave empty to get a random name):\n");
        fgets(buf, MAXLINE, stdin);
        if(buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';
        strncpy(name, buf, MSG_MAX_NAME_LENGTH + 1);
        if(strlen(name) == 0){
            sprintf(name, "user%d", rand());
        }
        printf("Your name is:%s \n", name);
        msg_send = (struct msg_client_to_server *)sendline;
        msg_send->flags = 4;//login
        strncpy(msg_send->name, name, MSG_MAX_NAME_LENGTH + 1);
        strncpy(msg_send->content, "Login message.", MSG_MAX_NAME_LENGTH);
        send(sockfd, sendline, MSG_CLI_SRV_LENGTH, 0);

        pthread_create(&recv_thread, NULL, recv_print, (void *)sockfd);
        while(1)//chat loop
        {
            fgets(buf, MAXLINE, stdin);
            printf("list\n");
            memset(sendline, 0, MAXLINE);
            msg_send->flags = MSG_LIST;
            send(sockfd, sendline, MSG_CLI_SRV_LENGTH, 0);
        }
    }
    exit(0);
}


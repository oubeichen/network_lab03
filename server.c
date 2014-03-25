#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<time.h>
#include<pthread.h>

#include"protocol.h"

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 6566 /*port*/
#define LISTENNQ 8 /*maximum number of client connections*/

#define USER_UNUSED 0
#define USER_USED 1

int usernum;
struct users{
    pthread_t recv_thread;//receive messages from client and work
    pthread_t send_thread;//send message to client initiatively
    unsigned char name[MSG_MAX_NAME_LENGTH + 1];
    unsigned char used;//whether this thread is used or not
    struct msg_server_to_client_not_list msg;
    pthread_mutex_t msg_mutex;
    pthread_cond_t msg_cond;
}users[MAX_ONLINE + 1];//one for sending error message.

struct thread_data{
    int connfd;
    int threadnum;
};

void *recv_thread_work(void *arg)
{
    struct thread_data *mydata = (struct thread_data *)arg;
    int connfd = mydata->connfd, threadnum = mydata->threadnum, i;
    struct users *myuser = &users[threadnum];
    unsigned char recvline[MAXLINE],sendline[MAXLINE];
    struct msg_client_to_server *msg_recv = (struct msg_client_to_server *)recvline;
    struct msg_server_to_client *msg_send = (struct msg_server_to_client *)sendline;
    //printf("Thread created..\n");
    while(recv(connfd, recvline, MAXLINE, 0) == MSG_CLI_SRV_LENGTH){
        memset(sendline, 0, MAXLINE);
        if(msg_recv->flags == MSG_LOGIN){
            printf("%s is attempting to login.\n", msg_recv->name);
            for(i = 0;i < MAX_ONLINE;i++){
                if(users[i].used == USER_USED && strcmp(msg_recv->name, users[i].name) == 0)
                    break;
            }
            if(i == MAX_ONLINE){
                msg_send->flags = MSG_LOGIN_SUCCEED;
                //need lock
                strncpy(myuser->name, msg_recv->name, MSG_MAX_NAME_LENGTH + 1);
                sprintf(msg_send->content, "%s is accepted.", myuser->name);
                printf("%s loged in.\n", myuser->name);
                //need unlock
            }else{
                msg_send->flags = MSG_LOGIN_FAILED;
                sprintf(msg_send->content, "%s is already used.", msg_recv->name);
                printf("%s is already used.\n", msg_recv->name);
            }
            send(connfd, sendline, MSG_CLI_SRV_LENGTH, 0);
        }
        if(msg_recv->flags == MSG_EVERYONE){
            printf("%s send a message to everyone.\n",myuser->name);
            strncpy(msg_recv->name, myuser->name, MSG_MAX_NAME_LENGTH);
            for(i = 0;i <= MAX_ONLINE;i++){
                //need lock
                if(users[i].used == USER_USED){
                    pthread_mutex_lock(&users[i].msg_mutex);
                    memcpy(&users[i].msg, &recvline, MSG_CLI_SRV_LENGTH);
                    pthread_cond_signal(&users[i].msg_cond);
                    pthread_mutex_unlock(&users[i].msg_mutex);
                    //usleep(100);
                }
                //need unlock
            }
        }
        if(msg_recv->flags == MSG_SPECFIC){
            printf("%s send a message to %s\n", myuser->name, msg_recv->name);
            for(i = 0;i < MAX_ONLINE;i++){
                if(users[i].used == USER_USED && (strcmp(users[i].name, msg_recv->name) == 0)){
                    pthread_mutex_lock(&users[i].msg_mutex);
                    memcpy(&users[i].msg, &recvline, MSG_CLI_SRV_LENGTH);
                    strncpy(users[i].msg.name, myuser->name, MSG_MAX_NAME_LENGTH);
                    pthread_cond_signal(&users[i].msg_cond);
                    pthread_mutex_unlock(&users[i].msg_mutex);
                    break;
                }
            }
            if(i == MAX_ONLINE){
                msg_send->flags = MSG_ANNOUNCE;
                sprintf(msg_send->content,"User %s not found!", msg_recv->name);
                printf("User %s not found!\n", msg_recv->name);
                send(connfd, sendline, MSG_CLI_SRV_LENGTH, 0);
            }else{
                msg_send->flags = MSG_SPECFIC_REPLY;
                strncpy(msg_send->name, msg_recv->name, MSG_MAX_NAME_LENGTH);
                strncpy(msg_send->content, msg_recv->content, MSG_MAX_CONTENT_LENGTH);
                send(connfd, sendline, MSG_CLI_SRV_LENGTH, 0);
            }
        }
        if(msg_recv->flags == MSG_LIST){
            int usernum;
            unsigned char (*listp)[MSG_MAX_NAME_LENGTH + 1] = msg_send->list;
            msg_send->flags = MSG_LIST;
            //need lock
            printf("%s wants the online user list.\n", myuser->name);
            for(i = 0, usernum = 0;i < MAX_ONLINE;i++){
                if(users[i].used == USER_USED){
                    strncpy(listp[usernum++], users[i].name, MSG_MAX_NAME_LENGTH);
                }
            }
            msg_send->name[0] = usernum;
            //need unlock
            send(connfd, sendline, 1 + (MSG_MAX_NAME_LENGTH + 1) + usernum * (MSG_MAX_NAME_LENGTH + 1), 0);
        }
    }
    //close socket of the server
    printf("%s loged out.\n", myuser->name);
    close(connfd);

    //exit send_thread
    pthread_mutex_lock(&myuser->msg_mutex);
    myuser->msg.flags = MSG_LOGOUT;
    pthread_cond_signal(&myuser->msg_cond);
    pthread_mutex_unlock(&myuser->msg_mutex);
    
    //need lock
    memset(users[threadnum].name, 0, MSG_MAX_NAME_LENGTH);
    users[threadnum].used = USER_UNUSED;
    //need unlock
    
    pthread_exit(NULL);
}

void *send_thread_work(void *arg)
{
    struct thread_data *mydata = (struct thread_data *)arg;
    int connfd = mydata->connfd, threadnum = mydata->threadnum, testnum = 0;
    struct users *myuser = &users[threadnum];
    unsigned char sendline[MAXLINE];
    struct msg_server_to_client *msg_send = (struct msg_server_to_client *)sendline;

    pthread_mutex_lock(&myuser->msg_mutex);
    while(1){
        pthread_cond_wait(&myuser->msg_cond, &myuser->msg_mutex);
        if(myuser->msg.flags == MSG_EVERYONE){
            msg_send->flags = MSG_EVERYONE;
            strncpy(msg_send->name, myuser->msg.name, MSG_MAX_NAME_LENGTH);
        }else if(myuser->msg.flags == MSG_SPECFIC){
            msg_send->flags = MSG_SPECFIC;
            strncpy(msg_send->name, myuser->msg.name, MSG_MAX_NAME_LENGTH);
        }else if(myuser->msg.flags == MSG_LOGOUT){
            break;
        }
        strncpy(msg_send->content, myuser->msg.content, MSG_MAX_CONTENT_LENGTH);
        send(connfd, sendline, MSG_CLI_SRV_LENGTH, 0);
    }
    pthread_mutex_unlock(&myuser->msg_mutex);
    pthread_mutex_destroy(&myuser->msg_mutex);
    pthread_cond_destroy(&myuser->msg_cond);

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    pthread_attr_t attr;
    int listenfd, connfd, rc, rc1;
    int i;
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    struct thread_data thread_dt[MAX_ONLINE + 1];

    /*Initialize and set thread detached attribute */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
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

        printf("Received a request,connfd=%d creating thread...\n", connfd);

        if(usernum < MAX_ONLINE){
            for(i = 0;i <= MAX_ONLINE && users[i].used != USER_UNUSED;i++);//find the first unused thread
            /*Initial thread objects*/
            thread_dt[i].connfd = connfd;
            thread_dt[i].threadnum = i;
            pthread_mutex_init(&users[i].msg_mutex, NULL); 
            pthread_cond_init(&users[i].msg_cond, NULL);


            rc = pthread_create(&(users[i].recv_thread), &attr, recv_thread_work, (void *)&thread_dt[i]);
            rc1 = pthread_create(&(users[i].send_thread), &attr, send_thread_work, (void *)&thread_dt[i]);
            if(rc){
                printf("ERROR: return code from pthread_create() is %d\n", rc);
            }else{
                users[i].used = USER_USED;
                usernum++;
            }
        }

    }
    pthread_exit(NULL);
}


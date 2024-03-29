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
    unsigned char status;//whether this thread is used or not
    struct msg_server_to_client_not_list msg;
    pthread_mutex_t name_mutex;
    pthread_mutex_t msg_mutex;
    pthread_cond_t msg_cond;
}users[MAX_ONLINE + 1];//one for sending error message.

pthread_mutex_t users_status_mutex;

struct thread_data{
    int connfd;
    int threadnum;
};

void *recv_thread_work(void *arg)//receive and process messages
{
    struct thread_data *mydata = (struct thread_data *)arg;
    int connfd = mydata->connfd, threadnum = mydata->threadnum, i;
    struct users *myuser = &users[threadnum];
    unsigned char recvline[MAXLINE], sendline[MAXLINE], myname[MSG_MAX_NAME_LENGTH + 1];
    struct msg_client_to_server *msg_recv = (struct msg_client_to_server *)recvline;
    struct msg_server_to_client *msg_send = (struct msg_server_to_client *)sendline;

    //printf("Thread created..\n");
    while(recv(connfd, recvline, MAXLINE, 0) == MSG_CLI_SRV_LENGTH){
        memset(sendline, 0, MAXLINE);
        if(msg_recv->flags == MSG_LOGIN){
            printf("%s is attempting to login.\n", msg_recv->name);
            for(i = 0;i < MAX_ONLINE;i++){
                pthread_mutex_lock(&users_status_mutex);
                pthread_mutex_lock(&users[i].name_mutex);
                if(i != threadnum && users[i].status == USER_USED && strcmp(msg_recv->name, users[i].name) == 0){
                    pthread_mutex_unlock(&users[i].name_mutex);//unlock before break
                    pthread_mutex_unlock(&users_status_mutex);
                    break;
                }
                pthread_mutex_unlock(&users[i].name_mutex);
                pthread_mutex_unlock(&users_status_mutex);
            }
            if(i == MAX_ONLINE){
                msg_send->flags = MSG_LOGIN_SUCCEED;
                pthread_mutex_lock(&myuser->name_mutex);
                strncpy(myuser->name, msg_recv->name, MSG_MAX_NAME_LENGTH + 1);
                strncpy(myname, myuser->name, MSG_MAX_NAME_LENGTH);
                pthread_mutex_unlock(&myuser->name_mutex);
 
                sprintf(msg_send->content, "%s is accepted.", myname);
                printf("%s logged in.\n", myname);

                //send announcement to everyone
                for(i = 0;i < MAX_ONLINE;i++){
                    pthread_mutex_lock(&users_status_mutex);
                    if(i != threadnum && users[i].status == USER_USED){
                        pthread_mutex_lock(&users[i].msg_mutex);
                        users[i].msg.flags = MSG_ANNOUNCE;
                        sprintf(users[i].msg.content, "%s logged in.", myname);
                        pthread_cond_signal(&users[i].msg_cond);
                        pthread_mutex_unlock(&users[i].msg_mutex);
                        //usleep(100);
                    }
                    pthread_mutex_unlock(&users_status_mutex);
                }
            }else{
                msg_send->flags = MSG_LOGIN_FAILED;
                sprintf(msg_send->content, "%s is already used.", msg_recv->name);
                printf("%s is already used.\n", msg_recv->name);
            }
            send(connfd, sendline, MSG_CLI_SRV_LENGTH, 0);
        }
        if(msg_recv->flags == MSG_EVERYONE){
            printf("%s sends a message to everyone.\n",myname);
            strncpy(msg_recv->name, myname, MSG_MAX_NAME_LENGTH);
            for(i = 0;i <= MAX_ONLINE;i++){
                pthread_mutex_lock(&users_status_mutex);
                if(users[i].status == USER_USED){
                    pthread_mutex_lock(&users[i].msg_mutex);
                    memcpy(&users[i].msg, &recvline, MSG_CLI_SRV_LENGTH);
                    pthread_cond_signal(&users[i].msg_cond);
                    pthread_mutex_unlock(&users[i].msg_mutex);
                    //usleep(100);
                }
                pthread_mutex_unlock(&users_status_mutex);
            }
        }
        if(msg_recv->flags == MSG_SPECIFIC){
            printf("%s sends a message to %s.\n", myname, msg_recv->name);
            for(i = 0;i < MAX_ONLINE;i++){
                pthread_mutex_lock(&users_status_mutex);
                pthread_mutex_lock(&users[i].name_mutex);
                if(users[i].status == USER_USED && (strcmp(users[i].name, msg_recv->name) == 0)){
                    pthread_mutex_lock(&users[i].msg_mutex);
                    memcpy(&users[i].msg, &recvline, MSG_CLI_SRV_LENGTH);
                    strncpy(users[i].msg.name, myname, MSG_MAX_NAME_LENGTH);
                    pthread_cond_signal(&users[i].msg_cond);
                    pthread_mutex_unlock(&users[i].msg_mutex);
 
                    pthread_mutex_unlock(&users[i].name_mutex);//unlock before break
                    pthread_mutex_unlock(&users_status_mutex);
                    break;
                }
                pthread_mutex_unlock(&users[i].name_mutex);
                pthread_mutex_unlock(&users_status_mutex);
            }
            if(i == MAX_ONLINE){
                msg_send->flags = MSG_ANNOUNCE;
                sprintf(msg_send->content,"User %s not found!", msg_recv->name);
                printf("User %s not found!\n", msg_recv->name);
                send(connfd, sendline, MSG_CLI_SRV_LENGTH, 0);
            }else if(i != threadnum){
                msg_send->flags = MSG_SPECIFIC_REPLY;
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
            printf("%s wants the online user list.\n", myname);
            for(i = 0, usernum = 0;i < MAX_ONLINE;i++){
                pthread_mutex_lock(&users_status_mutex);
                pthread_mutex_lock(&users[i].name_mutex);
                if(users[i].status == USER_USED){
                    strncpy(listp[usernum++], users[i].name, MSG_MAX_NAME_LENGTH);
                }
                pthread_mutex_unlock(&users[i].name_mutex);
                pthread_mutex_unlock(&users_status_mutex);
            }
            msg_send->name[0] = usernum;
            //need unlock
            send(connfd, sendline, 1 + (MSG_MAX_NAME_LENGTH + 1) + usernum * (MSG_MAX_NAME_LENGTH + 1), 0);
        }
    }
    //close socket of the server
    printf("%s logged out.\n", myname);
    close(connfd);

    //exit send_thread
    pthread_mutex_lock(&myuser->msg_mutex);
    myuser->msg.flags = MSG_LOGOUT;
    pthread_cond_signal(&myuser->msg_cond);
    pthread_mutex_unlock(&myuser->msg_mutex);

    pthread_mutex_lock(&users_status_mutex);
    myuser->status = USER_UNUSED;
    pthread_mutex_unlock(&users_status_mutex);

    pthread_mutex_lock(&myuser->name_mutex);
    memset(myuser->name, 0, MSG_MAX_NAME_LENGTH);
    pthread_mutex_unlock(&myuser->name_mutex);

    //send announcement to everyone
    for(i = 0;i < MAX_ONLINE;i++){
        pthread_mutex_lock(&users_status_mutex);
        if(i != threadnum && users[i].status == USER_USED){
            pthread_mutex_lock(&users[i].msg_mutex);
            users[i].msg.flags = MSG_ANNOUNCE;
            sprintf(users[i].msg.content, "%s logged out.", myname);
            pthread_cond_signal(&users[i].msg_cond);
            pthread_mutex_unlock(&users[i].msg_mutex);
            //usleep(100);
        }
        pthread_mutex_unlock(&users_status_mutex);
    }

    pthread_exit(NULL);
}

void *send_thread_work(void *arg)//send message from another user
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
        }else if(myuser->msg.flags == MSG_SPECIFIC){
            msg_send->flags = MSG_SPECIFIC;
            strncpy(msg_send->name, myuser->msg.name, MSG_MAX_NAME_LENGTH);
        }else if(myuser->msg.flags == MSG_ANNOUNCE){
            msg_send->flags = MSG_ANNOUNCE;
        }else if(myuser->msg.flags == MSG_LOGOUT){
            break;
        }
        strncpy(msg_send->content, myuser->msg.content, MSG_MAX_CONTENT_LENGTH);
        send(connfd, sendline, MSG_CLI_SRV_LENGTH, 0);
        memset(sendline, 0 ,MAXLINE);
    }
    pthread_mutex_unlock(&myuser->msg_mutex);
    pthread_mutex_destroy(&myuser->msg_mutex);
    pthread_mutex_destroy(&myuser->name_mutex);
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

    pthread_mutex_init(&users_status_mutex, NULL);
    while(1){
        clilen = sizeof(cliaddr);
        //accpet a connection
        connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);

        printf("Received a request,connfd=%d creating thread...\n", connfd);

        if(usernum < MAX_ONLINE){
            pthread_mutex_lock(&users_status_mutex);
            for(i = 0;i <= MAX_ONLINE && users[i].status != USER_UNUSED;i++);//find the first unused thread
            pthread_mutex_unlock(&users_status_mutex);

            /*Initial thread objects*/
            thread_dt[i].connfd = connfd;
            thread_dt[i].threadnum = i;
            pthread_mutex_init(&users[i].msg_mutex, NULL);
            pthread_mutex_init(&users[i].name_mutex, NULL);
            pthread_cond_init(&users[i].msg_cond, NULL);

            rc = pthread_create(&(users[i].recv_thread), &attr, recv_thread_work, (void *)&thread_dt[i]);
            rc1 = pthread_create(&(users[i].send_thread), &attr, send_thread_work, (void *)&thread_dt[i]);
            if(rc){
                printf("ERROR: return code from pthread_create() is %d\n", rc);
            }else{
                pthread_mutex_lock(&users_status_mutex);
                users[i].status = USER_USED;
                pthread_mutex_unlock(&users_status_mutex);
                usernum++;
            }
        }

    }
    pthread_exit(NULL);
}


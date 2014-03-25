#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<time.h>
#include<pthread.h>
#include<panel.h>

#include"protocol.h"

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 6566 /*server port*/

struct thread_data{
    int sockfd;
    WINDOW **wins;
};

unsigned char name[MSG_MAX_NAME_LENGTH];

void init_wins(WINDOW **);
void print_label(WINDOW **);
void *recv_print(void *);
void *send_input(void *);
void waiting_for_input(WINDOW **, char *);

int main(int argc, char **argv)
{
    WINDOW *my_wins[2];
    PANEL  *my_panels[2];
    PANEL  *top;
    pthread_t recv_thread,send_thread;
    pthread_attr_t attr;
    struct thread_data tdata;
    int sockfd, rc, second_launch = 0;
    void *status;
    struct sockaddr_in servaddr;
    unsigned char sendline[MAXLINE], recvline[MAXLINE], buf[MAXLINE];
    struct msg_client_to_server *msg_send;
    struct msg_server_to_client *msg_recv;

    srand((unsigned)time(NULL));//generate a random seed

    /* Setup for windows begin*/
    /* Initialize curses */
    initscr();
    start_color();

    /* Initialize all the colors */
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);

    init_wins(my_wins);
    
    /* Attach a panel to each window */ 
    my_panels[0] = new_panel(my_wins[0]);
    my_panels[1] = new_panel(my_wins[1]);

    /* Update the stacking order. */
    update_panels();

    /* Show it on the screen */
    attron(COLOR_PAIR(4));
    attroff(COLOR_PAIR(4));
    doupdate();
    refresh();

    /* Setup for chatting */
    wsetscrreg(my_wins[0], 0, LINES - 5);
    scrollok(my_wins[0], TRUE);
    /* Setup for windows end*/

    while(1){//main loop
        //inital input window
        box(my_wins[1], 0, 0);
        wattron(my_wins[1], COLOR_PAIR(2));
        strcpy(buf, "Welcome to oubeichen's chatroom.Please enter a name.");
        mvwprintw(my_wins[1], 0, (COLS - strlen(buf)) / 2, buf);
        wattroff(my_wins[1], COLOR_PAIR(2));
        //refresh();
        update_panels();
        doupdate();

        //Input a name to login
        waiting_for_input(my_wins, buf);
        
        wattron(my_wins[0], COLOR_PAIR(2));

        //Create a socket for the client
        //If sockfd<0 there was an error in the creation of the socket
        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            wprintw(my_wins[0], "Problem in creating the socket.\n");
            continue;
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
            wprintw(my_wins[0], "Problem in connecting to the server\n");
            continue;
        }

        //Login
        if(buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';
        strncpy(name, buf, MSG_MAX_NAME_LENGTH + 1);
        if(strlen(name) == 0){
            sprintf(name, "user%d", rand());
        }
        wprintw(my_wins[0], "Your name is:%s \n", name);
        msg_send = (struct msg_client_to_server *)sendline;
        msg_send->flags = MSG_LOGIN;//login
        strncpy(msg_send->name, name, MSG_MAX_NAME_LENGTH + 1);
        strncpy(msg_send->content, "Login message.", MSG_MAX_NAME_LENGTH);
        send(sockfd, sendline, MSG_CLI_SRV_LENGTH, 0);

        msg_recv = (struct msg_server_to_client *)recvline;
        if(recv(sockfd, recvline, MAXLINE, 0) == 0){
            wprintw(my_wins[0], "Login failed due to network.\n");
            continue;
        }
        if(msg_recv->flags == MSG_LOGIN_SUCCEED){
            wprintw(my_wins[0], "Login succeed:%s\n", msg_recv->content);
        }else if(msg_recv->flags == MSG_LOGIN_FAILED){
            wprintw(my_wins[0], "Login failed:%s\n", msg_recv->content);
            continue;
        }else{
            wprintw(my_wins[0], "Unknown issues during login:%s\n", msg_recv->content);
            continue;
        }
        update_panels();
        doupdate();

        wattroff(my_wins[0], COLOR_PAIR(2));

        //create two thread for sending and receiving
        tdata.sockfd = sockfd;
        tdata.wins = my_wins;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&recv_thread, &attr, recv_print, (void *)&tdata);
        pthread_create(&send_thread, &attr, send_input, (void *)&tdata);

        pthread_attr_destroy(&attr);
        rc = pthread_join(recv_thread, &status);
        if(rc){
            wprintf("ERROR: return code from pthread_join() is %d\n", rc);
        }
        pthread_cancel(send_thread);//avoid harmful issues
        //pthread_join(send_thread, &status);
        wprintw(my_wins[0], "Logout successful.\n");
        update_panels();
        doupdate();
    }
    endwin();
    pthread_exit(NULL);
}
/* Put all the windows */
void init_wins(WINDOW **wins)
{
    wins[0] = newwin(LINES - 4, COLS, 0, 0);
    wattron(wins[0], COLOR_PAIR(1));
    wprintw(wins[0], "Help: \n\":list\" \t\tList online users.\n\":logout\" \t\tLog out.\n\"[someone] some_message\" \tSend a private message to someone.\n");
    wattroff(wins[0], COLOR_PAIR(1));
    
    wins[1] = newwin(4, COLS, LINES - 5, 0);
}

void print_label(WINDOW **wins)
{
    box(wins[1], 0, 0);
    wattron(wins[1], COLOR_PAIR(2));
    mvwprintw(wins[1], 0, (COLS - 5) / 2, "Input");
    wattroff(wins[1], COLOR_PAIR(2));
}

void *recv_print(void *arg)
{
    unsigned char recvline[MAXLINE];
    struct msg_server_to_client *msg_recv;
    int sockfd = ((struct thread_data *)arg)->sockfd, i;
    WINDOW **wins = ((struct thread_data *)arg)->wins;

    msg_recv = (struct msg_server_to_client *)recvline;
    while(1)
    {
        if(recv(sockfd, recvline, MAXLINE, 0) == 0){
            break;
        }
        if(msg_recv->flags == MSG_EVERYONE){
            waddstr(wins[0], msg_recv->name);
            waddstr(wins[0], " : ");
            waddstr(wins[0], msg_recv->content);
        }else if(msg_recv->flags == MSG_SPECFIC){
            wattron(wins[0], COLOR_PAIR(3));
            wprintw(wins[0], "%s : [%s]%s", msg_recv->name, name, msg_recv->content);
            wattroff(wins[0], COLOR_PAIR(3));
        }else if(msg_recv->flags == MSG_SPECFIC_REPLY){
            wattron(wins[0], COLOR_PAIR(3));
            wprintw(wins[0], "%s : [%s]%s", name, msg_recv->name, msg_recv->content);
            wattroff(wins[0], COLOR_PAIR(3));
        }else if(msg_recv->flags == MSG_ANNOUNCE){
            wattron(wins[0], COLOR_PAIR(2));
            waddstr(wins[0], msg_recv->content);
            wattroff(wins[0], COLOR_PAIR(2));
        }else if(msg_recv->flags == MSG_LIST){
            wattron(wins[0], COLOR_PAIR(4));
            waddstr(wins[0], "Online users are: ");
            wattroff(wins[0], COLOR_PAIR(4));
            waddstr(wins[0], "|");
            wattron(wins[0], COLOR_PAIR(4));
            for(i = 0;i < msg_recv->name[0];i++){
                waddstr(wins[0], msg_recv->list[i]);
                wattroff(wins[0], COLOR_PAIR(4));
                waddstr(wins[0], "|");
                wattron(wins[0], COLOR_PAIR(4));
            }
            wattroff(wins[0], COLOR_PAIR(4));
        }
        waddch(wins[0], '\n');
        update_panels();
        doupdate();
    }
    pthread_exit(NULL);
}

void *send_input(void *arg)
{    
    unsigned char sendline[MAXLINE], buf[MAXLINE];
    struct msg_client_to_server *msg_send;
    int sockfd = ((struct thread_data *)arg)->sockfd;
    WINDOW **wins = ((struct thread_data *)arg)->wins;
    
    msg_send = (struct msg_client_to_server *)sendline;
    while(1)//chat loop
    {
        waiting_for_input(wins, buf);
        //wprintw(wins[0], "list\n");
        if(buf[0] == '['){
            int i;
            for(i = 1;i < MSG_MAX_NAME_LENGTH + 1 && buf[i] != ']';i++){
                msg_send->name[i - 1] = buf[i];
            }
            msg_send->name[i - 1] = '\0';//avoid issue
            if(buf[i] == ']'){// if not then use later words as content
                i++;
            }
            strncpy(msg_send->content, buf + i, MSG_MAX_CONTENT_LENGTH);
            msg_send->flags = MSG_SPECFIC;
        }else if(buf[0] == ':' && strcmp(buf + 1, "list") == 0){
            msg_send->flags = MSG_LIST;
        }else if(buf[0] == ':' && strcmp(buf + 1, "logout") == 0){
            break;
        }else{
            memset(sendline, 0, MAXLINE);
            msg_send->flags = MSG_EVERYONE;
            strncpy(msg_send->content, buf, MSG_MAX_CONTENT_LENGTH);
        }
        if(send(sockfd, sendline, MSG_CLI_SRV_LENGTH, 0) < MSG_CLI_SRV_LENGTH){
            wprintw(wins[0], "Sending error.\n");
            break;
        }
        update_panels();
        doupdate();
    }
    shutdown(sockfd, SHUT_RDWR);
    pthread_exit(NULL);
}

void waiting_for_input(WINDOW **wins, char *buf)
{
    wmove(wins[1], 1, 1);
    wgetstr(wins[1], buf);
    //wprintw(wins[0], "%s\n", buf);
    werase(wins[1]);
    print_label(wins);
    update_panels();
    doupdate();
}


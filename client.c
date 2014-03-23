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
    int sockfd, rc;
    void *status;
    struct sockaddr_in servaddr;
    unsigned char sendline[MAXLINE],buf[MAXLINE],name[MSG_MAX_NAME_LENGTH + 1];
    struct msg_client_to_server *msg_send;

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
        waiting_for_input(my_wins, buf);
        
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
        update_panels();
        doupdate();

        msg_send = (struct msg_client_to_server *)sendline;
        msg_send->flags = 4;//login
        strncpy(msg_send->name, name, MSG_MAX_NAME_LENGTH + 1);
        strncpy(msg_send->content, "Login message.", MSG_MAX_NAME_LENGTH);
        send(sockfd, sendline, MSG_CLI_SRV_LENGTH, 0);
        
        tdata.sockfd = sockfd;
        tdata.wins = my_wins;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&recv_thread, &attr, recv_print, (void *)&tdata);
        pthread_create(&send_thread, &attr, send_input, (void *)&tdata);

        pthread_attr_destroy(&attr);
        rc = pthread_join(send_thread, &status);
        if(rc){
            wprintf("ERROR: return code from pthread_join() is %d\n", rc);
        }
        wprintf("Logout successful.\n");
    }
    endwin();
    pthread_exit(NULL);
}
/* Put all the windows */
void init_wins(WINDOW **wins)
{
    char str[90];
    wins[0] = newwin(LINES - 4, COLS, 0, 0);
    wattron(wins[0], COLOR_PAIR(1));
    wprintw(wins[0], "Help: \n\":list\" \t\tList online users.\n\":logout\" \t\tLog out.\n\"@someone <message>\" \tSend a private message to someone.\n");
    wattroff(wins[0], COLOR_PAIR(1));
    
    wins[1] = newwin(4, COLS, LINES - 5, 0);
    box(wins[1], 0, 0);
    wattron(wins[1], COLOR_PAIR(2));

    strcpy(str, "Welcome to oubeichen's chatroom.Please enter a name.");
    mvwprintw(wins[1], 0, (COLS - strlen(str)) / 2, str);
    wattroff(wins[1], COLOR_PAIR(2));
    refresh();
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
    int sockfd = ((struct thread_data *)arg)->sockfd;
    WINDOW **wins = ((struct thread_data *)arg)->wins;

    msg_recv = (struct msg_server_to_client *)recvline;
    while(1)
    {
        if(recv(sockfd, recvline, MAXLINE, 0) == 0){
            perror("The server terminated prematurely.\n");
            exit(3);
        }
        wprintw(wins[0], "Received a line from server: ");
        waddstr(wins[0], msg_recv->content);
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
        memset(sendline, 0, MAXLINE);
        msg_send->flags = MSG_LIST;
        send(sockfd, sendline, MSG_CLI_SRV_LENGTH, 0);
    }
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


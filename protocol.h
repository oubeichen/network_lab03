/**
 * This headfile is to define the message format between clients and servers. 
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define MSG_MAX_CONTENT_LENGTH 255
#define MSG_MAX_NAME_LENGTH 20

#define MAX_ONLINE 150 /*maximum number of online clients*/

/*flags of both client_to_server and server_to_client message*/
#define MSG_EVERYONE    0
#define MSG_SPECFIC     1
#define MSG_LIST        2
#define MSG_ANNOUNCE    3 //only for server_to_client
#define MSG_LOGIN       4 //only for client_to_server

#define MSG_CLI_SRV_LENGTH 277
struct msg_client_to_server{
    unsigned char flags;//now only one flag, will adjust the size if flags are extended.
    unsigned char name[MSG_MAX_NAME_LENGTH + 1]; /*send_to or login_name*/
    unsigned char content[MSG_MAX_CONTENT_LENGTH + 1];
};

struct msg_server_to_client{
    unsigned char flags;//now only one flag.
    unsigned char name[MSG_MAX_NAME_LENGTH + 1]; /*send_to or online_user_num for the first char(byte).*/
    union{
        unsigned char content[MSG_MAX_CONTENT_LENGTH + 1];
        unsigned char list[MAX_ONLINE][MSG_MAX_NAME_LENGTH + 1];
    };
};

struct msg_server_to_client_not_list{
    unsigned char flags;//now only one flag.
    unsigned char name[MSG_MAX_NAME_LENGTH + 1]; /*send_to or online_user_num for the first char(byte).*/
    unsigned char content[MSG_MAX_CONTENT_LENGTH + 1];
};
#endif

/**
 * This headfile is to define the message format between clients and servers. 
 */

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define MAX_CONTENT_LENGTH 255

/*flags of both client_to_server and server_to_client message*/
#define MSG_EVERYONE	0
#define MSG_SPECFIC	1
#define MSG_LIST	2
#define MSG_ANNOUCE	3

#define MSG_CLI_SRV_LENGTH 277
struct msg_client_to_server{
	unsigned char flags;//now only one flag, will adjust the size if flags are extended.
	unsigned char name[20]; /*send_to or login_name*/
	unsigned char content[MAX_CONTENT_LENGTH + 1];
};

#define MSG_SRV_CLI_LENGTH 
struct msg_server_to_client{
	unsigned char flags;//now only one flag.
	unsigned char name[20]; /*send_to or online_user_num for the first char(byte).*/
	union{
		unsigned char content[MAX_CONTENT_LENGTH + 1];
		unsigned char **list;
	}
}

#endif

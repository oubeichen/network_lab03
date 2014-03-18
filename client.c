#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>

#define MAXLINE 4096 /*max text line length*/
#define SERV_PORT 6666 /*server port*/
#define MAXLEN_CITY 29/*max length of city name*/
#define SEND_SIZE 33 /*length of sending package's data*/

/*receive and send packages*/
void send_recv(unsigned char *sendline, unsigned char *recvline, int sockfd)
{
	send(sockfd, sendline, SEND_SIZE, 0);
	if(recv(sockfd, recvline, MAXLINE, 0) == 0){
		perror("The server terminated prematurely");
		exit(3);
	}
}
/*parse and print received messages*/
void output(unsigned char *recvline)
{
	char city[MAXLEN_CITY + 1], tenki[][7] = {"shower", "clear", "cloudy", "rain", "fog"};
	//printf("sample output\n");
	strncpy(city, recvline + 2, MAXLEN_CITY);
	//query successful
	if(recvline[0] == 1){
		printf("City: %s  ", city);
		printf("Today is: %d/%02d/%02d  ", recvline[32] * 0x100 + recvline[33], recvline[34], recvline[35]);
		printf("Weather information is as follows: \n");
		
		//one day
		if(recvline[1] == 0x41){
			//today,2nd day,3rd day,4th day
			if(recvline[36] == 1){
				printf("Today's ");
			}else if(recvline[36] == 2){
				printf("The 2nd day's ");
			}else if(recvline[36] == 3){
				printf("The 3rd day's ");
			}else if(recvline[36] <= 9 && recvline[36] >= 4){
				printf("The %dth day's ", recvline[36]);
			}else{
				printf("Server error!(date)\n");
				return;
			}

			//Weather
			if(recvline[37] >= 0 && recvline[37] <= 4){
				printf("Weather is: %s;  ", tenki[recvline[37]]);
			}else{
				printf("Server error!(weather)\n");
			}

			//Tempature
			printf("Temp:%02d\n", recvline[38]);
		}
		//three days
		else if(recvline[1] == 0x42){
			//debug server
			if(recvline[36] != 3 || recvline[37] < 0 || recvline[37] > 4
					|| recvline[39] < 0 || recvline[39] > 4
					|| recvline[41] < 0 || recvline[41] > 4){
				printf("Server error!\n");
			}
			printf("The 1st day's Weather is: %s;  Temp:%02d\n", tenki[recvline[37]], recvline[38]);
			printf("The 2nd day's Weather is: %s;  Temp:%02d\n", tenki[recvline[39]], recvline[40]);
			printf("The 3rd day's Weather is: %s;  Temp:%02d\n", tenki[recvline[41]], recvline[42]);
		}else{
			printf("Server error!\n");
		}
	}
	//no such info
	else if(recvline[0] == 2){
		printf("Sorry, no given day's weather information for city %s!\n", city);
	}
	else{
		printf("Server error!\n");
		return;
	}
}

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
	      servaddr.sin_addr.s_addr = inet_addr("114.212.191.33");
	 }
	
	//servaddr.sin_addr.s_addr = inet_addr("114.212.191.33");
	servaddr.sin_port = htons(SERV_PORT);//convert to big-endian order

	//Connection of the client to the socket
	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
		perror("Problem in connecting to the server");
		exit(2);
	}
	system("clear");

	while(1){//cityname loop
		memset(sendline, 0, MAXLINE);
		memset(recvline, 0, MAXLINE);
		printf("Welcome to Oubeichen's Weather Forecast Demo Program!(by oubeichen 111220086)\n");
		printf("Please input City Name in Chinese pinyin(e.g. nanjing or beijing)\n");
		printf("(c)cls,(#)exit\n");
		fgets(buffer, MAXLINE, stdin);
		if(buffer[strlen(buffer) - 1] == '\n')//remove '\n' at the end
			buffer[strlen(buffer) - 1] = '\0';
		//user selected to exit
		if(buffer[0] == '#' && buffer[1] == '\0'){
			exit(0);
		}
		//clear the screen
		if(buffer[0] == 'c' && buffer[1] == '\0'){
			system("clear");
			continue;
		}

		//validate city name
		sendline[0] = 1;
		strncpy(sendline + 2, buffer, MAXLEN_CITY);
		send_recv(sendline, recvline, sockfd);
		if(recvline[0] == 4){
			printf("Sorry, Server does not have weather information for city %s!\n", buffer);
		}
		//second menu
		else{
			//printf("Valid city.\n");
			system("clear");
			printf("Please enter the given number to query\n");
			printf("1.today\n2.three days from today\n");
			printf("3.custom day by yourself\n(r)back,(c)cls,(#)exit\n");
			printf("===================================================\n");
			while(1){//query choose loop
				fgets(buffer, MAXLINE ,stdin);
				//invalid input
				if(buffer[1] != '\n'){
					printf("Input error!\n");
				}
				//user selected to exit
				else if(buffer[0] == '#'){
					exit(0);
				}
				//clear the screen
				else if(buffer[0] == 'c'){
					system("clear");
					printf("Please enter the given number to query\n");
					printf("1.today\n2.three days from today\n");
					printf("3.custom day by yourself\n(r)back,(c)cls,(#)exit\n");
					printf("===================================================\n");
				}
				//return
				else if(buffer[0] == 'r'){
					system("clear");
					break;
				}
				//today
				else if(buffer[0] == '1'){
					//printf("choose today\n");
					sendline[0] = 2;
					sendline[1] = 1;
					sendline[32] = 1;
					send_recv(sendline, recvline, sockfd);
					output(recvline);
				}
				//three days
				else if(buffer[0] == '2'){
					//printf("choose three days\n");
					sendline[0] = 2;
					sendline[1] = 2;
					sendline[32] = 3;
					send_recv(sendline, recvline, sockfd);
					output(recvline);
				}
				//custom days
				else if(buffer[0] == '3'){
					while(1){//custom days loop
						printf("Please enter the day number(below 10, e.g. 1 means today):");
						fgets(buffer, MAXLINE ,stdin);
						//invalid input
						if(buffer[1] != '\n' || buffer[0] < '1' || buffer[0] > '9'){
							printf("Input error!\n");
						}
						else{
							sendline[0] = 2;
							sendline[1] = 1;
							sendline[32] = buffer[0] - '0';
							send_recv(sendline, recvline, sockfd);
							output(recvline);
							break;
						}
					}
				}
				//other invalid input
				else{
					printf("Input error!\n");
				}
			}
		}
	}
	exit(0);
}


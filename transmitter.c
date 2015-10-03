/*
* File : Transmitter.c
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include "UDP.h"


//Thread function (declaration)
void* XonXoffThread(void *threadArgs);
//Global Variabel
/* Socket */
int fd; 
Byte tCurrent; //current flag (XON/XOFF)
struct sockaddr_in addr; 

main(int argc, char *argv[])
{
	pthread_t child; //thread for buffer container
	//for print IP
	struct ifreq ifr; 
	/* Test for correct number of arguments*/
	if (argc < 3 || argc > 4) 
	printf("<IP Address/Name> <Server Port> [<File text>]");
	/*get port from argument*/		   
	int port = atoi(argv[2]);

     /* create what looks like an ordinary UDP socket */
     if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
     	//if error 
	  perror("socket");
	  exit(1);
     }
     /* set up destination address */
     memset(&addr,0,sizeof(addr));
     addr.sin_family=AF_INET;
     addr.sin_addr.s_addr=inet_addr(argv[1]);
     addr.sin_port=htons(port);
     /*print IP Address*/
     ifr.ifr_addr.sa_family = AF_INET;
     strncpy(ifr.ifr_name,"eth0", IFNAMSIZ-1);
     ioctl(fd, SIOCGIFADDR, &ifr);
     printf("%s: %d\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),port);
			
    /* Create thread process */
    pthread_create(&child,NULL,XonXoffThread,NULL);
    	
    /***inisialisasi for read file***/
    FILE *fp;
    int c;
    Byte End= Endfile;
    //reading from file and send
    fp = fopen(argv[3], "r");
    int i = 0;
    if (fp == 0){
    	fprintf(stderr, "Error while opening");
    	printf("\n");
    	exit(0);
    }
    //socket process
    while(1){
 	if(tCurrent!=XOFF){//flag XON
        	c = fgetc(fp);
        	if(feof(fp)){ //end of file, exits loop
        		
        		ssize_t numBytesSent = 	sendto(fd,&End,sizeof(End),4,(struct sockaddr *) &addr,
		  	sizeof(addr));
          		 break;
        	}
        	printf("sending %c\n", c);
		//send byte to receiver
        	if (sendto(fd,&c,sizeof(int),0,(struct sockaddr *) &addr,
		  sizeof(addr)) < 0) {
	       		perror("sendto");
	       		exit(1);
	  	}
        	i++;
        	if (i >= MAXLEN){ // Check input length
			printf("string too long\n");
			exit(1);
		}
		
	}else{//flag XOFF
		printf("menunggu XON...\n");
	}
	sleep(1);
     }
     //close socket
     fclose(fp);
     printf("\n");

}
//Thread function (implementation)
void* XonXoffThread(void *threadArgs){
	//local variable
	Byte th;
	int s = sizeof(addr);
	while(true){
	//read from socket
	// Receive a response
	ssize_t numBytesRcvd = recvfrom(fd, &th, sizeof(th), 0,
		(struct sockaddr *) &addr,(socklen_t*)&s);
		if (numBytesRcvd < 0) {
		//if error
			printf("recvfrom() failed\n");
		}
		else{
			if(th==XON){
			//flag XON
				printf("XON diterima.\n");
				
			}
			else{//flag XOFF
				printf("XOFF diterima.\n");
			}
			//set flag to global variable
			tCurrent = th;
		}
	}	
}

/* Library */
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include <iostream>

#include "dcomm.h"
/* Delay to adjust speed of sending bytes */
#define DELAY 1000

/* Socket */
int sockfd; // listen on sock_fd
struct sockaddr_in targetAddr;	/* target address */
unsigned int addrLen = sizeof(targetAddr);	/* length of address */
int targetPort;
int recvLen; /* # byte receive */

char bufs[MaxFrameLength]; /* buffer of string */
char res[MaxResponseLength];

char ip[INET_ADDRSTRLEN];

int x = 1; /* 0 = XOFF, 1 = XON */
int parentExit = 0;
char* filename;

/* Functions declaration */
void* threadParent(void *arg);
void* threadChild(void *arg);
void createFrames();
void createWindow();

/* Sliding Window Protocol */
Frame *F[MaxFrame];
int totalFrame = 0;
Window W;

int main(int argc, char *argv[]) {
	Byte c;

	/* Creating Socket */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 0;
	}

	filename = argv[3];
	targetPort = atoi(argv[2]);
	createWindow();

	/* Mengatur target pengiriman ke receiver */
	memset((char *) &targetAddr, 0, sizeof(targetAddr));
	targetAddr.sin_family = AF_INET;
	targetAddr.sin_port = htons(targetPort);
	if (inet_aton((argv[1]), &targetAddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	
	inet_ntop(AF_INET, &(targetAddr.sin_addr), ip, INET_ADDRSTRLEN);
	printf("Membuat socket untuk koneksi ke %s:%d ...\n", ip, ntohs(targetAddr.sin_port));

	/* Thread initialization */
	pthread_t tid[2];
	int err;

	/* Threading */
	err = pthread_create(&(tid[0]), NULL, &threadParent, NULL);
	if (err != 0) { printf("can't create thread : %s", strerror(err)); } else { }

	err = pthread_create(&(tid[1]), NULL, &threadChild, NULL);
	if (err != 0) { printf("can't create thread : %s", strerror(err)); } else { }

	/* Thread joining, finishing program */
	pthread_join( tid[0], NULL);
	pthread_join( tid[1], NULL);

	close(sockfd); 
	exit(EXIT_SUCCESS);

}

void createWindow() {
	createFrames();

	for (int i = 0; i < totalFrame; i++) {
		W.insertFrame(*F[i], i);
	}
}

void createFrames() {
	FILE *fp;
	fp = fopen(filename, "r");
	int c, counter = 0, idx = 0;
	char temp[(MsgLen+1)];

	/* Read the file */
	/* Potong per 5 huruf */
	while ( ((c = fgetc(fp)) != EOF)) {	
		temp[counter] = c;
		++counter;
		if ( counter == MsgLen) {
			temp[counter] = '\0';
 
			F[idx] = new Frame;
			F[idx]->SetMessage(temp); 
			F[idx]->SetNumber(idx); 
			
			counter = 0;
			++idx;
		}
	}

	if ( counter != 0 ) {
		for ( int i = counter; i < MsgLen; i++ ) {
			// Maybe fix this?
			temp[i] = ' ';
		}

		F[idx] = new Frame;
		F[idx]->SetMessage(temp); 
		F[idx]->SetNumber(idx); 
		++idx;		
	}

	totalFrame = idx;

}

void* threadParent(void *arg) {
	/* Parent Thread */

	int c, i = 0, counter = 0;

	while ( !W.isEnd() ) {

		if ( W.getACK(W.getPointer()) != 1 ) {
			string temp = ((W.getCurrentFrame()).GetCompiled());

			sprintf(bufs, "%s", temp.c_str());
			if (sendto(sockfd, bufs, MaxFrameLength, 0, (struct sockaddr *)&targetAddr, addrLen)==-1) {
				printf("err: sendto\n");	
			} else {			
				printf("Mengirim: %s\n", bufs);
			}
		}

		//W.setACK(W.getPointer());
		W.slideWindow();
		W.nextSlot();
		usleep(DELAY * 1000 * 3);

	}

	printf("Exiting Parent\n");
	parentExit = 1;

	return NULL;
}

void* threadChild(void *arg) {
	/* Child Thread */

	while (parentExit == 0) {
		/* Listening XON XOFF signal */ 

		if ( recvfrom(sockfd, res, MaxResponseLength, 0, (struct sockaddr *)&targetAddr, &addrLen) == -1 )
			perror("recvfrom");

		Response R;
		R.GetDecompiled(res);
		if ( R.GetType() == ACK ) {
			W.setACK(R.GetNumber());
			printf("ACK received from Frame %d\n", R.GetNumber());
		} else {
			printf("NAK received from Frame %d\n", R.GetNumber());		
		}

	}

	printf("Exiting Child\n");
	return NULL;
}


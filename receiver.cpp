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

#include "dcomm.h"
/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 2500
/* Define receive buffer size */
#define RXQSIZE 8

int parentExit = 0;

/* Limits */
int minUpperLimit = 2; /* To Send XOFF */
int maxLowerLimit = 4; /* To Send XON */ 

/* Socket */
int sockfd; // listen on sock_fd
struct sockaddr_in myAddr;	/* our address */
struct sockaddr_in targetAddr;	/* target address */
unsigned int addrLen = sizeof(targetAddr);	/* length of address */
int targetPort;
int recvLen; /* # byte receive */

char* buf; /* buffer of buffer */
char res[MaxResponseLength];
char* consumed;

char ip[INET_ADDRSTRLEN];

QTYPE buffer(RXQSIZE); /* the buffer */
int x = 1; /* 0 = XOFF, 1 = XON */
int receivedByte = 0;
int consumedByte = 0;

/* Functions declaration */
static char* rcvchar(int sockfd, QTYPE *queue);
static char* q_get(int sockfd, QTYPE *queue);
void* threadParent(void *arg);
void* threadChild(void *arg);

/* Sliding Window Protocol */
Response R;

int main(int argc, char *argv[]) {
	char* c;

	buf = (char *) malloc(MaxFrameLength * sizeof(char));
	consumed = (char *) malloc(MaxFrameLength * sizeof(char));

	/* Creating Socket */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 0;
	}

	targetPort = atoi(argv[1]);

	memset((void *)&myAddr, 0, sizeof(myAddr));
	myAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "localhost", &(myAddr.sin_addr));
	myAddr.sin_port = htons(targetPort);

	/* Binding socket pada localhost, 127.0.0.1 */	
	if (bind(sockfd, (struct sockaddr *)&myAddr, sizeof(myAddr)) < 0) {
		perror("bind failed");
		return 0;
	}
	
	inet_ntop(AF_INET, &(myAddr.sin_addr), ip, INET_ADDRSTRLEN);
	printf("Binding pada 127.0.0.1:%d\n", ntohs(myAddr.sin_port));
	
	/* Mengatur target kepada sender untuk mengirim XON dan XOFF */
	memset((char *) &targetAddr, 0, sizeof(targetAddr));
	targetAddr.sin_family = AF_INET;
	targetAddr.sin_port = htons(targetPort);
	
	/* Thread initialization */
	pthread_t tid[2];
	int err;

	/* Threading */
	err = pthread_create(&(tid[0]), NULL, &threadParent, NULL);
	if (err != 0) { printf("can't create thread : %s", strerror(err)); } else { }

	err = pthread_create(&(tid[1]), NULL, &threadChild, NULL);
	if (err != 0) { printf("can't create thread : %s", strerror(err)); } else { }

	/* Joining thread, finishing program */
	pthread_join( tid[0], NULL);
	pthread_join( tid[1], NULL);
 
	/* Closing socket */
	close(sockfd);
	exit(EXIT_SUCCESS);
}

void* threadParent(void *arg) {
	/* Parent Thread */
	char* chck;
	
	/* Ketika belum diterima EOF, 
	   teruskan listening untuk penerimaan byte */
	while (parentExit == 0) {	
		chck = rcvchar(sockfd, &buffer);
		if ( chck != NULL ) {
			//if ( *chck != 26 ) {
				printf("Menerima Frame\n");
			/*} else {
				printf("EOF diterima\n");
				parentExit = 1;
			}*/
		}
	}

	/*char state[1];
	sprintf(state, "%c",(char) XOFF);
	if (sendto(sockfd, state, 1, 0, (struct sockaddr *)&targetAddr, addrLen) == -1) {
		perror("sendto");
	}*/
	return NULL;
}

void* threadChild(void *arg) {
	/* Child Thread */
	char* chck;

	/* Sampai program diakhiri, konsumsi terus byte yang ada pada buffer*/
	while (true) {	
		chck = q_get(sockfd, &buffer);
		if ( chck != NULL ) {
			Frame F;
			F.GetDecompiled(chck);
			printf("Mengkonsumsi [ %s ] Mencoba mengirim ACK\n", (F.GetMessage()).c_str());

			R.SetNumber(F.GetNumber());
			R.SetChecksum(F.GetChecksum());

			// Randomizing ACK and NAK
			if ( (rand() % 5) < 3 ) {
				R.SetType(ACK);
			} else {
				R.SetType(NAK);
			}

			sprintf(res, "%s", (R.GetCompiled()).c_str());
			if (sendto(sockfd, res, MaxResponseLength, 0, (struct sockaddr *)&targetAddr, addrLen) == -1) {
				perror("sendto");
			}
		}
		usleep(DELAY * 1000);
		
	}

	return NULL;
}

static char* rcvchar(int sockfd, QTYPE *queue) {
	/*
	Read a character from socket and put it to the receive buffer.
	If the number of characters in the receive buffer is above certain
	level, then send XOFF and set a flag.
	Return a buffer value.
	*/
	/*int emptySpace = (*queue).EmptySpace();

	if (emptySpace <= minUpperLimit) {
		char state[1];

		printf("Buffer > minimum upperlimit. Mengirim XOFF.\n");
		sprintf(state, "%c",(char) XOFF);
		if (sendto(sockfd, state, 1, 0, (struct sockaddr *)&targetAddr, addrLen) == -1) {
			perror("sendto");
		}

		x = 0;

		sleep(5);
		
		return NULL;

	} else {*/

		recvLen = recvfrom(sockfd, buf, MaxFrameLength, 0, (struct sockaddr *)&targetAddr, &addrLen);

		//if ( buf[0] != 26 ) {
			(*queue).Add(buf);
		//}	

		return buf;
	/*}*/

}

/* q_get returns a pointer to the buffer where data is read or NULL if
 * buffer is empty.
 */
static char* q_get(int sockfd, QTYPE *queue) {
	int emptySpace = (*queue).EmptySpace();

	if (emptySpace < RXQSIZE) {
		/*
		Retrieve data from buffer
		If the number of characters in the receive buffer is below certain
		level, then send XON.
		Increment front index and check for wraparound.
		*/
		
		/*if ( ( emptySpace >= maxLowerLimit ) && ( x == 0 ) ) {
			char state[1];

			printf("Buffer < maximum lowerlimit. Mengirim XON.\n");
			sprintf(state, "%c",(char) XON);
			if (sendto(sockfd, state, 1, 0, (struct sockaddr *)&targetAddr, addrLen)==-1) {
				perror("sendto");
			}
			x = 1;

			sleep(5);

			return NULL;

		} else {*/

			consumed = (*queue).Del();
			return consumed;

		//}
	} else {
		/* Nothing in the queue */
		return NULL;
	}

}

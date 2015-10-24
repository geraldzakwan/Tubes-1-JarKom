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
/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 2500
/* Define receive buffer size */
#define RXQSIZE 8

int parentExit = 0;
int getEOF = 0;

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
bool cekBuffer(int arr[], int start, int length);

/* Sliding Window Protocol */
Response R;
Window Rw;

int main(int argc, char *argv[]) {
	char* c;

	/* Initializing buffer for receiving and sending Frame */
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
	
	/* Ketika belum diterima End Of Frame, 
	   teruskan listening untuk penerimaan byte */
	while ( (parentExit != 1) ) {	
		chck = rcvchar(sockfd, &buffer);
		if ( chck != NULL ) {
			Frame F;
			F.GetDecompiled(chck);

			if ( (F.GetMessage()).at(0) != 26 ) {
				printf("Menerima Frame ke-%d\n", F.GetNumber());
			} else {
				printf("EOF diterima\n");
				getEOF = F.GetNumber();
			}		
		} 
	}

	printf("Exiting Child\n");
	return NULL;
}

void* threadChild(void *arg) {
	/* Child Thread */
	char* chck;
	int i = 0;
	int length = 0;
	int start = 0;
	bool canBeConsumed = false;
	Frame Ftemp[100];
	int temp[100];
	int tempLength = 0;

	/* Sampai program diakhiri, konsumsi terus byte yang ada pada buffer*/
	while (parentExit != 1) {	
		chck = q_get(sockfd, &buffer);
		if ( chck != NULL ) {
			Frame F;
			F.GetDecompiled(chck);

			// Set frame number dan checksum pada Response
			R.SetNumber(F.GetNumber());
			R.SetChecksum(F.GetChecksum());

			// Checks checksum
			//cout<<"\nGetCheckSum = "<<F.GetChecksum () ;
			//cout<<"\nCheckSum = "<<F.GenerateChecksumCRC(chck) ;
			if ( F.GenerateChecksumCRC(chck) != F.GetChecksum () ) {
				printf("Invalid Checksum. Send NAK.\n");
				R.SetType(NAK);

				--receivedByte;
			} else {
				// Randomizing ACK and NAK Response (DEBUG ONLY)
				if ((rand() % 5) < 3) {
					R.SetType(ACK);	
				} else {
					R.SetType(NAK);
				
					--receivedByte;
				}
			}

			int num = R.GetNumber();
			
			if (tempLength < ( R.GetNumber() + 1 )) {
				tempLength = R.GetNumber() + 1;
			}

			if (R.GetType() == ACK)
			{
				if (num < i)
				{
					temp[num] = num;
					Ftemp[num] = F;
				}
				else {
					temp[i] = i;
					Ftemp[i] = F;
					if ( i > 0 ) { length++; }
				}
			}	
			else {
				//NAK
				if (num < i)
					temp[num] = -1;
				else
					temp[i] = -1;
			}

			int j = Rw.getLength();

			while ( ( temp[j] != -1 ) && ( j < tempLength ) ) {

				Rw.insertFrame(Ftemp[j], j);
				cout << "Mengkonsumsi frame ke-" << j << " [" << Rw.getFrame(j).GetMessage() << "] Mencoba mengirim ACK" << endl;
				
				++consumedByte;
				Rw.slideWindow();
				++j;
			}
			i++;


			sprintf(res, "%s", (R.GetCompiled()).c_str());
			if (sendto(sockfd, res, MaxResponseLength, 0, (struct sockaddr *)&targetAddr, addrLen) == -1) {
				perror("sendto");
			}

			if ( ( getEOF != 0 ) && ( (getEOF + 1) == consumedByte ) ) {
				parentExit = 1;
			}

		}
		
	}

	Rw.iterateFrames();
	printf("Exiting Parent\n");
	return NULL;
}

static char* rcvchar(int sockfd, QTYPE *queue) {
	/*
	Read a character from socket and put it to the receive buffer.
	Return a buffer value.
	*/
	recvLen = recvfrom(sockfd, buf, MaxFrameLength, 0, (struct sockaddr *)&targetAddr, &addrLen);

	(*queue).Add(buf);

	return buf;

}

/* q_get returns a pointer to the buffer where data is read or NULL if
 * buffer is empty.
 */
static char* q_get(int sockfd, QTYPE *queue) {
	int emptySpace = (*queue).EmptySpace();

	if (emptySpace < RXQSIZE) {
		/*
		Retrieve data from buffer
		*/
		
		consumed = (*queue).Del();
		return consumed;

	} else {
		/* Nothing in the queue */
		return NULL;
	}

}

bool cekBuffer(int arr[], int start, int length)
{
	bool hasil = true;
	for (int i = start; i <= length; i++)
	{
		if (arr[i] == -1)
			hasil = false;
	}

	return hasil;
}

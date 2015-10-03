/*
* File : dcomm.h
*/
#ifndef _DCOMM_H_

#define _DCOMM_H_

/* XON/XOFF protocol */
#define XON (0x11)
#define XOFF (0x13)
/* Const */
#define BYTESIZE 256 /* The maximum value of a byte */
#define MAXLEN 1024 /* Maximum messages length */

typedef unsigned char Byte;
typedef struct QTYPE
{
	unsigned int count;
	unsigned int front;
	unsigned int rear;
	unsigned int maxsize;
	Byte *data;
} QTYPE;

bool IsFull(QTYPE Q);
int EmptySpace(QTYPE Q);
void CreateQueue(QTYPE *Q, unsigned int size);
void Add(QTYPE *Q, Byte b);
Byte Del(QTYPE *Q);
void ViewContent(QTYPE *Q);

typedef struct Frame
{
	int number;
	char *message;
	char *checksum;
	int length;
	
} Frame;

void CreateFrame (Frame *F);
void SetNumber (Frame *F, int i);
void SetMessage (Frame *F, char* msg);
void SetChecksum (Frame *F);
void SetLength (Frame *F, int l);
int GetNumber (Frame F);
char* GetMessage (Frame F);
char* GetChecksum (Frame F);
int GetLength (Frame F);
char* CompileFrame (Frame F);

#endif










/*
* File : dcomm.h
*/
#include <string>
using namespace std;

#ifndef _DCOMM_H_

#define _DCOMM_H_

/* XON/XOFF protocol */
#define XON (0x11)
#define XOFF (0x13)

/* Const */
#define SOH 1
#define STX 2
#define ETX 3
#define EOT 4
#define ACK 6
#define NAK 15

/* Message */
#define MsgLen 5
#define MaxFrame 250
#define MaxFrameLength 50
#define MaxResponseLength 40
#define WindowSize 5
#define IntLen 5

typedef unsigned char Byte;
class QTYPE {
private:
	int count;
	int front;
	int rear;
	int maxsize;
	string *data;

public:
	QTYPE();
	QTYPE(int size);
	bool IsFull();
	int EmptySpace();
	void Add(char* b);
	char* Del();
	void ViewContent();
};


class Frame {
private:
	int number;
	string message;
	int checksum;
	int length;
	string compiled;
	
public:
	Frame();
	Frame(const Frame &F);
	~Frame();
	void SetNumber (int i);
	void SetMessage (char* msg);
	void SetChecksum (char* checkString);
	void SetLength (int l);
	string GetMessage ();
	int GetNumber ();
	int GetLength ();
	int GetChecksum ();
	string GetCompiled();
	void GetDecompiled (char* frame);
	int GenerateChecksum(char* checkString);
	int GenerateChecksumCRC(char* checkString);
	void printbit(long long a);
	string GetCompiledWithoutChecksum ();
};

class Response {
private:
	int number;
	char type;
	int checksum;
	string compiled;

public:
	Response();
	void SetNumber(int i);
	void SetType(char c);
	void SetChecksum(int c);
	int GetChecksum ();
	int GetNumber();
	char GetType();
	string GetCompiled();
	void GetDecompiled(char* frame);
};

class Window {
private:
	Frame *Frames;
	int *ackStatus;
	int totalFrames;
	int start;
	int size;
	int pointer;
	int length;

public:
	Window();
	~Window();
	void insertFrame(Frame F, int i);
	Frame getCurrentFrame();
	Frame getFrame(int FNum);
	int getLength();
	int getPointer();
	int getACK(int i);
	void slideWindow();
	void nextSlot();
	void setACK(int i);
	void iterateFrames();
	int isEnd();
};

char* StringToChars (string S);
int getIntLength(int i);

#endif

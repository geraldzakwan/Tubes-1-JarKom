#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "dcomm.h"

using namespace std;

/* QYTPE PROCEDURE AND FUNCTIONS */

bool QTYPE::IsFull(){
	return (count == maxsize);
}

int QTYPE::EmptySpace() {
	return maxsize - count;
}

QTYPE::QTYPE() : maxsize(0) {
	rear = -1;
	front = 0; 
	count = 0;
	data = new string [maxsize];
}

QTYPE::QTYPE(int size) : maxsize(size) {
	rear = -1;
	front = 0; 
	count = 0;
	data = new string [maxsize];
}

void QTYPE::Add(char* b){
	if ( rear == maxsize - 1 ) {
		rear = 0;
	} else {
		rear += 1;
	}
	string temp(b);
	data[rear] = temp;
	count++;
}

char* QTYPE::Del() {
	char *temp = (char *) malloc (sizeof (char) * MaxFrameLength);
	sprintf(temp, "%s", (data[front]).c_str());
	data[front] = "";
	if ( front == maxsize - 1 ) {
		front = 0;
	} else {
		front += 1;
	}
	count--;
	return temp;	
}

void QTYPE::ViewContent() {
	int i;

	for ( i = 0; i <= maxsize-1; i++ ) {
		cout << " * " << data[i] << endl; 
	}

	cout << " [ Front: " << front << " ] " << endl;
	cout << " [ Rear: " << rear << " ] " << endl;
}

/* FRAME PROCEDURE AND FUNCTIONS */

Frame::Frame () {
}

Frame::Frame (const Frame &F) {
	number = F.number;
	message = F.message;
	length = F.length;
	compiled = F.compiled;
	checksum = F.checksum;
}

Frame::~Frame() {
}

void Frame::SetNumber (int i) {
	number = i;	
}

void Frame::SetMessage (char* msg) {
	string temp(msg);
	message = temp;
	SetChecksum(msg);
}

void Frame::SetChecksum (char* checkString) {
	int bitSum = 0, x = 0, r = 0;
	int bitTest, bitHeader, bitBody, checkSum;

	for ( int i = 0; i < strlen(checkString); i++ ) {
		bitSum += (int) checkString[i];
	}

	bitTest = bitSum;
	while ( bitTest != 0 ) {
		bitTest = bitTest >> 1;
		++r;
		if ( r % 4 == 1 ) {
			++x;
		}
	}

	x *= 4;

	bitHeader = ( bitSum >> (x - 4) ) << (x - 4);
	bitBody = bitSum - bitHeader;

	checkSum = ~(bitBody + (bitSum >> (x - 4))) & 0x000000ff;

	this->checksum = checkSum;
}

void Frame::SetLength (int l) {
	length = l;	
}

int Frame::GetNumber () {
	return number;
}

string Frame::GetMessage () {
	return message;
}

int Frame::GetLength () {
	return length;
}

int Frame::GetChecksum () {
	return checksum;
}

string Frame::GetCompiled () {
	int n = getIntLength(GetNumber());
	int m = getIntLength(GetChecksum());
	int size = 1 + n + 1 + MsgLen + 1 + 1 + 1;
	char ret[size]; 
	char num[n];
	char chk[m];

	ret[0] = SOH;

	sprintf(num, "%d", GetNumber());	
	sprintf(chk, "%d", GetChecksum());

	for (int i = 0; i < n; i++) {
		ret[i+1] = num[i];
	}

	ret[1 + n] = STX;

	for (int i = 0; i < MsgLen; i++) {
		ret[1 + n + i + 1] = (GetMessage()).at(i);	
	} 

	ret[1 + n + MsgLen + 1] = ETX;

	for (int i = 0; i < m; i++) {
		ret[1 + n + MsgLen + 2 + i] = chk[i];
	}

	ret[1 + n + MsgLen + m + 2] = '\0';

	string temp(ret);
	compiled = temp;

	return compiled;
}

void Frame::GetDecompiled (char* frame) {
	if ( frame[0] == SOH ) {

		int i = 1;
		char num[33];
		char chk[33];
		char msg[MsgLen];

		while ( frame[i] != STX ) {
			num[i-1] = frame[i];
			++i;
		}

		num[i-1] = '\0';
		++i;
		number = atoi(num);
		int n = 0;

		while ( frame[i] != ETX ) {
			msg[n] = frame[i];
			++i; ++n;
		}
		msg[n] = '\0';
		++i;

		string temp(msg);
		message = temp;

		n = 0;
		while ( frame[i] != '\0' ) {
			chk[n] = frame[i];
			++i; ++n;			
		}
		chk[n] = '\0';

		checksum = atoi(chk);

	} else {
		printf("Error - Frame Corrupted");
	}
}

/* WINDOW PROCEDURE AND FUNCTIONS */

Window::Window() : size(WindowSize) {
	Frames = new Frame [MaxFrame];
	ackStatus = new int [MaxFrame];
	totalFrames = 0;
	start = 0;
	pointer = 0;
	length = 0;
}

Window::~Window() {
	delete [] Frames;
	delete [] ackStatus;
}

void Window::insertFrame(Frame F, int i) {
	Frames[i] = F;
	ackStatus[i] = 0;
	++length;
}

Frame Window::getCurrentFrame() {
	return (Frames[pointer]);
}

int Window::getACK(int i) {
	return ackStatus[i];
}

void Window::setACK(int i) {
	ackStatus[i] = 1;
}

int Window::getPointer() {
	return pointer;	
}

void Window::nextSlot() {
	++pointer;

	if ( (pointer < start) || (pointer >= start + size) ) {
		pointer = start;
		cout << "invalid pointer p:" << pointer << endl;
	}

}

void Window::slideWindow() {
	if ( ( start + size ) <= ( length ) ) {
		while ( ackStatus[start] == 1 ) {
			// Geser Window
			cout << "Geser" << endl;
			++start;
			
		}
	}
}

int Window::isEnd() {
	return ( pointer == length ); 
}


/* RESPONSE PROCEDURE AND FUNCTIONS */

Response::Response() {
	number = 0;
	type = NAK;
}

void Response::SetNumber(int i) {
	number = i;
}

void Response::SetType(char c) {
	type = c;
}

void Response::SetChecksum(int c) {
	checksum = c;
}

int Response::GetNumber() {
	return number;
}

char Response::GetType() {
	return type;
}

int Response::GetChecksum () {
	return checksum;
}

string Response::GetCompiled() {
	int n = getIntLength(GetNumber());
	int size = 1 + n + 1 + 1;
	char ret[size]; 
	char num[n];

	ret[0] = type;

	sprintf(num, "%d", GetNumber());	
	for (int i = 0; i < n; i++) {
		ret[i+1] = num[i];
	}

	ret[1 + n] = 'X';
	ret[1 + n + 1] = '\0';

	string temp(ret);
	compiled = temp;

	return compiled;	
}

void Response::GetDecompiled(char* frame) {
	type = frame[0];

	int i = 1;
	char num[33];

	while ( frame[i] != 'X' ) {
		num[i-1] = frame[i];
		++i;
	}

	num[i-1] = '\0';
	++i;
	number = atoi(num);
}

/* Others */

char* StringToChars (string S) {
	char * writable = new char[S.size() + 1];
	copy(S.begin(), S.end(), writable);
	writable[S.size()] = '\0'; 

	return writable;

	delete[] writable;
}

int getIntLength(int i) {
	int n = 0;
	do {
		++n;
		i /= 10;
	} while (i);

	return n;
}
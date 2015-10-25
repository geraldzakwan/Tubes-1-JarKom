#include<stdio.h>
#include<string.h>

void printbit(long long a){
int i;
	for(i= sizeof(long long)*8; i>0; i--){
		printf("%d", (int)((a>>(i-1))&1));
	}
	printf("\n");
}
int GenerateChecksumCRC() {
	unsigned long long dummy= 0x3132303361733100;
int i;
	
	unsigned long long seed= 0x131;

	printbit(dummy);
	for( i = sizeof(long long)*8; i>8; i--){
		if((dummy>>(i-1))&1){
			dummy = dummy ^ (seed<<(i-9));
			printbit(dummy);
		}
		
	}
	printbit(dummy);
	return (int) dummy;
	
}


int main()
{
	printf("A= %d", GenerateChecksumCRC());
	
	
	
}

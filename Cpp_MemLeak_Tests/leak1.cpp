#include <iostream>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <sys/timeb.h>
using namespace std;

int getMilliCount(){
	timeb tb;
	ftime(&tb);
	int nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
	return nCount;
}

int getMilliSpan(int nTimeStart){
	int nSpan = getMilliCount() - nTimeStart;
	if(nSpan < 0)
		nSpan += 0x100000 * 1000;
	return nSpan;
}


void createLeak()
{
	char	*memptr;
	memptr = new char [100];
	if (memptr != NULL)
	{
		cout << "memptr is not Null" << endl;
	}else{
		cout << "memptr is Null" << endl;
	}
	//delete [] memptr; 
}
 
int main(){
	cout << "Leak 1 test" << endl;
	int start = getMilliCount();
	createLeak();
	int milliSecondsElapsed = getMilliSpan(start);
	cout << "Runtime: " <<  milliSecondsElapsed << endl; 
return 0;
}

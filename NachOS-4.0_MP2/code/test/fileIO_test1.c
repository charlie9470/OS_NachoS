#include "syscall.h"

int main(void)
{
	char test[] = "abcdefghijklmnopqrstuvwxyz";
//	int a[20];
	int i;
	OpenFileId fid;
	OpenFileId f1;
	OpenFileId f2;
	OpenFileId f3;
	OpenFileId f4;
	OpenFileId f5;
	OpenFileId f6;
	OpenFileId f7;
	OpenFileId f8;
	OpenFileId f9;
	OpenFileId f10;
	int success;
	success = Create("1.txt");
	success = Create("3.txt");
	success = Create("4.txt");
	success = Create("5.txt");
	success = Create("7.txt");
	success = Create("8.txt");
	success = Create("9.txt");
	success = Create("10.txt");
	success = Create("file1.test");
	if (success != 1) MSG("Failed on creating file");
	fid = Open("file1.test");
	f1 = Open("1.txt");
	f2 = Open("2.txt");
	f3 = Open("3.txt");
	f4 = Open("4.txt");
	f5 = Open("5.txt");
	f6 = Open("6.txt");
	f7 = Open("7.txt");
	f8 = Open("8.txt");
	f9 = Open("9.txt");
	f10 = Open("10.txt");
	
	if (fid < 0) MSG("Failed on opening file");
	
	for (i = 0; i < 26; ++i) {
		int count = Write(test + i, 1, fid);
		if (count != 1) MSG("Failed on writing file");
	}
       
	success = Close(fid);
	
	if (success != 1) MSG("Failed on closing file");
	MSG("Success on creating file1.test");
	Halt();
}


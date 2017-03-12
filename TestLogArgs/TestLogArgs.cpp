// TestLogArgs.cpp : Defines the entry point for the console application.
//

#include "windows.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <locale>
#include <codecvt>

using namespace std;

int wmain(int argc, wchar_t *argv[])
{
	SYSTEMTIME lt;
	GetLocalTime(&lt);
	std::wofstream flog(L"TestLogArgs.log",ios_base::binary);
	flog.imbue(std::locale(flog.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));

	flog << L"TestLogArgs " << L"запущен argc= " << argc << " ";
	flog << setfill(L'0') << setw(2)  << lt.wHour << ":" << setfill(L'0') << setw(2) << lt.wMinute << ":" << setfill(L'0') << setw(2) << lt.wSecond << "." << setfill(L'0') << setw(3) <<lt.wMilliseconds << endl;
	for (int i = 0; i < argc; i++) {
		flog << argv[i];
	}
	
    return 0;
}


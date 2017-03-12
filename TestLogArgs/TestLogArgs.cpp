// TestLogArgs.cpp : Defines the entry point for the console application.
//

#include "windows.h"
#include <iostream>
#include <fstream>

#include "easylogging++.h"

int wmain(int argc, wchar_t *argv[])
{
	fostre
	LOG(INFO) << "TestLogArgs " << L"запущен argc= " << argc;
	for (int i = 0; i < argc; i++) {
		LOG(INFO) << argv[i];
	}
    return 0;
}


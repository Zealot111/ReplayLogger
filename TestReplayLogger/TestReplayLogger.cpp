// TestReplayLogger.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <Windows.h>
#include <string>
using namespace std;

#include "easylogging++.h"


void test(const char * str, void(__stdcall* function)(char*, size_t, const char*)) {
	char buffer[10240];
	buffer[0] = 0;
	function(buffer, sizeof(buffer), str);
	LOG(INFO) << "IN(" << str << ") OUT(" << buffer << ")";
}


int main()
{
	HMODULE module = LoadLibrary(L"ReplayLogger.dll");

	if (!module)
	{
		LOG(ERROR) << "!module";
		return -1;
	}

	FARPROC farproc = GetProcAddress(module, "_RVExtension@12");

	if (!farproc)
	{
		LOG(ERROR) << "!farproc";
		return -1;
	}

	auto* f = reinterpret_cast<void(__stdcall*)(char*, size_t, const char*)>(farproc);



	test(":VER:test", f);
	test("just parameter", f);
	test(":COM:{sdf dfdf dfdg;fg dfdf;444}dfgdfgfg", f);
	test(":DEBUG:1", f);
	test(":DEBUG:ON", f);
	test(":FILE:new mission", f);
	test(":COM:{first parameter;second parameter;123456}just left string", f);

	return 0;
}

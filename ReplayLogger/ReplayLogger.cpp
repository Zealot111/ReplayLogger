// by Zealot 
// MIT licence https://opensource.org/licenses/MIT

/*
v. 1.2		07-11-2016 Zealot Добавлена спец команда для включения Debug-протокола, по умолчанию он отключен
v. 1.3 		07-11-2016 Zealot Немного обобщена лолгика работы расширения
v. 1.31		07-11-2016 Zealot Небольшой багфикс
v. 1.4		08-11-2016 Zealot Расширение понимает Unicode имена миссий
v. 1.5		08-11-2016 Zealot записывает в replay.log в utf8 формате
v. 2.0a		17-02-2016 Zealot Новая версия, другая логика работы
v. 2.01a	17-02-2016 Zealot Пофикшен баг с необнулением переменных
v. 2.02a	17-02-2016 Zealot Пофикшен баг с необнулением переменных
v. 2.03a	17-02-2016 Zealot ПОфикшен баг с неправильным именем файла
v. 2.04a	17-02-2016 Zealot ПОфикшен баг с неправильным составлением итоговой записи
v. 2.05a	17-02-2016 Dell   Обнуление переменных выведена в самостоятельную команду, подправлен вывод event, изменена папка записи
v. 2.06b	17-02-2016 Zealot При команде COM файл каждый раз пишется с самого начала
v. 2.07a	18-02-2017 Dell   Закрытие файла в команде COM + вызов команды DEL
v. 2.08b	01-03-2017 Zealot Добавлен запуск ocap.exe в конце команды COM
v. 2.09b	01-03-2017 Zealot ocap.exe запускается в отдельном потоке
v. 2.10b	01-03-2017 Zealot Исправлена ошибка с неправильным парсингом строки для COM
v. 2.13		03-03-2017 Zealot Логгирование через easylogging++
*/

#define VERSION "v. 2.13"

#include "easylogging++.h"

#include <cstring>
#include <string>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <Windows.h>
#include <direct.h>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include <locale>
#include <codecvt>
#include <vector>
#include <thread>
#include <mutex>

#define CURRENT_LOG_FILENAME L"replays.log"
#define REPLAY_CDIR     L"Temp"
#define REPLAY_FILEMASK L"%Y_%m_%d__%H_%M_"
#define OCAP_EXE_NAME	"ocap.exe"

#define CMD_DEBUG			":DEBUG:"
#define CMD_NEW				":NEW:"
#define CMD_EVENT			":EVENT:"
#define CMD_FILE			":FILE:"
#define CMD_ADD				":ADD:"
#define CMD_DEFAULT			":!DEFAULT!:"
#define CMD_COM				":COM:"
#define CMD_DEL				":DEL:"
#define CMD_VER				":VER:"

using namespace std;

string commandDEBUG(const string &param);
string commandVER(const string &param);

string commandNEW(const string &str);
string commandEVENT(const string &str);
string commandADD(const string &str);
string commandFILE(const string &str);
string commandDEL(const string &str);
string commandCOM(const string &str_in);
string commandDEFAULT(const string &param);

namespace {

	
	fstream currentReplay;
	bool replayStarted = false;

	std::unordered_map<std::string, std::function<string(const std::string &)> > commands = {
		{	CMD_DEBUG,			commandDEBUG		},
		{	CMD_VER,			commandVER			},
		{	CMD_NEW,			commandNEW			},
		{	CMD_EVENT,			commandEVENT		},
		{	CMD_ADD,			commandADD			},
		{	CMD_FILE,			commandFILE			},
		{	CMD_DEL,			commandDEL			},
		{	CMD_COM,			commandCOM			},
		{	CMD_DEFAULT,		commandDEFAULT		}
	};

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	vector<string> lines(100); // итоговый массив строчек, 100 - размер в начале работы 
	string events(""); // events - переменная с евентами, туда будут подклеиваться все евенты

	string ocapCmdLine("");
}


void initializeLogger(bool forcelog = false, int verb_level = 0) {
	bool file_exists = forcelog;
	if (ifstream("ReplayLogger.log"))
		file_exists = true;

	el::Configurations defaultConf;
	defaultConf.setToDefault();
	if (!file_exists) {
		defaultConf.setGlobally(el::ConfigurationType::Enabled, "false");
		defaultConf.setGlobally(el::ConfigurationType::ToFile, "false");
	}
	else {
		defaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
		defaultConf.setGlobally(el::ConfigurationType::Enabled, "true");
		defaultConf.setGlobally(el::ConfigurationType::Filename, "ReplayLogger.log");
	}

	defaultConf.setGlobally(el::ConfigurationType::ToStandardOutput,		"false");
	defaultConf.setGlobally(el::ConfigurationType::Format,					"%datetime %fbase:%line %level %msg");
	defaultConf.setGlobally(el::ConfigurationType::MillisecondsWidth,		"4");
	defaultConf.setGlobally(el::ConfigurationType::MaxLogFileSize,			"104857600"); // 100 Mb
	
	el::Loggers::reconfigureAllLoggers(defaultConf);
	el::Loggers::setDefaultConfigurations(defaultConf);
	el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);
	el::Loggers::addFlag(el::LoggingFlag::AutoSpacing);
	el::Loggers::setVerboseLevel(verb_level);
//	el::Loggers::addFlag(el::LoggingFlag::HierarchicalLogging);
	LOG(INFO) << "Logging initialized " << VERSION;
}

string makeResponseString(int code, const char * ok, const char * msg) {
	stringstream ss;
	ss << "[" << code << ", " << "'" << ok << "', '" << msg << "']";
	return ss.str();
}

/*
Команды
*/

string commandDEBUG(const string &param) {
	LOG(INFO) << param;
	string res = makeResponseString(-1, "ER", "Parameter unknown" );
	if (!param.compare("ON")) {
		initializeLogger(true,5);
		LOG(INFO) << "Log enabled";
		res = makeResponseString(1, "OK", "Extended log enabled");
	} else if (!param.compare("OFF")) {
		initializeLogger(false, 0);
		LOG(INFO) << "Log disabled";
		res = makeResponseString(0, "OK", "Extended log disabled");
	}
	else {
		LOG(ERROR) << "Unknown  imput parameter" << param;
	}
	return res;
}


string commandNEW(const string &str) {
	lines.push_back(str);
	return makeResponseString(lines.size() - 1, "OK", "Inserted as number");
}

string commandEVENT(const string &str) {
	events.append(str);
	return makeResponseString(events.size() - 1,"OK","Events appended");
}

string commandADD(const string &str) {
	LOG(INFO) << str;
	string::size_type pos;
	int str_num = stoi(str, &pos); // выделяем номер строки
	string val = str.substr(pos); // отрезаем оставшуюся часть
	if (val.front() == ';') { // если первый символ остатка не ; то что-то пошло не так
		val = val.substr(1);
		if (lines.size() <= (size_t)str_num) { // если номер больше количества элементов сейчас в массиве, то аргументы некорректны
			string res = makeResponseString(-str_num, "ER", "Incorrect array index");
			LOG(ERROR) << res;
			return res;
		}
		else {
			lines.at(str_num) += val; // добавляем в наш массив остаток строки
			string res = makeResponseString(str_num, "OK", "String appended");
			LOG(INFO) << res;
			return res;
		}
	}
	else {
		string res = makeResponseString(-(int) pos, "ER", "Incorrect sequence");
		LOG(ERROR) << res;
		return res;
	}
}

wstring commandFILE_getFileName(const string &name) {
	std::time_t t = std::time(nullptr);
	std::tm tm;
	localtime_s(&tm, &t);
	wstringstream ss;
	ss << put_time(&tm, REPLAY_FILEMASK) << converter.from_bytes(name) << L".json";
	return ss.str();
}


string commandFILE(const string &str) {
	LOG(INFO) << str;
	if (!replayStarted) {
		wstring newName = commandFILE_getFileName(str);

		// создаем файл на файловой системе для реплея
		
		currentReplay.open(wstring(REPLAY_CDIR) + L"\\" + newName, fstream::out | fstream::trunc | fstream::binary);
		replayStarted = true;
		fstream replaysLog;
		replaysLog.open(wstring(REPLAY_CDIR) + L"\\" CURRENT_LOG_FILENAME, fstream::out | fstream::trunc | fstream::binary);
		replaysLog << converter.to_bytes(newName);
		replaysLog.flush();
		replaysLog.close();
		string msg = converter.to_bytes(wstring(wstring(L"Files created: ") + wstring(REPLAY_CDIR) + L"\\" + newName + L" , " + wstring(REPLAY_CDIR) + L"\\" CURRENT_LOG_FILENAME));
		string res = makeResponseString(0, "OK", msg.c_str());
		LOG(INFO) << res;
		return res;
	}
	else {
		string res = makeResponseString(-1, "ER", "Unexpected command, already FILE context" );
		LOG(ERROR) << res;
		return res;
	}
}

string commandDEL(const string &str) {
	LOG(INFO) << str;
	lines.clear();
	events.clear();
	replayStarted = false;

	return "[0,'OK','Variables cleared']";
}

string parseOcapParams(const string &string_in, vector<string> &str_out) {
	size_t sbeg = string_in.find_first_of("{");
	size_t send = string_in.find_first_of("}");
	string result(string_in);
	LOG(TRACE) << sbeg << send;
	if (sbeg != string::npos && send != string::npos) {
		string params_string = string_in.substr(sbeg, send - sbeg + 1);
		result = string_in.substr(send + 1);
		params_string.erase(0, 1);
		params_string.erase(params_string.length() - 1);
		size_t pos = string::npos;
		while ((pos = params_string.find(";")) != string::npos) {
			str_out.push_back(params_string.substr(0, pos));
			params_string.erase(0, pos + 1);
		}
		if (!params_string.empty())
			str_out.push_back(params_string);
	}
	return result;
}

string prepareOcapCommand(const vector<string> & params) {
	string res(OCAP_EXE_NAME);
	for (auto &ii : params) {
		res += " " + ii;
	}
	return res;
}

void runOcap(const string &params) {
	LOG(TRACE) << params;
	system(params.c_str());
}

string commandCOM(const string &str_in) {
	LOG(INFO) << str_in;
	if (!replayStarted) {
		string res = makeResponseString(-1, "ER", "Unexpected command, you should use FILE command before");
		LOG(ERROR) << res;
		return res;
	}

	vector<string> ocap_params;
	const string str = parseOcapParams(str_in, ocap_params);
	LOG(TRACE) << "str=" << str << "ocap_params=" << ocap_params;
	ocapCmdLine = prepareOcapCommand(ocap_params);
	
	currentReplay.seekp(ios_base::beg);
	currentReplay << str << endl;
	for (auto &s1 : lines) {
		currentReplay << s1 << "]" << endl;
	}
	currentReplay << events << "]}" << endl;
	currentReplay.flush();
	currentReplay.close();
	commandDEL(str);
	// запускаем ocap в отдельном треде
	thread jt(runOcap, ocapCmdLine);
	jt.detach();

	return "[0,'OK','Written to file']";
}



string commandDEFAULT(const string &param) {
	LOG(WARNING) << param;
	return makeResponseString(0, "ER", "Using DEFAULT command");
}

string commandVER(const string &param) {
	LOG(INFO) << param << VERSION;
	return makeResponseString(0, "OK", VERSION);
}

extern "C"
{
	__declspec (dllexport) void __stdcall RVExtension(char *output, int outputSize, const char *function);
}

void __stdcall RVExtension(char *output, int outputSize, const char *function)
{
	string input(function);
	string output_s(VERSION);

	VLOG(2) << input;

	if (input.length() != 0) {
		if (input.front() == ':') {
			for (auto & pair : commands) {
				auto ns = input.find(pair.first);
				if (ns != string::npos && ns == 0) {
					input.replace(ns, pair.first.length(), "");
					output_s = pair.second(input);
					goto end;
				}
			}
		}
		LOG(WARNING) << "Default command" << input;
		output_s = commands[CMD_DEFAULT](input);

	}
end:
	strncpy_s(output, outputSize, output_s.c_str(), _TRUNCATE);
	VLOG(2) << output_s;
}

// Normal Windows DLL junk...
BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: {
		initializeLogger();
		int res = _wmkdir(REPLAY_CDIR);
		LOG(INFO) << "Logger initialized," << "making dir" << REPLAY_CDIR << "result=" << res;
		

		//commands[CMD_DEBUG] = commandDEBUG;
/*		commands[CMD_NEW] = commandNEW;
		commands[CMD_EVENT] = commandEVENT;
		commands[CMD_FILE] = commandFILE;
		commands[CMD_ADD] = commandADD;
		commands[CMD_DEFAULT] = commandDEFAULT;
		commands[CMD_COM] = commandCOM;
		commands[CMD_DEL] = commandDEL;*/
		//commands[CMD_VER] = commandVER;

		break;
	}

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH: {
		break;

	}
	}
	return TRUE;
}
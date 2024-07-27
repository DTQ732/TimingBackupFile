/*
	Windows 文件被修改则定时备份文件，2024年7月27日18点49分
	Debug, 完成于 2024年7月27日23点39分（中间打了三小时游戏）
*/

#ifndef _DEBUG
// 后台方式运行，退出需要在任务管理器关闭
#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )

#endif

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using namespace std;

struct Setting
{
	Setting(const string& path, int sec) 
		: Path(path)
		, Seconds(sec)
	{}

	string Path; // 备份路径
	int Seconds; // 秒数
};

void simpleLog(const string& str)
{
	static ofstream ofs("bak.log", ios::out | ios::trunc);
	(ofs << str).flush();
	return;
}

void backupFile(Setting setting)
{
	struct _stat64i32 statbuf;
	_stat64i32(setting.Path.c_str(), &statbuf);

	while (true)
	{
		struct _stat64i32 statbuf2;
		_stat64i32(setting.Path.c_str(), &statbuf2);
		// 如果最新获取的修改时间大于之前保存的时间，则备份一次

		if (statbuf.st_mtime < statbuf2.st_mtime)
		{
			statbuf = statbuf2;
			char newFileName[4096];
			struct tm t;
			localtime_s(&t, &statbuf2.st_mtime);
			sprintf_s(newFileName, 4096, "%s.%04d%02d%02d%02d%02d%02d.bak", setting.Path.c_str(), 
				t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
			
			BOOL ret = CopyFileA(setting.Path.c_str(), newFileName, FALSE);
			if (ret != 0)
			{
				simpleLog(string("copy") + " " + setting.Path + " " + newFileName + "\n");
			}

			// 以下代码防止弹出窗口，但是只能执行bat脚本
			// 经过测试win11 执行system不会新建窗口 ^_^|| 好像就是不会来着，之前用的是要执行bat脚本才会弹出窗口
			// 但是我想做成后台程序，每次执行system都会有个cmd弹出来，所以还是用Windows API去复制文件吧
			// //  && pause 管道连接顺序执行
			// 1. sprintf_s(cmd, 4096, "copy \"%s\" \"%s.%04d%02d%02d%02d%02d%02d.bak\"", setting.Path.c_str(),
			//	setting.Path.c_str(), t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
			// system(cmd);
			// 2. WinExec(cmd, SW_HIDE);
			// 3.
			// PROCESS_INFORMATION pi;
			//STARTUPINFOA si{};
			//si.lpReserved = NULL;
			//si.lpDesktop = NULL;
			//si.lpTitle = NULL;
			//si.dwFlags = STARTF_USESHOWWINDOW;
			//si.wShowWindow = SW_HIDE;
			//si.cbReserved2 = NULL;
			//si.lpReserved2 = NULL;
			//DWORD dwExitCode;
			//BOOL ret = CreateProcessA("cmd.exe", cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
			//if (ret != 0)
			//{
			//	CloseHandle(pi.hThread);
			//	WaitForSingleObject(pi.hProcess, INFINITE);
			//	GetExitCodeProcess(pi.hProcess, &dwExitCode);
			//	CloseHandle(pi.hProcess);
			//}
		}
		
		Sleep(setting.Seconds * 1000);
	}
}

int main()
{
	{
		char currentDic[4096];
		GetCurrentDirectoryA(4096, currentDic);
		// 替换currentDic原因：名称可以具有“Global”或“Local”前缀，以在全局命名空间或会话命名空间中显式创建对象。 名称的其余部分可以包含除反斜杠字符 (\) 以外的任何字符。
		replace(currentDic, currentDic + strlen(currentDic), '\\', ' ');
		HANDLE hEvent = CreateEventA(NULL, 0, 0, currentDic);
		if (hEvent == INVALID_HANDLE_VALUE || hEvent == 0)
		{
			simpleLog(to_string(GetLastError()));
			MessageBoxW(NULL, TEXT("创建防多开失败"), TEXT("Warning"), MB_OK);
			_exit(-1);
		}
		else
		{
			SetEvent(hEvent);
		}
		if (hEvent)
		{
			if (ERROR_ALREADY_EXISTS == GetLastError()) {
				MessageBoxW(NULL, TEXT("不能多开哦"), TEXT("Warning"), MB_OK);
				_exit(-1);
			}
		}
	}
	// 录入配置文件，读取待备份的文件，及备份间隔，一行文件一行秒数
	ifstream ifs("setting.txt");
	vector<string> settingStrs;

	while (true)
	{
		char line[4096];
		ifs.getline(line, 4096);
		if (strlen(line) == 0)
		{
			break;
		}
		settingStrs.push_back(line);
	}

	if (settingStrs.size() <= 0 || settingStrs.size() % 2 != 0)
	{
		// 混用一下试试
		MessageBoxW(NULL, TEXT("参数不符合格式"), TEXT("Warning"), MB_OK);
		_exit(-1);
	}

	// 解析参数，创建线程
	vector<Setting> settings;
	vector<std::thread> threads;
	for (size_t i = 0, size = settingStrs.size(); i < size; i+=2)
	{ 
		settings.push_back(Setting(settingStrs[i], atoi(settingStrs[i + 1].c_str())));
		threads.push_back(thread(backupFile, settings.back()));
	}

	for (size_t i = 0, size = threads.size(); i < size; i++)
	{
		threads[i].join();
	}
}
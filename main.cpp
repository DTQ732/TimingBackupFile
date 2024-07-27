/*
	Windows �ļ����޸���ʱ�����ļ���2024��7��27��18��49��
	Debug, ����� 2024��7��27��23��39�֣��м������Сʱ��Ϸ��
*/

#ifndef _DEBUG
// ��̨��ʽ���У��˳���Ҫ������������ر�
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

	string Path; // ����·��
	int Seconds; // ����
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
		// ������»�ȡ���޸�ʱ�����֮ǰ�����ʱ�䣬�򱸷�һ��

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

			// ���´����ֹ�������ڣ�����ֻ��ִ��bat�ű�
			// ��������win11 ִ��system�����½����� ^_^|| ������ǲ������ţ�֮ǰ�õ���Ҫִ��bat�ű��Żᵯ������
			// �����������ɺ�̨����ÿ��ִ��system�����и�cmd�����������Ի�����Windows APIȥ�����ļ���
			// //  && pause �ܵ�����˳��ִ��
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
		// �滻currentDicԭ�����ƿ��Ծ��С�Global����Local��ǰ׺������ȫ�������ռ��Ự�����ռ�����ʽ�������� ���Ƶ����ಿ�ֿ��԰�������б���ַ� (\) ������κ��ַ���
		replace(currentDic, currentDic + strlen(currentDic), '\\', ' ');
		HANDLE hEvent = CreateEventA(NULL, 0, 0, currentDic);
		if (hEvent == INVALID_HANDLE_VALUE || hEvent == 0)
		{
			simpleLog(to_string(GetLastError()));
			MessageBoxW(NULL, TEXT("�������࿪ʧ��"), TEXT("Warning"), MB_OK);
			_exit(-1);
		}
		else
		{
			SetEvent(hEvent);
		}
		if (hEvent)
		{
			if (ERROR_ALREADY_EXISTS == GetLastError()) {
				MessageBoxW(NULL, TEXT("���ܶ࿪Ŷ"), TEXT("Warning"), MB_OK);
				_exit(-1);
			}
		}
	}
	// ¼�������ļ�����ȡ�����ݵ��ļ��������ݼ����һ���ļ�һ������
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
		// ����һ������
		MessageBoxW(NULL, TEXT("���������ϸ�ʽ"), TEXT("Warning"), MB_OK);
		_exit(-1);
	}

	// ���������������߳�
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
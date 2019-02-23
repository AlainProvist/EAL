/* 
 * This file is part of the Eidolon Auto Link distribution (https://github.com/AlainProvist/EAL).
 * Copyright (c) 2019 AlainProvist.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#include <iostream>
#include <csignal>
#include <windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <malloc.h>
#include <direct.h>
#include <TlHelp32.h>
#include <Shlobj.h>
#include <string>
#include <vector>


const char* const TargetProcessName = "game.bin";

class Injector
{
public:
	Injector()
	{
		m_ProcessBlacklist.push_back(ProcessToKillInfo("aeriaignite.exe"));
		m_ProcessBlacklist.push_back(ProcessToKillInfo("netsession_win.exe"));
		m_ProcessBlacklist.push_back(ProcessToKillInfo("Steam.exe"));
	}
	~Injector() {}

	bool ReadDllListFile(const char* const path);
	void CheckBlackListProcesses();
	DWORD GetFirstGameProcessToInject();
	void InjectGameProcess(DWORD processID);
	void Cleanup();

private:
	DWORD FindProcessByName(const std::string& processName);

	std::vector<DWORD> m_GameInstanceProcessIdList;

	struct ProcessToKillInfo
	{
		enum KillState {EKS_None, EKS_ToKill, EKS_ToIgnore};

		const char* const m_Name;
		KillState m_IsToKill;
		DWORD m_LastPIDKilled;

		ProcessToKillInfo() : m_Name(""), m_IsToKill(EKS_None), m_LastPIDKilled(0) {}
		ProcessToKillInfo(const char* const name) : m_Name(name), m_IsToKill(EKS_None), m_LastPIDKilled(0) {}
	};

	std::vector< ProcessToKillInfo > m_ProcessBlacklist;

	std::vector< std::string > m_DllNames;
};


namespace
{
	BOOL IsElevated( ) 
	{
		BOOL fRet = FALSE;
		HANDLE hToken = NULL;
		if( OpenProcessToken( GetCurrentProcess( ),TOKEN_QUERY,&hToken ) ) 
		{
			TOKEN_ELEVATION Elevation;
			DWORD cbSize = sizeof( TOKEN_ELEVATION );
			if( GetTokenInformation( hToken, TokenElevation, &Elevation, sizeof( Elevation ), &cbSize ) ) 
			{
				fRet = Elevation.TokenIsElevated;
			}
		}
		if( hToken ) 
		{
			CloseHandle( hToken );
		}

		if(!fRet)
		{
			fRet = IsUserAnAdmin();
		}
		return fRet;
	}

	DWORD dwGetModuleBaseAddress(DWORD dwProcessIdentifier, TCHAR *lpszModuleName)
	{
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessIdentifier);
		DWORD dwModuleBaseAddress = 0;
		if(hSnapshot != INVALID_HANDLE_VALUE)
		{
			MODULEENTRY32 ModuleEntry32 = {0};
			ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
			if(Module32First(hSnapshot, &ModuleEntry32))
			{
				do
				{
					if(_tcscmp(ModuleEntry32.szModule, lpszModuleName) == 0)
					{
						dwModuleBaseAddress = (DWORD)ModuleEntry32.modBaseAddr;
						break;
					}
				}
				while(Module32Next(hSnapshot, &ModuleEntry32));
			}
			CloseHandle(hSnapshot);
		}
		return dwModuleBaseAddress;
	} 

	typedef BOOL (WINAPI *WriteProcessMemory_t)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);

	BOOL InjectLibrary(DWORD processID, const char *fnDll)
	{
		HANDLE hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION  | PROCESS_VM_WRITE,
			FALSE, processID);
		if (!hProcess)
		{
			return false;
		}

		BOOL success = FALSE;
		HANDLE hThread = NULL;
		char *fnRemote = NULL;
		FARPROC procLoadLibraryA = NULL;
		int count = 0;

		size_t lenFilename = strlen(fnDll) + 1;

		// Allocate space in the remote process 
		fnRemote = (char *) VirtualAllocEx(hProcess, NULL, lenFilename, MEM_COMMIT, PAGE_READWRITE);

		if(fnRemote)
		{
			// Write the filename to the remote process. 
			WriteProcessMemory_t procWriteProcessMemory = (WriteProcessMemory_t)GetProcAddress(GetModuleHandle("Kernel32"), "WriteProcessMemory");// AV false positive bypass
			if(procWriteProcessMemory(hProcess, fnRemote, fnDll, lenFilename, NULL))
			{
				// Get the address of the LoadLibraryA function 
				procLoadLibraryA = GetProcAddress(GetModuleHandle("Kernel32"), "LoadLibraryA");// AV false positive bypass
				hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) procLoadLibraryA, fnRemote, 0, NULL);
				if(hThread)
				{
					WaitForSingleObject(hThread, INFINITE);
					success = TRUE;
					CloseHandle(hThread);
				}
			}
			VirtualFreeEx(hProcess, fnRemote, 0, MEM_RELEASE);
		}

		CloseHandle(hProcess);

		return success;
	}

	BOOL EjectLibrary(DWORD processID, const char *fnDll)
	{
		HANDLE hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION  | PROCESS_VM_WRITE,
			FALSE, processID);
		if (!hProcess)
		{
			return false;
		}

		BOOL success = FALSE;
		HANDLE hSnapshot = NULL;
		HANDLE hThread = NULL;
		FARPROC procFreeLibrary = NULL;

		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);

		if(hSnapshot)
		{
			MODULEENTRY32 me = { sizeof(me) };
			BOOL isFound = FALSE;
			BOOL isMoreMods = Module32First(hSnapshot, &me);
			for(; isMoreMods; isMoreMods = Module32Next(hSnapshot, &me))
			{
				isFound = (_strcmpi(me.szModule, fnDll) == 0 || _strcmpi(me.szExePath, fnDll) == 0);
				if(isFound)
					break;
			}

			if(isFound)
			{
				// Get the address of the LoadLibraryA function 
				procFreeLibrary = GetProcAddress(GetModuleHandle("Kernel32"), "FreeLibrary");// AV false positive bypass
				hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) procFreeLibrary, me.modBaseAddr, 0, NULL);
				if(hThread)
				{
					WaitForSingleObject(hThread, INFINITE);
					success = TRUE;
					CloseHandle(hThread);
				}
			}

			CloseHandle(hSnapshot);	
		}

		CloseHandle(hProcess);

		return success;
	}

	BOOL HasLibrary(DWORD processID, const char *fnDll)
	{
		HANDLE hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION  | PROCESS_VM_WRITE,
			FALSE, processID);
		if (!hProcess)
		{
			return false;
		}

		BOOL success = FALSE;
		HANDLE hSnapshot = NULL;
		HANDLE hThread = NULL;
		FARPROC procFreeLibrary = NULL;

		hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);

		BOOL isFound = FALSE;

		if(hSnapshot)
		{
			MODULEENTRY32 me = { sizeof(me) };
			BOOL isMoreMods = Module32First(hSnapshot, &me);
			for(; isMoreMods && !isFound; isMoreMods = Module32Next(hSnapshot, &me))
			{
				isFound = (_strcmpi(me.szModule, fnDll) == 0 || _strcmpi(me.szExePath, fnDll) == 0);
			}

			CloseHandle(hSnapshot);	
		}

		CloseHandle(hProcess);

		return isFound;
	}

	BOOL KillProcess(DWORD processID)
	{
		BOOL ret = FALSE;
		HANDLE hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE,
			FALSE, processID);
		if (!hProcess)
		{
			return false;
		}

		UINT exitCode = 0;
		ret = TerminateProcess(hProcess, exitCode);

		CloseHandle(hProcess);

		return ret;
	}
}




DWORD Injector::FindProcessByName(const std::string& processName)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if( hProcessSnap == INVALID_HANDLE_VALUE )
	{
		return( FALSE );
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof( PROCESSENTRY32 );

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if( !Process32First( hProcessSnap, &pe32 ) )
	{
		CloseHandle( hProcessSnap );          // clean the snapshot object
		return( FALSE );
	}

	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		// make sure the game instance is not currently processed
		bool processed = false;
		for(UINT j = 0; j < m_GameInstanceProcessIdList.size(); ++j)
		{
			DWORD processID = m_GameInstanceProcessIdList[j];
			if(processID == pe32.th32ProcessID)
			{
				processed = true;
				break;
			}
		}
		if ( !processName.compare(pe32.szExeFile) && !processed )
		{
			CloseHandle(hProcessSnap);
			return pe32.th32ProcessID;
		}
	} while( Process32Next( hProcessSnap, &pe32 ) );

	CloseHandle( hProcessSnap );
	return( 0 );
}

bool Injector::ReadDllListFile(const char* const path)
{
	m_DllNames.clear();

	FILE* file = fopen(path, "r");
	if (!file)
		return false;

	char line[2048];

	while (fgets(line, 2048, file) != NULL) 
	{
		std::string name = line;
		if(name != "")
		{
			size_t pos = name.find(".dll");
			if(pos != std::string::npos)
				m_DllNames.push_back(name.substr(0, pos+4));
		}
	}

	fclose(file);

	return !m_DllNames.empty();
}

void Injector::CheckBlackListProcesses()
{
	for(unsigned int i = 0; i < m_ProcessBlacklist.size(); ++i)
	{
		ProcessToKillInfo& info = m_ProcessBlacklist[i];
		if(info.m_IsToKill == ProcessToKillInfo::EKS_ToIgnore)
			continue;

		DWORD processID = FindProcessByName(info.m_Name); //Get PID of Process
		if(processID != 0)
		{
			if(info.m_IsToKill == ProcessToKillInfo::EKS_ToKill)
			{
				if(processID != info.m_LastPIDKilled)
				{
					BOOL ret = KillProcess(processID);
					if(ret)
					{
						info.m_LastPIDKilled = processID;
						std::cout << info.m_Name << " (PID = " << processID << ") has been killed !" << std::endl;
					}
					else
						std::cout << info.m_Name << " (PID = " << processID << ") has failed to be killed (" << GetLastError() << ") !" << std::endl;
				}
			}
			else if(info.m_IsToKill == ProcessToKillInfo::EKS_None)
			{
				char txt[1024] = "";
				sprintf(txt, "%s is running and could interfere with the injection process. \nDo you want to kill it ?", info.m_Name);
				int msgboxID = MessageBoxA(NULL, txt, "Injection.exe", MB_YESNO | MB_ICONEXCLAMATION | MB_APPLMODAL | MB_SETFOREGROUND);
				info.m_IsToKill = (msgboxID == IDYES)? ProcessToKillInfo::EKS_ToKill : ProcessToKillInfo::EKS_ToIgnore;
				if(msgboxID == IDYES)
					std::cout << info.m_Name << " has been added to the blacklist !" << std::endl;
				else
					std::cout << info.m_Name << " will now be ignored !" << std::endl;
			}
		}
	}
}

DWORD Injector::GetFirstGameProcessToInject()
{
	DWORD processID = FindProcessByName(TargetProcessName); //Get PID of a game Process not already processed
	bool isInjected = true;
	for(UINT i = 0; i < m_DllNames.size(); ++i)
	{
		char srcDll[512]; //dll in the same directory as the injection.exe
		_getcwd(srcDll, 512);
		strcat_s(srcDll, "\\");
		strcat_s(srcDll, m_DllNames[i].c_str());

		if(!HasLibrary(processID, srcDll))// missing dll
		{
			isInjected = false;// try inject it
			break;
		}
	}

	return processID;
}

void Injector::InjectGameProcess(DWORD processID)
{
	bool allInjected = true;
	for(UINT i = 0; i < m_DllNames.size(); ++i)
	{
		char srcDll[512]; //dll in the same directory as the injection.exe
		_getcwd(srcDll, 512);
		sprintf(srcDll, "%s\\%s", srcDll, m_DllNames[i].c_str());

		if(!HasLibrary(processID, srcDll))
		{
			if (InjectLibrary(processID, srcDll))
			{
				std::cout << "Injection successful : " << srcDll << std::endl;
			}
			else
			{
				allInjected = false;
				std::cout << "Injection failed ("<< GetLastError() << ") : " << srcDll << std::endl;
			}
		}
		else
		{
			std::cout << "Dll " << srcDll << " already injected !" << std::endl;
		}
	}
	if(allInjected)
		m_GameInstanceProcessIdList.push_back(processID);
	std::cout << std::endl;
}

void Injector::Cleanup()
{
	for(UINT j = 0; j < m_GameInstanceProcessIdList.size(); ++j)
	{
		DWORD processID = m_GameInstanceProcessIdList[j];

		for(UINT i = m_DllNames.size(); i > 0; --i)
		{
			char srcDll[512]; //dll in the same directory as the injection.exe
			_getcwd(srcDll, 512);
			strcat_s(srcDll, "\\");
			strcat_s(srcDll, m_DllNames[i-1].c_str());

			BOOL ejectionDone = FALSE;
			while (!ejectionDone)
			{
				if(EjectLibrary(processID, srcDll))
				{
					if (!HasLibrary(processID, srcDll))
					{
						ejectionDone = true;
						printf("Ejection successful : %s\n", srcDll);// std::cout won't work from the callback call used there...
					}
					else// in case it was referenced more than one... what unfortunatly happens each time you start a thread from the dll...
					{
						printf("Ejection failed : %s\nRetrying...\n", srcDll);
					}
				}
				else
				{
					ejectionDone = true;
					printf("Ejection impossible (%d) : %s\n", GetLastError(), srcDll);
				}
			}
		}
	}
}


static Injector injector;

void OnExit();
void OnSignal(int sig);
int _tmain(int argc, _TCHAR* argv[])
{
	if(!IsElevated())
	{
		printf("Please restart the program with admin rights.\n");

		Sleep(3000);
		return 0;
	}

	if(!injector.ReadDllListFile("dlls.txt"))
	{
		printf("No dll to inject found in \"dlls.txt\" ! Exiting...\n");

		Sleep(3000);
		return 0;
	}

	int ret = atexit(OnExit);
	signal(SIGINT, OnSignal);
	signal(SIGTERM, OnSignal);
#ifdef SIGBREAK
	signal(SIGBREAK, OnSignal);
#endif

	while(1)
	{
		DWORD processID = 0;
		std::cout << "Waiting for game start..." << std::endl;
		while(processID == 0)// while no game to inject
		{
			injector.CheckBlackListProcesses();// monitor the blacklist

			processID = injector.GetFirstGameProcessToInject();

			Sleep(100);
		}
		std::cout << "Game found : PID = " << processID << std::endl;
		Sleep(500);

		injector.InjectGameProcess(processID);

		Sleep(100);
	}

	return 0;
}

void OnExit()
{
	injector.Cleanup();

	printf("All dlls ejected from running targets. Now Exiting ...\n");

	Sleep(3000);
}

void OnSignal(int sig)
{
	OnExit();
}

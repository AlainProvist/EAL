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


#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <cstdio>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>

#include "Vector3.h"
#include "Matrix3.h"

#include "utils.h"
#include "objects.h"
#include "memorybrowser.h"
#include "dllprotec.h"

#include <vector>
#include <algorithm>
#include <locale>

#pragma warning( disable : 4731 ) 


void *SetDetour(u8 *source, const u8 *destination, u32 length);
void UnSetDetour(u8 *source, const u8 *destination, u32 length, u8 *tunnel);

unsigned int __stdcall HookThread(void* pArguments);
unsigned int __stdcall UnHookThread(void* pArguments);

#define DECLARE_HOOK(Name) VOID hkHandle##Name##(VOID);\
typedef VOID(*Handle##Name##_t)(VOID);\
Handle##Name##_t pHandle##Name##;\
FARPROC dwHandle##Name## = NULL;\
FARPROC dwHandle##Name##Ret = NULL;

#define DEFINE_HOOK(Name) VOID hkHandle##Name##()

#define INSTALL_HOOK(Name, Size, Offset) dwHandle##Name = (FARPROC) ((u32)Offset);\
dwHandle##Name##Ret = (FARPROC) ((u32)Offset + Size);\
pHandle##Name = (Handle##Name##_t) SetDetour((pu8) dwHandle##Name, (pu8) hkHandle##Name, Size);

#define UNINSTALL_HOOK(Name, Size) UnSetDetour((u8 *)dwHandle##Name, (u8 *)hkHandle##Name, Size, (u8 *)pHandle##Name);\
pHandle##Name = NULL;

DECLARE_HOOK(MainLoop)
DECLARE_HOOK(CrashHandler)


u32 PID = 0;
static volatile bool exitThread = false;
bool invalidTarget = false;
volatile HANDLE hUserThread = NULL, hGameThread = NULL;
DeclareScopeLock(GameThreadSet);

enum SafeExitBitField 
{
	SEBF_OK			= 0, 
	SEBF_MainLoop	= 0x1,
	SEBF_Interface	= 0x2
};
volatile u32 safeExitFlags = SEBF_OK;
DeclareScopeLock(SafeExitFlags);

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HANDLE hModule, u32 lpReason, LPVOID lpReserved)
{
	switch (lpReason)
	{
	case DLL_PROCESS_ATTACH:
		//MessageBox(NULL, "DLL_PROCESS_ATTACH", "DLL", MB_OK);
		// attach to process
		{
			hUserThread = (HANDLE)_beginthreadex(NULL, 0, &HookThread, NULL, 0, NULL);
			hDllModule = hModule;
			DllModuleSize = GetModuleSize(reinterpret_cast<size_t>(hDllModule));
			//std::cout << "DLL_PROCESS_ATTACH" << std::endl;
		}
		break;
	case DLL_PROCESS_DETACH:
		//MessageBox(NULL, "DLL_PROCESS_DETACH", "DLL", MB_OK);
		// detach from process
		{
			// request our thread to stop
			{
				UseScopeLock(GameThreadSet);
				exitThread = true;
			}

			// wait for our thread and hooks to end before unhooking everything
			bool readyToExit = false;
			while(!readyToExit)
			{
				{
					UseScopeLock(SafeExitFlags);
					readyToExit = (safeExitFlags == SEBF_OK);
				}
				if(!readyToExit)
				{
					u32 dwExitCode = 0;
					GetExitCodeThread(hUserThread, &dwExitCode);
					readyToExit = (dwExitCode == 0);
				}
				Sleep( 100 );
			}

			UnHookThread(0);
			CloseHandle(hUserThread);

			DllProtecEnd();
			//std::cout << "DLL_PROCESS_DETACH" << std::endl;
		}
		break;
	case DLL_THREAD_ATTACH:
		//MessageBox(NULL, "DLL_THREAD_ATTACH", "DLL", MB_OK);
		// attach to thread
		//std::cout << "DLL_THREAD_ATTACH" << std::endl;
		break;
	case DLL_THREAD_DETACH:
		//MessageBox(NULL, "DLL_THREAD_DETACH", "DLL", MB_OK);
		// detach from thread
		//std::cout << "DLL_THREAD_DETACH" << std::endl;
		break;
	}
	return TRUE; // successful
}

unsigned int __stdcall HookThread(void* pArguments)
{
	{
		UseScopeLock(SafeExitFlags);
		safeExitFlags |= SEBF_Interface;
	}
	PID = GetCurrentProcessId( );

	RetrieveAddresses();


	DllProtec();

	if(!invalidTarget)
	{
        INSTALL_HOOK(MainLoop, 7, DETOUR_MAIN_LOOP_OFFSET);
        INSTALL_HOOK(CrashHandler, 6, DETOUR_CRASH_HANDLER_OFFSET);
	}

	srand((u32)time(NULL)+GetCurrentThreadId());

	
	while(!exitThread)
    {
		// TODO : some UI window can safely update here

		// Monitor the game window for closing
        {
            UseScopeLock(GameThreadSet);
            if(hGameThread && !exitThread)
            {
                u32 dwExitCode = 0;
                GetExitCodeThread(hGameThread, &dwExitCode);
                exitThread = (dwExitCode == 0);
            }
        }

        Sleep( 100 );
    }

	{
		UseScopeLock(SafeExitFlags);
		safeExitFlags &= ~SEBF_Interface;
	}
	_endthreadex( 0 );
	return 0; // exit current thread
}

unsigned int __stdcall UnHookThread(void* pArguments)
{
	if(invalidTarget)
		return S_OK;

    UNINSTALL_HOOK(MainLoop, 7);
    UNINSTALL_HOOK(CrashHandler, 6);
	
	return S_OK; // exit current thread
}

void *SetDetour(u8 *source, const u8 *destination, u32 length)
{
	u32 const jmpLength(5);
	u32 const nopOpcode(0x90);
	u32 const jmpOpcode(0xE9);
	u32 const pushaOpcode(0x60);
	u32 const popaOpcode(0x61);

	if (length < jmpLength+1) 
		length = jmpLength+1; // Make sure the patch's length is long enough to hold a 32bit JMP + a pusha
	u32 tunnelLength = length + jmpLength + 1; // + popa
	u8 *tunnel  = new u8[tunnelLength]; // Create a body for the "tunnel" function.
	FillMemory(tunnel, tunnelLength, 0);
	u32 oldProtection(NULL); // Old page protection.
	VirtualProtect(source, length, PAGE_EXECUTE_READWRITE, &oldProtection);
	tunnel[0] = popaOpcode;
	memcpy(tunnel+1, source, length);
	FillMemory(source, length, nopOpcode);// erase source opcode
	source[0] = pushaOpcode;
	source[1] = jmpOpcode;
	tunnel[length+1] = jmpOpcode;
	*(u32*)(source + 1+1) = (u32)(destination - (source+jmpLength+1)); // JMP Offset 1
	*(u32*)(tunnel + 1 + length+1) = (u32)((source+jmpLength+1) - (tunnel+jmpLength+length+1)); // JMP Offset 2
	VirtualProtect(source, length, oldProtection, &oldProtection);
	return tunnel;
}

void UnSetDetour(u8 *source, const u8 *destination, u32 length, u8 *tunnel)
{
	u32 const jmpLength(5);
	
	if (length < jmpLength+1) 
		length = jmpLength+1; // Make sure the patch's length is long enough to hold a 32bit JMP.
	u32 oldProtection(NULL); // Old page protection.
	VirtualProtect(source, length, PAGE_EXECUTE_READWRITE, &oldProtection);
	memcpy(source, tunnel+1, length);// copy back the original opcode
	VirtualProtect(source, length, oldProtection, &oldProtection);
	delete[] tunnel;
}

// main update loop detour
DEFINE_HOOK(MainLoop)
{
	// multi client hack check
	if(!exitThread)
	{
		{
			UseScopeLock(SafeExitFlags);
			safeExitFlags |= SEBF_MainLoop;
		}


		// Setup multiclient hack if not already done
		static bool multiClientHackDone = false;
		if(!multiClientHackDone)
		{
			HANDLE hMutex = NULL;
			__asm
			{
				mov eax, [ebp+4h];
				mov hMutex, eax;
			}
			if(hMutex != NULL)
			{
				HANDLE testMutex = OpenMutexA(MUTEX_ALL_ACCESS, FALSE, "FFClientTag");
				if(testMutex != NULL)
				{
					BOOL ok;
					ok = ReleaseMutex(testMutex);
					ok = CloseHandle(testMutex);

					ok = ReleaseMutex(hMutex);
					ok = CloseHandle(hMutex);
					multiClientHackDone = true;
				}
			}
		}

		{
			UseScopeLock(GameThreadSet);
			if(!hGameThread)
			{
				hGameThread = GetCurrentThread();
			}
		}


		{
			static long long last = milliseconds_now();

			long long now = milliseconds_now();
			float dt = (float)(now - last)*0.001f;
			if(dt > 1.0f)
				dt = 1.0f;
			last = now;

			// TODO : hotkey handling if needed
			{
				u32 dwProcId = NULL;
				if (GetAsyncKeyState(VK_F10) & 1)
				{
					HWND dwWindow = GetForegroundWindow();
					if (GetWindowThreadProcessId(dwWindow, &dwProcId) &&
						dwProcId == PID)
					{

					}
				}
			}

			// Bot updates from main loop (can use game code threadsafely)
			{
				UpdateEudemons(dt);
			}
		}
	}

	{
		UseScopeLock(SafeExitFlags);
		safeExitFlags &= ~SEBF_MainLoop;
	}

	// rebuild the stack exactly as if we exited this function normally, then restore the registers, 
	// rewrite the original instructions removed by our jump and finally jump back to the instruction after where we jumped
	__asm
	{
		mov esp,ebp;
		pop ebp;
		popad;
		cmp [esi+108h], 0;// instruction replaced by our jump
		jmp dwHandleMainLoopRet;
	}

	// Never ever execute anything after the previous asm code !!!
}

#include <winnt.h>
DEFINE_HOOK(CrashHandler) 
{
	_EXCEPTION_POINTERS *exceptionInfo;
	__asm
	{
		mov eax, [ebp];
		mov ecx, [eax+0x8];
		mov exceptionInfo, ecx;
	}

	HandleCrash(exceptionInfo);

	// rebuild the stack exactly as if we exited this function normally, then restore the registers, 
	// rewrite the original instructions removed by our jump and finally jump back to the instruction after where we jumped
	// immediate return
	__asm
	{
		mov esp,ebp;
		pop ebp;
		popad;
		mov eax, 1;// bypass the original code
		mov esp,ebp;
		pop ebp;
		retn 4;
	}

	// Never ever execute anything after the previous asm code !!!
}



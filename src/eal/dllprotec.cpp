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


#include "dllprotec.h"

#include "logger.h"
#include "version.h"

#include <iostream>
#include <windows.h>
#include <WindowsX.h>
#include <tchar.h>
#include <malloc.h>
#include <direct.h>
#include <TlHelp32.h>
#include <string>
#include <vector>

#include <intrin.h> 
#pragma intrinsic(_ReturnAddress)


HANDLE hDllModule = NULL;
size_t DllModuleSize = 0;

typedef BOOL (WINAPI *Module32Next_t)(HANDLE, LPMODULEENTRY32);
Module32Next_t Module32Next_Unhook;

typedef NTSTATUS (NTAPI *NtQueryVirtualMemory_t)(HANDLE, PVOID, DWORD, PVOID, ULONG, PULONG);
NtQueryVirtualMemory_t NtQueryVirtualMemory_Unhook;

typedef HANDLE (WINAPI *CreateThread_t)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
CreateThread_t CreateThread_Unhook;

size_t GetModuleSize (size_t address)
{
	IMAGE_DOS_HEADER& dosHeader = *reinterpret_cast<PIMAGE_DOS_HEADER>(address);
	IMAGE_NT_HEADERS& ntHeader = *reinterpret_cast<PIMAGE_NT_HEADERS>(address + dosHeader.e_lfanew);
	return ntHeader.OptionalHeader.SizeOfImage;
}

BOOL CheckDebugger()
{
	BOOL IsDebugged = FALSE;

	__asm
	{
		MOV EAX,DWORD PTR FS:[18] ; //pointeur sur TEB
		MOV EAX,DWORD PTR DS:[EAX+30]; //pointeur sur PEB
		MOVZX EAX,BYTE PTR DS:[EAX+2] ; //IsDebugged ?
		MOV IsDebugged, EAX;
	}

	return IsDebugged; /*TRUE si debug, FALSE autrement*/
}

void ResetDebugger()
{
	__asm
	{
		MOV EAX,DWORD PTR FS:[18] ; //pointeur sur TEB
		MOV EAX,DWORD PTR DS:[EAX+30]; //pointeur sur PEB
		MOV BYTE PTR DS:[EAX+2], 0;
	}
}

BOOL WINAPI Module32Next_Hook (HANDLE snapshot, LPMODULEENTRY32 moduleEntry)
{
	//WarningMessage("Calling Module32Next.");
	MODULEENTRY32 entry = {0};
	entry.dwSize = sizeof(MODULEENTRY32);
	if(!Module32Next_Unhook(snapshot, &entry))
	{
		//WarningMessage("Module32Next : no more entries.");
		return FALSE;
	}

	if(entry.modBaseAddr == (BYTE*)hDllModule)
	{
		WarningMessage("Module32Next tried to access this module.");
		WarningMessage(StringFormat("Function returns to %p", _ReturnAddress()).c_str());
		if(!Module32Next_Unhook(snapshot, &entry))
		{
			//WarningMessage("Module32Next : no more entries.");
			return FALSE;
		}
	}

	//WarningMessage(StringFormat("Module32Next : %p", entry.modBaseAddr).c_str());
	memcpy(moduleEntry, &entry, sizeof(entry));
	return TRUE;
}

NTSTATUS NTAPI NtQueryVirtualMemory_Hook (HANDLE processHandle, PVOID baseAddress, DWORD memoryInformationClass, PVOID buffer, ULONG length, PULONG resultLength)
{
	DWORD errorCode = NtQueryVirtualMemory_Unhook(processHandle, baseAddress, memoryInformationClass, buffer, length, resultLength);
	if(reinterpret_cast<size_t>(baseAddress) >= reinterpret_cast<size_t>(hDllModule) && reinterpret_cast<size_t>(baseAddress) < reinterpret_cast<size_t>(hDllModule) + DllModuleSize)
	{
#ifdef LOG_WARNINGS
		WarningMessage("NtQueryVirtualMemory tried to access our module.");
		PVOID line;
		__asm 
		{
			call NextLine;
		NextLine:
			pop eax;
			mov line, eax;
		}
		WarningMessage(StringFormat("Function returns to %p (currently %p)", _ReturnAddress(), line).c_str());
#endif

		errorCode = NtQueryVirtualMemory_Unhook(processHandle, (PVOID)(reinterpret_cast<size_t>(baseAddress) + DllModuleSize), memoryInformationClass, buffer, length, resultLength);
	}

	return errorCode;
}

HANDLE WINAPI CreateThread_Hook (LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
	WarningMessage(StringFormat("CreateThread called with start address %p.", lpStartAddress).c_str());
	const u32 length = 16;
	u8 opcode[length+1] = "\x83\xEC\x20\x53\x56\x57\x89\x65\xF0\x33\xF6\x39\x75\x08\x75\x05";
	bool doExit = (memcmp((void*)((u32)lpStartAddress+0xA), opcode, length) == 0);

	if(doExit)
	{
		// gameguard thread : do not allow it to start (started each time you select your character and enter the game, and stopped when you return to login screen)
		// it is preventing dll injection in the game, preventing dbg to be attached and is probably doing other crappy stuff
		WarningMessage("CreateThread tried to start inquisition code.");
		WarningMessage(StringFormat("Function returns to %p.", _ReturnAddress()).c_str());
		return NULL;
	}
	return CreateThread_Unhook(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
}

void* SetHook(void* baseAddr, void* hookAddr)
{
	DWORD oldprotect = 0;

	// alloc new space for the trampoline
	byte* newregion = (byte*) VirtualAlloc(0, 10, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	// unprotect prologue
	VirtualProtect(baseAddr, 5, PAGE_EXECUTE_READWRITE, &oldprotect);

	memcpy( newregion, baseAddr, 5);

	byte* t = (byte*)baseAddr;
	*t = 0xe9; // jmp
	t++;
	//jmp relative to our function
	*(DWORD*) t = ((DWORD)hookAddr - (DWORD) t - 4);

	// restore prologues protection
	VirtualProtect(baseAddr, 5, oldprotect, &oldprotect);

	t = newregion + 5;
	*t = 0xe9;
	t++;
	*(DWORD*) t = ( (DWORD) baseAddr - (DWORD) t + 1);

	// we have to set protection to PAGE_EXECUTE_READ
	VirtualProtect(newregion, 10, PAGE_EXECUTE_READ, 0);

	// this is the pointer to function that we will call from the hook
	return (void*)newregion;
}

void UnsetHook(void* addr, void* original)
{
	// get functions address
	DWORD oldprotect = 0;

	VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &oldprotect);

	memcpy( addr, original, 5);

	VirtualProtect(addr, 5, oldprotect, &oldprotect);

	VirtualFree(original, 10, MEM_RELEASE);
	original = NULL;
}

void DllProtec()
{//return;
	Module32Next_Unhook = (Module32Next_t)SetHook(GetProcAddress(GetModuleHandle("Kernel32.dll"), "Module32Next"), Module32Next_Hook);

	NtQueryVirtualMemory_Unhook = (NtQueryVirtualMemory_t)SetHook(GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryVirtualMemory"), NtQueryVirtualMemory_Hook);

	CreateThread_Unhook = (CreateThread_t)SetHook(GetProcAddress(GetModuleHandle("Kernel32.dll"), "CreateThread"), CreateThread_Hook);
}

void DllProtecEnd()
{//return;
	UnsetHook(GetProcAddress(GetModuleHandle("Kernel32.dll"), "Module32Next"), Module32Next_Unhook);

	UnsetHook(GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryVirtualMemory"), NtQueryVirtualMemory_Unhook);

	UnsetHook(GetProcAddress(GetModuleHandle("Kernel32.dll"), "CreateThread"), CreateThread_Unhook);
}



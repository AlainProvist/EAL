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


#include "memorybrowser.h"

#include "utils.h"
#include "logger.h"

#include <string>

bool GetProductAndVersion(VS_FIXEDFILEINFO& fixedInfoOut);

const char* const MemoryPatternTableStrings[GAID_MAX] = 
{
	"GET_LOCAL_PLAYER",					

	"INVENTORY_ACCESS_FUNCTION",		

	"TARGETING_COLLECTIONS_BASE",		

	"WND_INTERFACE_BASE",				

	"EUDEMON_GETEUDEMON_FUNCTION",	
	"EUDEMON_SENDCOMMAND_FUNCTION",	
	"EUDEMON_SELECT_FUNCTION",
	"EUDEMON_ISMEDITATING_FUNCTION",
	"EUDEMON_HASGIFT_FUNCTION",

	"CURRENT_MAP_BASE",
	
	"DETOUR_MAIN_LOOP_OFFSET",			
	"DETOUR_CRASH_HANDLER_OFFSET"
};


MemorySearchEntry MemoryPatternTable[GAID_MAX] = 
{
	// GET_LOCAL_PLAYER :				C0 89 87 04 01 // OK (search for s90351)
	MemorySearchEntry((u8*)"\xC0\x89\x87\x04\x01",		"xxxxx",	-0x5,	2, MemorySearchEntry::RT_REL_ADDRESS), 

	// INVENTORY_ACCESS_FUNCTION :		FF 5E 5D C2 14 // OK
	MemorySearchEntry((u8*)"\xFF\x5E\x5D\xC2\x14",		"xxxxx",	-0xD,	2, MemorySearchEntry::RT_REL_ADDRESS), 

	// TARGETING_COLLECTIONS_BASE :		68 50 06 00 00 // OK ++
	MemorySearchEntry((u8*)"\x68\x50\x06\x00\x00",		"xxxxx",	0x30,	1, MemorySearchEntry::RT_ADDRESS), 

	// WND_INTERFACE_BASE :				4E 24 83 B9 B0 // OK +++
	MemorySearchEntry((u8*)"\x4E\x24\x83\xB9\xB0",		"xxxxx",	0x1F,	1, MemorySearchEntry::RT_ADDRESS), 

	// EUDEMON_GETEUDEMON_FUNCTION :	DB 0F 84 5E 01 // OK (search for call of SET_NEAREST_TARGET_FUNCTION)
	MemorySearchEntry((u8*)"\xDB\x0F\x84\x5E\x01",		"xxxxx",	-0x7,	1, MemorySearchEntry::RT_REL_ADDRESS), 
	// EUDEMON_SENDCOMMAND_FUNCTION :	6A 04 6A 00 52 // OK +++
	MemorySearchEntry((u8*)"\x6A\x04\x6A\x00\x52",		"xxxxx",	0x6,	1, MemorySearchEntry::RT_REL_ADDRESS), 
	// EUDEMON_SELECT_FUNCTION :		FB FF 56 8B F1 // OK
	MemorySearchEntry((u8*)"\xFB\xFF\x56\x8B\xF1",		"xxxxx",	-0x21,	1, MemorySearchEntry::RT_LOCATION), 
	// EUDEMON_ISMEDITATING_FUNCTION :	3B 01 7D 1A 0F // OK (search in EUDEMON_SELECT_FUNCTION)
	MemorySearchEntry((u8*)"\x3B\x01\x7D\x1A\x0F",		"xxxxx",	-0x11,	1, MemorySearchEntry::RT_LOCATION), 
	// EUDEMON_HASGIFT_FUNCTION :		3B 01 7D 1A 0F // OK (search in EUDEMON_SELECT_FUNCTION)
	MemorySearchEntry((u8*)"\x3B\x01\x7D\x1A\x0F",		"xxxxx",	-0x11,	2, MemorySearchEntry::RT_LOCATION), 

	// CURRENT_MAP_BASE :				C0 74 0D 83 3D // OK
	MemorySearchEntry((u8*)"\xC0\x74\x0D\x83\x3D",		"xxxxx",	-0xA,	1, MemorySearchEntry::RT_ADDRESS),
	
	// DETOUR_MAIN_LOOP_OFFSET :		FF 80 BE 08 01 // OK
	MemorySearchEntry((u8*)"\xFF\x80\xBE\x08\x01",		"xxxxx",	0x1,	1, MemorySearchEntry::RT_LOCATION, true, (u8*)"\x80\xBE\x08\x01\x00\x00\x00"),	
	// DETOUR_CRASH_HANDLER_OFFSET :	4D EC 51 6A 05 // OK (TopLevelExceptionFilter)
	MemorySearchEntry((u8*)"\x4D\xEC\x51\x6A\x05",		"xxxxx",	-0x56,	1, MemorySearchEntry::RT_LOCATION, true, (u8*)"\x64\xA1\x00\x00\x00\x00")
};

FILETIME DateTimeCreation;

bool Match(const u8* pData, const u8* bMask, const char* szMask)
{
	for(;*szMask;++szMask,++pData,++bMask)
		if(*szMask=='x' && *pData!=*bMask ) 
			return false;
	return (*szMask) == NULL;
}


u32 FindPattern(u32 dwAddress, u32 dwLen, const u8* const bMask, const char* const szMask, u32 occurence)
{
	if(occurence == 0)
		return 0;

	for(u32 i=0; i <= dwLen - strlen(szMask); ++i, ++dwAddress)
		if( Match( (u8*)dwAddress,bMask,szMask) )
			if((--occurence) == 0)
				return dwAddress;

	return 0;
}

bool CheckOpCode(const u8* pData, const u8* check)
{
	if(check == (const u8*)"")
		return true;

	for(;*check;++check,++pData)
		if(*pData!=*check) 
			return false;

	return true;
}

static VS_FIXEDFILEINFO FixedInfo = {0};     // pointer to fixed file info structure
bool RetrieveAddresses(bool fastCheck/* = false*/)
{
	HMODULE hmod = GetModuleHandle((LPCSTR)"game.bin");
	if(hmod == 0)
		return false;
	MEMORY_BASIC_INFORMATION info;
	// Start at PE32 header
	SIZE_T len = VirtualQuery(hmod, &info, sizeof(info));
	u8* processBase = (u8*)info.AllocationBase;
	u8* address = processBase;
	SIZE_T size = 0;
	u32 count = 0;
	for (;;++count) 
	{
		len = VirtualQuery(address, &info, sizeof(info));
		if (info.AllocationBase != processBase) 
			break;
		address = (u8*)info.BaseAddress + info.RegionSize;
		size += info.RegionSize;
	}

	GetProductAndVersion(FixedInfo);

	if(fastCheck)
		return true;

	bool errorFound = false;
	for(int i = 0; i < GAID_MAX; ++i)
	{
		MemorySearchEntry& entry = MemoryPatternTable[i];
		u32 dwaddy = FindPattern((u32)processBase, (u32)size, entry.Pattern, entry.PatternMask, entry.Occurence);
		if(dwaddy == 0)
		{
			entry.Address = 0;
			errorFound = true;
			continue;
		}
		dwaddy += entry.Offset;
		if(entry.ReadType == MemorySearchEntry::RT_ADDRESS)
		{
			dwaddy = *(u32*)dwaddy;
		}
		else if(entry.ReadType == MemorySearchEntry::RT_REL_ADDRESS)
		{
			if(entry.CheckResult)
			{
				const u8* check = (u8*)"\xE8"/*CALL*/;
				if(!CheckOpCode((u8*)(dwaddy-1), check))
				{
					entry.Address = 0;
					errorFound = true;
					continue;
				}
			}
			u32 addr = *(u32*)dwaddy;
			addr += (dwaddy + 4);
			dwaddy = addr;
		}
		else if(entry.ReadType == MemorySearchEntry::RT_LOCATION)
		{
			if(entry.CheckResult)
			{
				const u8* check = (entry.OpCodeCheck == (u8*)"")? (const u8*)"\x55\x8B\xEC"/*PUSH EBP; MOV EBP, ESP;*/ : entry.OpCodeCheck;
				if(!CheckOpCode((u8*)dwaddy, check))
				{
					entry.Address = 0;
					errorFound = true;
					continue;
				}
			}
		}

		entry.Address = dwaddy;
	}

	return !errorFound;
}


bool GetProductAndVersion(VS_FIXEDFILEINFO& fixedInfoOut)
{
	// get the filename of the executable containing the version resource
	char szFilename[MAX_PATH + 1] = {0};
	if (GetModuleFileName(NULL, szFilename, MAX_PATH) == 0)
	{
		//TRACE("GetModuleFileName failed with error %d\n", GetLastError());
		return false;
	}

	// allocate a block of memory for the version info
	DWORD dummy;
	DWORD dwSize = GetFileVersionInfoSize(szFilename, &dummy);
	if (dwSize == 0)
	{
		//TRACE("GetFileVersionInfoSize failed with error %d\n", GetLastError());
		return false;
	}
	std::vector<BYTE> data(dwSize);

	// load the version info
	if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0]))
	{
		//TRACE("GetFileVersionInfo failed with error %d\n", GetLastError());
		return false;
	}

	VS_FIXEDFILEINFO* pFixedInfo = NULL;
	UINT uiVerLen = 0;
	// get the fixed file info (language-independend) 
	if(VerQueryValue(&data[0], TEXT("\\"), (void**)&pFixedInfo, (UINT *)&uiVerLen) == 0)
	{
		return false;
	}
	memcpy(&fixedInfoOut, pFixedInfo, sizeof(VS_FIXEDFILEINFO));

	return true;
}

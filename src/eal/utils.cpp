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


#include "utils.h"

#include "logger.h"

#include <math.h>
#include <vector>
#include <algorithm>


WindowManager* WindowManager::ms_Instance = NULL;


double GetTime()
{
	static LARGE_INTEGER liPerformanceFrequency;
	static BOOL s_use_qpc = QueryPerformanceFrequency( &liPerformanceFrequency);

	LARGE_INTEGER liPerformanceCount;
	QueryPerformanceCounter( &liPerformanceCount);
	double dTime = double(liPerformanceCount.QuadPart)/double(liPerformanceFrequency.QuadPart);

	return dTime;
}

long long milliseconds_now() 
{
	static LARGE_INTEGER s_frequency;
	static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
	if (s_use_qpc) 
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (1000LL * now.QuadPart) / s_frequency.QuadPart;
	} 
	else 
	{
		return GetTickCount();
	}
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
std::string GetDllPath()
{
	char dllPath[_MAX_PATH];
	::GetModuleFileName((HINSTANCE)&__ImageBase, dllPath, _MAX_PATH);
	std::string path = dllPath;
	std::string::size_type pos = path.rfind('\\');
	path = path.substr(0, pos+1);

	return path;
}

size_t* ThreadSafeReadAddress(size_t* addr, size_t offset)
{
	u32 OldProtect;
	VirtualProtect((LPVOID)addr, sizeof(size_t), PAGE_EXECUTE_READWRITE, &OldProtect);
	size_t* ret = (size_t*)*(size_t*)((size_t)addr+offset);
	VirtualProtect((LPVOID)addr, sizeof(size_t), OldProtect, &OldProtect);

	return ret;
}

CurrentMap* GetCurrentMap(u32 lpBase/* = CURRENT_MAP_BASE*/)
{
	CurrentMap* cw = (CurrentMap*)*(u32*)lpBase;
	return cw;
}

Entity* GetLocalPlayer(u32 lpBase/* = TARGETING_COLLECTIONS_BASE*/, u32 lpFunction/* = GET_LOCAL_PLAYER*/)
{
	Entity* localPlayer = NULL;
	__asm
	{
		mov eax, lpBase;
		mov ecx, ds:[eax];
		test ecx, ecx;
		jz Finnish;
		call lpFunction;
		mov localPlayer, eax;
Finnish:
	}
	if(localPlayer && localPlayer->entityID == 0)
		return NULL;

	return localPlayer;
}

MainPlayerInfo* GetLocalPlayerInfo()
{
	Entity* localPlayer = GetLocalPlayer();
	EntityInfo* data = localPlayer? localPlayer->info : NULL;

	return (MainPlayerInfo*)data;
}

EntityCollection* GetEntityCollection(EntityCollectionType type, u32 lpBase/* = TARGETING_COLLECTIONS_BASE*/)
{
	size_t* addr = (size_t*)lpBase;
	if (addr)
		addr = ThreadSafeReadAddress(addr, 0);
	if (!addr)
		return NULL;
	addr = (size_t*)((size_t)addr + 0x51C + type * sizeof(EntityCollection));

	return (EntityCollection*)addr;
}

bool IsInGame()
{
	CurrentMap* curMap = GetCurrentMap();
	if (curMap && curMap->mapID > 0 && curMap->mapID != 0x63/*char selection*/)
	{
		EntityCollection* ec = GetEntityCollection(ECT_Chara);
		if (ec && ec->container)
		{
			Entity* locPlayer = GetLocalPlayer();
			if (locPlayer)
				return locPlayer->info && locPlayer->model && locPlayer->actor && locPlayer->info->charName != "";
		}
	}
	return false;
}

u32 GetInventoryAccessPtr()
{
	MainPlayerInfo* info = GetLocalPlayer()? (MainPlayerInfo*)GetLocalPlayer()->info : NULL;
	void* res = info? info->inventoryPtr : NULL;
	return (u32)res;
}

InventoryBag* GetInventoryBag( u32 bagId, u32 inventoryType/* = IT_BackPack*/, u32 lpFunction/* = INVENTORY_ACCESS_FUNCTION */)
{
	u32 thisPtr = GetInventoryAccessPtr();
	if(!thisPtr)
		return NULL;

	InventoryBag* bag = NULL;
	__asm
	{
		mov ecx, thisPtr;//mov edx, thisPtr;

		push bagId;//mov ecx, bagId;
		push inventoryType;//mov esi, inventoryType;
		call lpFunction;
		mov bag, eax;
	}
}

u32 GetInventorySize()
{
	u32 count = 0;
	for(u32 i = 0; i <= IBT_MAX; ++i)
	{
		InventoryBag* bag = GetInventoryBag(i, IT_BackPack);
		if(bag)
			count += bag->GetItemCount();
	}
	return count;
}

bool GetEmptySlots( std::vector<u32>& slotIds, u32 inventoryType/* = IT_BackPack*/)
{
	slotIds.clear();

	u32 count = 0;
	for(u32 i = 0; i <= IBT_MAX; ++i)
	{
		InventoryBag* bag = GetInventoryBag(i, inventoryType);

		if(bag)
		{
			u32 newCount = bag->GetItemCount();
			for(u32 j = 0; j < newCount; ++j)
			{
				InventoryItem* item = bag->GetItem(j);
				if(!item || !item->itemID)
					slotIds.push_back(count + j);
			}
			count += newCount;
		}
	}
	return !slotIds.empty();
}

Eudemon* GetEudemonBySlot(int slotID, u32 fct/* = EUDEMON_GETEUDEMON_FUNCTION*/)
{
	if(slotID < 0 || slotID > 3)
		return NULL;

	Eudemon* eudemon = NULL;
	Entity* localPlayer = GetLocalPlayer();
	__asm
	{
		movsx edi, slotID;
		push edi;
		mov ecx, localPlayer;
		call fct;
		mov eudemon, eax;
	}

	return eudemon;
}

bool TryEudemonAction(int slotID, EudemonAction action, u32 fct/* = EUDEMON_SENDCOMMAND_FUNCTION*/)
{
	static u32 bestMessageIDs[3] = {0x36, 0xd8, 0xd9};
	u32 arg1 = 0;

	Eudemon* eudemon = GetEudemonBySlot(slotID);
	if(!eudemon)
		return false;

	switch(action)
	{
	case EA_TALK:
		if(eudemon->chatAttempts == 0)
			return false;
		LogMessage(Eidolons, StringFormat("Talking to eidolon at slot %d.", slotID + 1));
		arg1 = bestMessageIDs[rand()%3];
		break;
	case EA_MEDITATION:
		if(eudemon->currentPM < 10)
			return false;
		LogMessage(Eidolons, StringFormat("Linking eidolon at slot %d.", slotID + 1));
		break;
	case EA_RETRIEVE:
		LogMessage(Eidolons, StringFormat("Retrieving object from eidolon at slot %d.", slotID + 1));
		break;
	}

	__asm
	{
		mov eax, eudemon;
		mov eax, [eax];// eudemon ID
		push 0;
		push action;
		push arg1;
		push eax;
		call fct;
		add esp, 10h;
	}

	return true;
}

bool IsEudemonMeditating(int slotID, u32 lpFunction/* = EUDEMON_ISMEDITATING_FUNCTION*/)
{
	Entity* localPlayer = GetLocalPlayer();
	if(!localPlayer)
		return false;

	bool isMeditating = false;

	__asm
	{
		mov			eax, slotID;
		push		eax;
		mov         ecx, localPlayer;
		call        lpFunction;
		mov			isMeditating, al;
	}

	return isMeditating;
}

bool HasEudemonGift(int slotID, u32 lpFunction/* = EUDEMON_HASGIFT_FUNCTION*/)
{
	Entity* localPlayer = GetLocalPlayer();
	if(!localPlayer)
		return false;

	bool hasGift = false;

	__asm
	{
		mov			eax, slotID;
		push		eax;
		mov         ecx, localPlayer;
		call        lpFunction;
		mov			hasGift, al;
	}

	return hasGift;
}

void SelectEudemon(int slotID, EudemonWindow* ei, u32 fct/* = EUDEMON_SELECT_FUNCTION*/)
{
	if(ei)
	{
		__asm
		{
			mov			ecx, ei;
			push        slotID; 
			call        fct;
		}
	}
}

void UpdateEudemons(float dt)
{
	if(!IsInGame() || GetLocalPlayerInfo()->level <= 1)
		return;

	return;
	EudemonWindow* ew = (EudemonWindow*)WindowManager::GetWindowByName("EudemonExtendWnd");
	if(ew && !ew->unkPtr)
	{
		SelectEudemon(0, ew);
	}

	{
		static float time = 0.0f;
		time += dt;

		if(time > 1.0f)
		{
			time = 0.0f;

			for(u32 i = 0; i < 4; ++i)
			{
				if(!IsEudemonMeditating(i))
				{
					if(HasEudemonGift(i))
					{
						bool isInventoryFull = false;
						std::vector<u32> emptySlots;
						GetEmptySlots(emptySlots);
						isInventoryFull = emptySlots.empty();

						if(isInventoryFull)
							continue;
						else
							if(TryEudemonAction(i, EA_RETRIEVE))
								break;
					}
					if(!TryEudemonAction(i, EA_TALK))
					{
						if(TryEudemonAction(i, EA_MEDITATION))
							break;// only 1 meditation start at a time
					}
					else// 1 talk only at a time
						break;
				}
			}
		}
	}
}


#include "imagehlp.h"
#pragma comment(lib,"Dbghelp.lib")
const char* const crashDumpFilename = "crashdump.txt";
static void PrintStack( void )
{
	unsigned int   i;
	void         * stack[ 100 ];
	unsigned short frames;
	SYMBOL_INFO  * symbol;
	HANDLE         process;

	process = GetCurrentProcess();
	SymInitialize( process, NULL, TRUE );
	frames               = CaptureStackBackTrace( 0, 100, stack, NULL );
	symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
	symbol->MaxNameLen   = 255;
	symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

	for( i = 0; i < frames; i++ )
	{
		if(SymFromAddr( process, (DWORD)stack[i], 0, symbol ))
			LogInFile(StringFormat( "%i: %s - 0x%08X", frames - i - 1, symbol->Name, symbol->Address ), crashDumpFilename);
		else
			LogInFile(StringFormat( "%i: 0x%08X", frames - i - 1, (DWORD)stack[i] ), crashDumpFilename);
	}
	LogInFile("", crashDumpFilename);

	free( symbol );
}

void MakeMinidump(EXCEPTION_POINTERS* e)
{
	std::string path = GetDllPath();

	std::string longname = path + "crashdump.dmp";

	HANDLE hFile = CreateFileA(longname.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(hFile == INVALID_HANDLE_VALUE)
		return;

	MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
	exceptionInfo.ThreadId = GetCurrentThreadId();
	exceptionInfo.ExceptionPointers = e;
	exceptionInfo.ClientPointers = FALSE;

	BOOL dumped = MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
		e ? &exceptionInfo : NULL,
		NULL,
		NULL);

	CloseHandle(hFile);

	return;
}


void HandleCrash(_EXCEPTION_POINTERS* exceptionInfo)
{
	//PrintStack();
	MakeMinidump(exceptionInfo);

	MessageBoxA( NULL, "Please send us your crashdump.dmp to help us fix this crash.", "EAL just crashed !", NULL );
}

void DumpMemoryRegion(void* address, u32 size, const std::string& filename, u32 lineSize/* = 0x10*/)
{
	std::string dump = "";
	std::string hexview = "";
	std::string charview = "";
	u8* ptr = (u8*)address;
	for(u32 j = 0; j < size; ++j, ++ptr)
	{
		hexview += StringFormat("%02X%s", *ptr, (j%4 == 3)? "   ":" ");
		charview += (*ptr < 0x20)? '.' : (char)*ptr;
		if(j%lineSize == lineSize-1)
		{
			dump += hexview;
			dump += charview;
			dump += "\n";
			hexview = "";
			charview = "";
		}
	}
	dump += "\n";
	LogInFile(dump, filename);
}

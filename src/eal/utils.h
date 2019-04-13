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


#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <vector>
#include <string>
#include "objects.h"
#include "Vector3.h"
#include "Matrix3.h"

#include "version.h"
#include "memorybrowser.h"


extern MemorySearchEntry MemoryPatternTable[GAID_MAX];

#define GET_LOCAL_PLAYER				MemoryPatternTable[GAID_GET_LOCAL_PLAYER].Address 

#define INVENTORY_ACCESS_FUNCTION		MemoryPatternTable[GAID_INVENTORY_ACCESS_FUNCTION].Address  

#define TARGETING_COLLECTIONS_BASE		MemoryPatternTable[GAID_TARGETING_COLLECTIONS_BASE].Address  

#define WND_INTERFACE_BASE				MemoryPatternTable[GAID_WND_INTERFACE_BASE].Address  



#define EUDEMON_GETEUDEMON_FUNCTION		MemoryPatternTable[GAID_EUDEMON_GETEUDEMON_FUNCTION].Address 
#define EUDEMON_SENDCOMMAND_FUNCTION	MemoryPatternTable[GAID_EUDEMON_SENDCOMMAND_FUNCTION].Address 
#define EUDEMON_SELECT_FUNCTION			MemoryPatternTable[GAID_EUDEMON_SELECT_FUNCTION].Address
#define EUDEMON_ISMEDITATING_FUNCTION	MemoryPatternTable[GAID_EUDEMON_ISMEDITATING_FUNCTION].Address
#define EUDEMON_HASGIFT_FUNCTION		MemoryPatternTable[GAID_EUDEMON_HASGIFT_FUNCTION].Address

#define CURRENT_MAP_BASE				MemoryPatternTable[GAID_CURRENT_MAP_BASE].Address


#define DETOUR_MAIN_LOOP_OFFSET			MemoryPatternTable[GAID_DETOUR_MAIN_LOOP_OFFSET].Address 
#define DETOUR_CRASH_HANDLER_OFFSET		MemoryPatternTable[GAID_DETOUR_CRASH_HANDLER_OFFSET].Address

class Mutex
{
public:
	Mutex() 
	{
		HANDLE hMutex0 = CreateMutexA(NULL, FALSE, "AKMutex00");// if 2 mutex are created at the same time, one will be locked by hMutex0
		WaitForSingleObject(hMutex0, INFINITE);

		static int id = 1;
		char name[128] = "";
		sprintf(name, "AKMutex%02d", id);
		m_Name = name;
		m_hMutex = CreateMutexA(NULL, FALSE, name);
		++id;

		ReleaseMutex(hMutex0);
		CloseHandle(hMutex0);
	}
	~Mutex() 
	{
		CloseHandle(m_hMutex);
	}

	bool Lock() {return (WaitForSingleObject(m_hMutex, INFINITE) == WAIT_OBJECT_0);}
	bool Unlock() {return (ReleaseMutex(m_hMutex) == TRUE);}
private:
	HANDLE m_hMutex;
	std::string m_Name;
};

class LockObject
{
public:
	LockObject() 
	{
		InitializeCriticalSection(&m_CS);
	}

	void Lock() {EnterCriticalSection(&m_CS);}
	void Unlock() {LeaveCriticalSection(&m_CS);}
private:
	CRITICAL_SECTION m_CS;
};

struct CriticalSectionObject
{
	CriticalSectionObject()
	{
		InitializeCriticalSection(&CS);
	}
	~CriticalSectionObject()
	{
		DeleteCriticalSection(&CS);
	}
	CRITICAL_SECTION CS;
};

class ScopeLock
{
public:
	ScopeLock(CriticalSectionObject& cso/*, const char* const str*/) : m_CSO(cso)/*, m_Name(str)*/
	{
		EnterCriticalSection(&(m_CSO.CS));
	}

	~ScopeLock()
	{
		LeaveCriticalSection(&(m_CSO.CS));
	}
private:
	CriticalSectionObject& m_CSO;
	//std::string m_Name;
};

#define DeclareScopeLock(name) static CriticalSectionObject CSO_##name
#define UseScopeLock(name) ScopeLock SL_##name(CSO_##name/*, #name*/);


double GetTime();
long long milliseconds_now();

std::string GetDllPath();

size_t* ThreadSafeReadAddress(size_t* addr, size_t offset);

CurrentMap* GetCurrentMap(u32 lpBase = CURRENT_MAP_BASE);

Entity* GetLocalPlayer( u32 lpBase = TARGETING_COLLECTIONS_BASE, u32 lpFunction = GET_LOCAL_PLAYER );
MainPlayerInfo* GetLocalPlayerInfo();
EntityCollection* GetEntityCollection(EntityCollectionType type, u32 lpBase = TARGETING_COLLECTIONS_BASE);
bool IsInGame();


InventoryBag* GetInventoryBag( u32 bagId, u32 inventoryType = IT_BackPack, u32 lpFunction = INVENTORY_ACCESS_FUNCTION );
u32 GetInventorySize();
bool GetEmptySlots( std::vector<u32>& slotIds, u32 inventoryType = IT_BackPack);


Eudemon* GetEudemonBySlot(int slotID, u32 fct = EUDEMON_GETEUDEMON_FUNCTION);
bool TryEudemonAction(int slotID, EudemonAction action, u32 fct = EUDEMON_SENDCOMMAND_FUNCTION);
void SelectEudemon(int slotID, EudemonWindow* ei, u32 fct = EUDEMON_SELECT_FUNCTION);
bool IsEudemonMeditating(int slotID, u32 lpFunction = EUDEMON_ISMEDITATING_FUNCTION);
bool HasEudemonGift(int slotID, u32 lpFunction = EUDEMON_HASGIFT_FUNCTION);
void UpdateEudemons(float dt);



class WindowManager
{
public:
	struct WindowElement
	{
		WindowElement* next;
		WindowElement* last;
		std::string wndName;// 0x8
		AKWindow* wnd;// 0x24
	};

	static WindowManager* GetInstance()
	{
		if(!ms_Instance && WND_INTERFACE_BASE)
			ms_Instance = (WindowManager*)WND_INTERFACE_BASE;
		return ms_Instance;
	}

	static AKWindow* GetWindowByName(const char* const wndName)
	{
		WindowManager* instance = GetInstance();
		if(!instance || !instance->m_Container)
			return NULL;
		WindowElement* it = instance->m_Container->begin;
		while((u32*)it != (u32*)instance->m_Container)
		{
			if(it->wndName == wndName)
				return it->wnd;
			it = it->next;
		}
		return NULL;
	}

	static AKWindow* GetActiveWindow()
	{
		WindowManager* instance = GetInstance();
		if(!instance || !instance->m_Container)
			return NULL;
		return instance->m_Container->last? instance->m_Container->last->wnd : NULL;
	}

	static AKWindow* GetWindowByIndex(u32 index)
	{
		u32 i = 0;
		WindowManager* instance = GetInstance();
		if(!instance || !instance->m_Container)
			return NULL;
		WindowElement* it = instance->m_Container->begin;
		while((u32*)it != (u32*)instance->m_Container)
		{
			if(i == index)
				return it->wnd;
			it = it->next;
			++i;
		}
		return NULL;
	}

private:
	WindowManager() {}
	~WindowManager() {}

	struct WindowElementContainer
	{
		WindowElement* begin;
		WindowElement* last;
	};

	WindowElementContainer *m_Container;

	static WindowManager* ms_Instance;
};


void HandleCrash(_EXCEPTION_POINTERS* exceptionInfo);

void DumpMemoryRegion(void* address, u32 size, const std::string& filename, u32 lineSize = 0x10);

#endif // UTILS_H
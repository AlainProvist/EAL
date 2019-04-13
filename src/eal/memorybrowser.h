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


#ifndef MEMORYBROWSER_H
#define MEMORYBROWSER_H

#include<stdio.h> 
#include<windows.h>

#include "objects.h"
#include "version.h"

enum GameAddressId
{
	GAID_GET_LOCAL_PLAYER,

	GAID_INVENTORY_ACCESS_FUNCTION,

	GAID_TARGETING_COLLECTIONS_BASE,

	GAID_WND_INTERFACE_BASE,

	GAID_EUDEMON_GETEUDEMON_FUNCTION,
	GAID_EUDEMON_SENDCOMMAND_FUNCTION,
	GAID_EUDEMON_SELECT_FUNCTION,
	GAID_EUDEMON_ISMEDITATING_FUNCTION,
	GAID_EUDEMON_HASGIFT_FUNCTION,

	GAID_CURRENT_MAP_BASE,
	
	GAID_DETOUR_MAIN_LOOP_OFFSET,
	GAID_DETOUR_CRASH_HANDLER_OFFSET,
	
	GAID_MAX
};

// ONLY USE THIS STRUCTURE IN STATIC MEMORY (no string copy from ctor)
struct MemorySearchEntry
{
	enum ReadType {RT_LOCATION, RT_ADDRESS, RT_REL_ADDRESS};
	MemorySearchEntry() : Pattern((u8*)""), PatternMask(""), Offset(0), Occurence(0), ReadType(RT_LOCATION), CheckResult(true), OpCodeCheck((u8*)""), Address(0) {}
	MemorySearchEntry(const u8* const pattern, const char* const patternMask, s32 offset = 0, u32 occurence = 1, ReadType readType = RT_LOCATION, bool checkResult = true, const u8* const opCodeCheck = (u8*)"")
		: Pattern(pattern), PatternMask(patternMask), Offset(offset), Occurence(occurence), ReadType(readType), CheckResult(checkResult), OpCodeCheck(opCodeCheck), Address(0) {}

	const u8* const Pattern;
	const char* const PatternMask;
	s32 Offset;
	u32 Occurence;
	ReadType ReadType;
	bool CheckResult;
	const u8* const OpCodeCheck;

	u32 Address;
};

bool RetrieveAddresses(bool fastCheck = false);



#endif

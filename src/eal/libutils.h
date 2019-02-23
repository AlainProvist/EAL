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


#ifndef LIBUTILS_H
#define LIBUTILS_H

#include <stdio.h>
#include <vector>
#include "Vector3.h"

#ifdef __cplusplus
extern "C" {
#endif
	typedef signed long	long	s64,	*ps64;
	typedef unsigned long long	u64,	*pu64;

	typedef signed long			s32,	*ps32;
	typedef unsigned long		u32,	*pu32;

	typedef signed short		s16,	*ps16;
	typedef unsigned short		u16,	*pu16;

	typedef signed char			s8,		*ps8;
	typedef unsigned char		u8,		*pu8;
#ifdef __cplusplus
}
#endif

std::string StringFormat(const char * fmt,...);
std::string GetUTF8GameString(const std::string& gameStr);


struct EntityInfo
{
	u32 unk1;//0
	u32 unk2;//4
	u32 currentHP;//8
	u32 cash;//C
	u32 level;//10
	u8 unk3[0x454];//14
};


struct MainPlayerInfo : public EntityInfo
{
	u8 unk[0xEC]; // 0x468
	void* inventoryPtr; // 0x554
};

struct Entity
{
	u32 unk1;
	u32 unk2;
	u32 entityID;//8
	EntityInfo *info;//C
};

enum InventoryType {IT_BackPack = 0, IT_Equipment = 1, IT_Bank = 4, IT_BackPack_Bags = 5, IT_EudemonInventory = 6};
// for IT_BackPack and IT_Bank
enum InventoryBagType 
{
	IBT_MainBag	= 0, 
	IBT_Bag1	= 1, 
	IBT_Bag2	= 2, 
	IBT_Bag3	= 3, 
	IBT_Bag4	= 4, 
	IBT_Bag5	= 5, 
	IBT_Bag6	= 6, 
	IBT_Bag7	= 7, 
	IBT_Bag8	= 8, 
	IBT_Bag9	= 9, 
	IBT_Bag10	= 10, 
	IBT_Bag11	= 11, 
	IBT_Bag12	= 12, 
	IBT_Bag13	= 13, 
	IBT_Bag14	= 14, 
	IBT_Bag15	= 15,
	IBT_MAX		= 15
};


struct InventoryItem // size = 0x150
{
	u32 itemID;
	u8 unk[0x14C];// 4
};


struct Eudemon // size = 0x160 ?
{
	u32 eudemonPtr;
	u16 slotIdx;// 4
	u16 unk1;
	u8 unk2[0x124];// 8
	u32 currentPM;// 12C
	u16 chatAttempts;// 130
	u16 unk3;	
	u8 unk4[0x2C];// 134
};

enum EudemonAction{EA_TALK = 1, EA_MEDITATION = 2, EA_RETRIEVE = 4};


class AKWindow
{
public:
    u32 unk1;
    std::string wndName;// 0x4
};


#endif // LIBUTILS_H

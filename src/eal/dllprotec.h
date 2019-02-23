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


/*This code was originally written to defeat the gameguard used on the US client of the game (but it seems it's not more used...), and also prevent any code to detect our module in the game process.*/

#ifndef DLLPROTEC_H
#define DLLPROTEC_H

#include <windows.h>


extern HANDLE hDllModule;
extern size_t DllModuleSize;

size_t GetModuleSize (size_t address);

BOOL CheckDebugger();
void ResetDebugger();

void DllProtec();
void DllProtecEnd();

#endif // DLLPROTEC_H
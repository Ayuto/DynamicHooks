/**
* =============================================================================
* DynamicHooks
* Copyright (C) 2015 Robin Gohmert. All rights reserved.
* =============================================================================
*
* This software is provided 'as-is', without any express or implied warranty.
* In no event will the authors be held liable for any damages arising from 
* the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose, 
* including commercial applications, and to alter it and redistribute it 
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not 
* claim that you wrote the original software. If you use this software in a 
* product, an acknowledgment in the product documentation would be 
* appreciated but is not required.
*
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any source distribution.
*
* asm.h/cpp from devmaster.net (thanks cybermind) edited by pRED* to handle gcc
* -fPIC thunks correctly
*
* Idea and trampoline code taken from DynDetours (thanks your-name-here).
*/

#ifndef _X86_MS_CDECL_H
#define _X86_MS_CDECL_H

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "../convention.h"


// ============================================================================
// >> CLASSES
// ============================================================================
/*
*/
class x86MsCdecl: public ICallingConvention
{	
public:
	x86MsCdecl(std::vector<DataType_t> vecArgTypes, DataType_t returnType, int iAlignment=4);

	virtual std::list<Register_t> GetRegisters();
	virtual int GetPopSize();
	
	virtual void* GetArgumentPtr(int iIndex, CRegisters* pRegisters);
	virtual void* GetReturnPtr(CRegisters* pRegisters);
};

#endif // _X86_MS_CDECL_H
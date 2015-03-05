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

// ============================================================================
// >> INCLUDES
// ============================================================================
#include "x86MsCdecl.h"


// ============================================================================
// >> x86MsCdecl
// ============================================================================
x86MsCdecl::x86MsCdecl(std::vector<DataType_t> vecArgTypes, DataType_t returnType, int iAlignment) : 
	ICallingConvention(vecArgTypes, returnType, iAlignment)
{
}

std::list<Register_t> x86MsCdecl::GetRegisters()
{
	std::list<Register_t> registers;

	registers.push_back(ESP);

	if (m_returnType == DATA_TYPE_FLOAT || m_returnType == DATA_TYPE_DOUBLE)
		registers.push_back(ST0);
	else
		registers.push_back(EAX);

	return registers;
}

int x86MsCdecl::GetPopSize()
{
	return 0;
}

void* x86MsCdecl::GetArgumentPtr(int iIndex, CRegisters* pRegisters)
{
	int iOffset = 4;
	for(int i=0; i < iIndex; i++)
	{
		DataType_t type = m_vecArgTypes[i];
		iOffset += GetDataTypeSize(type, m_iAlignment);
	}

	return (void *) (pRegisters->m_esp->GetValue<unsigned long>() + iOffset);
}

void* x86MsCdecl::GetReturnPtr(CRegisters* pRegisters)
{
	if (m_returnType == DATA_TYPE_FLOAT || m_returnType == DATA_TYPE_DOUBLE)
		return pRegisters->m_st0;

	return pRegisters->m_eax;
}
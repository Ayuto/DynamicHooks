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
#include "x86MsThiscall.h"


// ============================================================================
// >> x86MsCdecl
// ============================================================================
x86MsThiscall::x86MsThiscall(std::vector<DataType_t> vecArgTypes, DataType_t returnType, int iAlignment) : 
	ICallingConvention(vecArgTypes, returnType, iAlignment)
{
}

std::list<Register_t> x86MsThiscall::GetRegisters()
{
	std::list<Register_t> registers;
	
	registers.push_back(ESP);
	registers.push_back(ECX);

	if (m_returnType == DATA_TYPE_FLOAT || m_returnType == DATA_TYPE_DOUBLE)
	{
		registers.push_back(ST0);
	}
	else
	{
		registers.push_back(EAX);
		if (GetDataTypeSize(m_returnType, m_iAlignment) > 4)
		{
			registers.push_back(EDX);
		}
	}

	return registers;
}

int x86MsThiscall::GetPopSize()
{
	int iPopSize = 0;

	for(unsigned int i=1; i <= m_vecArgTypes.size() - 1; i++)
	{
		iPopSize += GetDataTypeSize(m_vecArgTypes[i], m_iAlignment);
	}

	return iPopSize;
}

void* x86MsThiscall::GetArgumentPtr(int iIndex, CRegisters* pRegisters)
{
	if (iIndex == 0)
	{
		return pRegisters->m_ecx->m_pAddress;
	}

	int iOffset = 4;
	for(int i=1; i < iIndex; i++)
	{
		iOffset += GetDataTypeSize(m_vecArgTypes[i], m_iAlignment);
	}

	return (void *) (pRegisters->m_esp->GetValue<unsigned long>() + iOffset);
}

void* x86MsThiscall::GetReturnPtr(CRegisters* pRegisters)
{
	// TODO: Handle integers > 4 bytes
	if (m_returnType == DATA_TYPE_FLOAT || m_returnType == DATA_TYPE_DOUBLE)
		return pRegisters->m_st0->m_pAddress;

	return pRegisters->m_eax->m_pAddress;
}
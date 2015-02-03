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
#include "manager.h"


// ============================================================================
// >> CHookManager
// ============================================================================
CHook* CHookManager::HookFunction(void* pFunc, int iPopSize, std::list<Register_t> vecRegistersToSave)
{
	if (!pFunc)
		return NULL;

	CHook* pHook = FindHook(pFunc);
	if (pHook)
		return pHook;
	
	pHook = new CHook(pFunc, iPopSize, vecRegistersToSave);
	m_Hooks.push_back(pHook);
	return pHook;
}

void CHookManager::UnhookFunction(void* pFunc)
{
	CHook* pHook = FindHook(pFunc);
	if (pHook)
	{
		m_Hooks.remove(pHook);
		delete pHook;
	}
}

CHook* CHookManager::FindHook(void* pFunc)
{
	if (!pFunc)
		return NULL;

	for(std::list<CHook *>::iterator it=m_Hooks.begin(); it != m_Hooks.end(); it++)
	{
		CHook* pHook = *it;
		if (pHook->m_pFunc == pFunc)
			return pHook;
	}
	return NULL;
}

void CHookManager::UnhookAllFunctions()
{
	for(std::list<CHook *>::iterator it=m_Hooks.begin(); it != m_Hooks.end(); it++)
		delete *it;

	m_Hooks.clear();
}


// ============================================================================
// >> GetHookManager
// ============================================================================
CHookManager* GetHookManager()
{
	static CHookManager* s_pManager = new CHookManager;
	return s_pManager;
}
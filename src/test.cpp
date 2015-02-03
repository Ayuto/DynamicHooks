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
#include <iostream>
using namespace std;

#include "manager.h"


// ============================================================================
// >> HELPER FUNCTIONS
// ============================================================================
#ifdef _WIN32
void pause()
{
	system("pause");
}
#endif

// ============================================================================
// >> cdecl test
// ============================================================================
int MyFunc(int x, int y)
{
	cout << x << " Orignal func " << y << endl;
	return x + y;
}

bool MyHook(HookType_t eHookType, CHook* pHook)
{
	cout << "MyHook " << eHookType << endl;
	cout << pHook->m_pRegisters->GetValue<int>(pHook->m_pRegisters->m_esp, 8) << endl;
	cout << pHook->m_pRegisters->GetValue<int>(pHook->m_pRegisters->m_esp, 4) << endl;
	//pHook->m_pRegisters->SetValue<int>(pHook->m_pRegisters->m_eax, 1337);
	return false;
}

// ============================================================================
// >> thiscall test
// ============================================================================
class Entity
{
public:
	int AddHealth(int x, int y)
	{
		cout << x << " Orignal func " << y << endl;
		return x + y;
	}
};

bool MyHook2(HookType_t eHookType, CHook* pHook)
{
	cout << "MyHook2" << endl;
	cout << pHook->m_pRegisters->GetValue<int>(pHook->m_pRegisters->m_esp, 8) << endl;
	cout << pHook->m_pRegisters->GetValue<int>(pHook->m_pRegisters->m_esp, 4) << endl;
	return false;
}


// ============================================================================
// >> main
// ============================================================================
#ifdef __linux
#define __thiscall
#endif

int main()
{
	CHookManager* pHookMngr = GetHookManager();
	CHook* pHook = NULL;

	/*
		CDECL test
	*/
	cout << "CDECL test" << endl;

	std::list<Register_t> registers;
	registers.push_back(ESP);
	registers.push_back(EAX);

	pHook = pHookMngr->HookFunction((void *) &MyFunc, 0, registers);
	pHook->AddCallback(HOOKTYPE_PRE, (HookHandlerFn *) (void *) &MyHook);
	pHook->AddCallback(HOOKTYPE_POST, (HookHandlerFn *) (void *) &MyHook);

	cout << "End result: " << MyFunc(3, 10) << endl << endl;
	
	/*
		THISCALL test
	*/
	cout << "THISCALL test" << endl;
	int (__thiscall Entity::*func)(int, int) = &Entity::AddHealth;
	void* pFunc = (void *&) func;

	std::list<Register_t> regs;
	regs.push_back(ESP);
	regs.push_back(EAX);
	regs.push_back(ECX);

	pHook = pHookMngr->HookFunction(pFunc, 8, regs);
	pHook->AddCallback(HOOKTYPE_PRE, (HookHandlerFn *) (void *) &MyHook2);
	pHook->AddCallback(HOOKTYPE_POST, (HookHandlerFn *) (void *) &MyHook2);

	Entity e;
	cout << "End result: " << e.AddHealth(3, 10) << endl << endl;

	/*
		Clean Up
	*/
	pHookMngr->UnhookAllFunctions();
	pause();
	return 0;
}

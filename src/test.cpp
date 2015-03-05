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
#include "conventions/x86MsCdecl.h"


// ============================================================================
// >> HELPER FUNCTIONS
// ============================================================================
void pause()
{
#ifdef _WIN32
	system("pause");
#endif
}

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
	cout << " 8: " << pHook->m_pRegisters->m_esp->GetPointerValue<int>(8) << endl;
	cout << " 4: " << pHook->m_pRegisters->m_esp->GetPointerValue<int>(4) << endl;
	cout << "Return value: " << pHook->m_pRegisters->m_eax->GetValue<int>() << endl;
	
	cout << "Arg 0: " << pHook->GetArgument<int>(0) << endl;
	cout << "Arg 1: " << pHook->GetArgument<int>(1) << endl;
	cout << "Return value: " << pHook->GetReturnValue<int>() << endl;;
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
	cout << "12: " << pHook->m_pRegisters->m_esp->GetPointerValue<int>(12) << endl;
	cout << " 8: " << pHook->m_pRegisters->m_esp->GetPointerValue<int>(8) << endl;
	cout << " 4: " << pHook->m_pRegisters->m_esp->GetPointerValue<int>(4) << endl;
	cout << "Return value: " << pHook->m_pRegisters->m_eax->GetValue<int>() << endl;
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

	// Prepare registers
	std::list<Register_t> registers;
	registers.push_back(ESP);
	registers.push_back(EAX);

	std::vector<DataType_t> vecArgType;
	vecArgType.push_back(DATA_TYPE_INT);
	vecArgType.push_back(DATA_TYPE_INT);

	// Hook function
	pHook = pHookMngr->HookFunction((void *) &MyFunc, new x86MsCdecl(vecArgType, DATA_TYPE_INT));
	pHook->AddCallback(HOOKTYPE_PRE, (HookHandlerFn *) (void *) &MyHook);
	pHook->AddCallback(HOOKTYPE_POST, (HookHandlerFn *) (void *) &MyHook);

	// Call the function
	cout << "End result: " << MyFunc(3, 10) << endl << endl;

	/*
		THISCALL test
	*/
	cout << "THISCALL test" << endl;

	int (__thiscall Entity::*func)(int, int) = &Entity::AddHealth;
	void* pFunc = (void *&) func;
	
	// Prepare registers
	std::list<Register_t> regs;
	regs.push_back(ESP);
	regs.push_back(EAX);
	regs.push_back(ECX);

#ifdef __linux__
	int iPopSize = 0;
#else
	int iPopSize = 8;
#endif
	
	/*
	// Hook function
	pHook = pHookMngr->HookFunction(pFunc, iPopSize, regs);
	pHook->AddCallback(HOOKTYPE_PRE, (HookHandlerFn *) (void *) &MyHook2);
	pHook->AddCallback(HOOKTYPE_POST, (HookHandlerFn *) (void *) &MyHook2);
	
	// Call the function
	Entity e;
	cout << "End result: " << e.AddHealth(3, 10) << endl << endl;
	cout << "this pointer: " << (int) &e << endl;
	*/

	/*
		Clean Up
	*/
	pHookMngr->UnhookAllFunctions();
	pause();
	return 0;
}

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
#include "conventions/x86MsThiscall.h"
#include "conventions/x86GccCdecl.h"
#include "conventions/x86GccThiscall.h"


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
int g_iRet = 0;

int MyFunc(int x, int y)
{
	cout << x << " Orignal func " << y << endl;
	if (g_iRet == 0)
	{
		cout << "Calling MyFunc again..." << endl;
		g_iRet = 1;
		MyFunc(1, 2);
		cout << "Called" << endl;
	}

	return x + y;
}

void* g_pRetAddr = NULL;
int g_iCounter = 0;

bool MyHook(HookType_t eHookType, CHook* pHook)
{
	cout << "MyHook " << eHookType << endl;
	cout << "Arg 0: " << pHook->GetArgument<int>(0) << endl;
	cout << "Arg 1: " << pHook->GetArgument<int>(1) << endl;
	cout << "Return value: " << pHook->GetReturnValue<int>() << endl;
	/*
	g_iCounter++;
	if (!g_pRetAddr)
		g_pRetAddr = pHook->m_pRetAddr;
	else if (g_iCounter == 4)
	{
		//
		pHook->m_pRetAddr = g_pRetAddr;
	}
	*/
	
	pHook->SetReturnValue<int>(1337);
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
		cout << x << " Orignal thiscall  func " << y << endl;

		// Enable this line if the ecx register shouldn't be overwritten
		z = this;
		return x + y;
	}

public:
	void* z;
};

bool MyHook2(HookType_t eHookType, CHook* pHook)
{
	cout << "MyHook2 " << eHookType << endl;
	cout << "   8: " << pHook->m_pRegisters->m_esp->GetPointerValue<int>(8) << endl;
	cout << "   4: " << pHook->m_pRegisters->m_esp->GetPointerValue<int>(4) << endl;
	//cout << "this: " << pHook->m_pRegisters->m_ecx->GetValue<int>() << endl;
	
	cout << "Arg 0: " << pHook->GetArgument<int>(0) << endl;
	cout << "Arg 1: " << pHook->GetArgument<int>(1) << endl;
	cout << "Arg 2: " << pHook->GetArgument<int>(2) << endl;
	cout << "Return value: " << pHook->GetReturnValue<int>() << endl;
	
	pHook->SetReturnValue<int>(1337);
	return false;
}

// ============================================================================
// >> cdecl test
// ============================================================================
void MyFunc2()
{
	puts("Original function");
	if (g_iCounter == 0)
	{
		puts("Calling again");
		g_iCounter++;
		MyFunc2();
	}
	
}

bool MyHook3(HookType_t eHookType, CHook* pHook)
{
	cout << "MyHook3 " << eHookType << endl;
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

	// Prepare calling convention
	std::vector<DataType_t> vecArgTypes;
	vecArgTypes.push_back(DATA_TYPE_INT);
	vecArgTypes.push_back(DATA_TYPE_INT);

	// Hook function
	pHook = pHookMngr->HookFunction((void *) &MyFunc, new x86MsCdecl(vecArgTypes, DATA_TYPE_INT));
	pHook->AddCallback(HOOKTYPE_PRE, (HookHandlerFn *) (void *) &MyHook);
	pHook->AddCallback(HOOKTYPE_POST, (HookHandlerFn *) (void *) &MyHook);

	// Call the function
	cout << "End result: " << MyFunc(3, 10) << endl << endl;
	
    
	
	std::vector<DataType_t> vecArgTypes3;

	// Hook function
	pHook = pHookMngr->HookFunction((void *) &MyFunc2, new x86MsCdecl(vecArgTypes3, DATA_TYPE_VOID));
	pHook->AddCallback(HOOKTYPE_PRE, (HookHandlerFn *) (void *) &MyHook3);
	pHook->AddCallback(HOOKTYPE_POST, (HookHandlerFn *) (void *) &MyHook3);

	MyFunc2();
	puts("END");

	/*
		THISCALL test
	*/
	
	cout << "THISCALL test" << endl;

	int (__thiscall Entity::*func)(int, int) = &Entity::AddHealth;
	void* pFunc = (void *&) func;
	
	// Prepare calling convention
	std::vector<DataType_t> vecArgTypes2;
	vecArgTypes2.push_back(DATA_TYPE_POINTER);
	vecArgTypes2.push_back(DATA_TYPE_INT);
	vecArgTypes2.push_back(DATA_TYPE_INT);

	// Hook function
	pHook = pHookMngr->HookFunction(pFunc, new x86MsThiscall(vecArgTypes2, DATA_TYPE_INT));
	pHook->AddCallback(HOOKTYPE_PRE, (HookHandlerFn *) (void *) &MyHook2);
	pHook->AddCallback(HOOKTYPE_POST, (HookHandlerFn *) (void *) &MyHook2);
	
	// Call the function
	Entity e;
	cout << "this pointer: " << (int) &e << endl;
	cout << "End result: " << e.AddHealth(3, 10) << endl << endl;
	

	/*
		Clean Up
	*/
	pause();
	pHookMngr->UnhookAllFunctions();
	pause();
	return 0;
}

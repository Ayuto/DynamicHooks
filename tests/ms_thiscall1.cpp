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
#include "assert.h"

#include "manager.h"
#include "conventions/x86MsThiscall.h"


// ============================================================================
// >> GLOBAL VARIABLES
// ============================================================================
int g_iMyFuncCallCount = 0;
int g_iPreMyFuncCallCount = 0;
int g_iPostMyFuncCallCount = 0;


// ============================================================================
// >> cdecl test
// ============================================================================
class MyClass;
MyClass* g_pMyClass = NULL;

class MyClass
{
public:
	int MyFunc(int x, int y)
	{
		g_iMyFuncCallCount++;
		assert(this == g_pMyClass);
		assert(x == 3);
		assert(y == 10);

		int result = x + y;
		assert(result == 13);

		return result;
	}
};

bool PreMyFunc(HookType_t eHookType, CHook* pHook)
{
	g_iPreMyFuncCallCount++;
	MyClass* pMyClass = pHook->GetArgument<MyClass *>(0);
	assert(pMyClass == g_pMyClass);

	int x = pHook->GetArgument<int>(1);
	assert(x == 3);

	int y = pHook->GetArgument<int>(2);
	assert(y == 10);
	return false;
}

bool PostMyFunc(HookType_t eHookType, CHook* pHook)
{
	g_iPostMyFuncCallCount++;
	int x = pHook->GetArgument<int>(1);
	assert(x == 3);

	int y = pHook->GetArgument<int>(2);
	assert(y == 10);

	int return_value = pHook->GetReturnValue<int>();
	assert(return_value == 13);
	
	pHook->SetReturnValue<int>(1337);
	return false;
}


// ============================================================================
// >> main
// ============================================================================
int main()
{
	CHookManager* pHookMngr = GetHookManager();

	int (__thiscall MyClass::*MyFunc)(int, int) = &MyClass::MyFunc;

	// Prepare calling convention
	std::vector<DataType_t> vecArgTypes;
	vecArgTypes.push_back(DATA_TYPE_POINTER);
	vecArgTypes.push_back(DATA_TYPE_INT);
	vecArgTypes.push_back(DATA_TYPE_INT);

	// Hook the function
	CHook* pHook = pHookMngr->HookFunction(
		(void *&) MyFunc,
		new x86MsThiscall(vecArgTypes, DATA_TYPE_INT)
	);

	pHook->AddCallback(HOOKTYPE_PRE, (HookHandlerFn *) (void *) &PreMyFunc);
	pHook->AddCallback(HOOKTYPE_POST, (HookHandlerFn *) (void *) &PostMyFunc);

	// Call the function
	MyClass a;
	g_pMyClass = &a;

	int return_value = a.MyFunc(3, 10);
	
	assert(g_iMyFuncCallCount == 1);
	assert(g_iPreMyFuncCallCount == 1);
	assert(g_iPostMyFuncCallCount == 1);
	assert(return_value == 1337);

	pHookMngr->UnhookAllFunctions();
	return 0;
}

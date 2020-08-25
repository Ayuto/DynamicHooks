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
#include "conventions/x86MsFastcall.h"


// ============================================================================
// >> GLOBAL VARIABLES
// ============================================================================
int g_iMyFuncCallCount = 0;
int g_iPreMyFuncCallCount = 0;
int g_iPostMyFuncCallCount = 0;


// ============================================================================
// >> stdcall test
// ============================================================================
int __fastcall MyFunc(int x)
{
	g_iMyFuncCallCount++;
	assert(x >= 0 && x <= 3);
	if (x == 3)
		return x;

	return MyFunc(x + 1);
}

bool PreMyFunc(HookType_t eHookType, CHook* pHook)
{
	g_iPreMyFuncCallCount++;
	int x = pHook->GetArgument<int>(0);
	assert(x >= 0 && x <= 3);
	return false;
}

bool PostMyFunc(HookType_t eHookType, CHook* pHook)
{
	g_iPostMyFuncCallCount++;
	int return_value = pHook->GetReturnValue<int>();
	assert(return_value == 3);
	return false;
}


// ============================================================================
// >> main
// ============================================================================
int main()
{
	CHookManager* pHookMngr = GetHookManager();

	// Prepare calling convention
	std::vector<DataType_t> vecArgTypes;
	vecArgTypes.push_back(DATA_TYPE_INT);

	// Hook the function
	CHook* pHook = pHookMngr->HookFunction(
		(void *) &MyFunc,
		new x86MsFastcall(vecArgTypes, DATA_TYPE_INT)
	);

	pHook->AddCallback(HOOKTYPE_PRE, (HookHandlerFn *) (void *) &PreMyFunc);
	pHook->AddCallback(HOOKTYPE_POST, (HookHandlerFn *) (void *) &PostMyFunc);

	// Call the function
	int return_value = MyFunc(0);
	
	assert(g_iMyFuncCallCount == 4);
	assert(g_iPreMyFuncCallCount == 4);
	assert(g_iPostMyFuncCallCount == 4);
	assert(return_value == 3);

	//pHookMngr->UnhookAllFunctions();
	return 0;
}

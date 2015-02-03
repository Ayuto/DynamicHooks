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
#include "hook.h"
#include "utilities.h"
#include "asm.h"

#include "AsmJit.h"
using namespace AsmJit;


// ============================================================================
// >> DEFINITIONS
// ============================================================================
#define JMP_SIZE 6


// ============================================================================
// >> CHook
// ============================================================================
CHook::CHook(void* pFunc, int iPopSize, std::list<Register_t> vecRegistersToSave)
{
	m_pFunc = pFunc;
	m_iPopSize = iPopSize;
	m_vecRegistersToSave = vecRegistersToSave;
	m_pRegisters = new CRegisters();

	unsigned char* pTarget = (unsigned char *) pFunc;

	// Determine the number of bytes we need to copy
	int iBytesToCopy = copy_bytes(pTarget, NULL, JMP_SIZE);

	// Create an array for the bytes to copy + a jump to the rest of the
	// function.
	unsigned char* pCopiedBytes = new unsigned char[iBytesToCopy + JMP_SIZE];

	// Fill the array with NOP instructions
	memset(pCopiedBytes, 0x90, iBytesToCopy + JMP_SIZE);

	// Copy the required bytes to our array
	SetMemPatchable(pCopiedBytes, iBytesToCopy + JMP_SIZE);
	copy_bytes(pTarget, pCopiedBytes, JMP_SIZE);
	
	// Write a jump after the copied bytes to the function/bridge + number of bytes to copy
	WriteJMP(pCopiedBytes + iBytesToCopy, pTarget + iBytesToCopy);

	// Save the trampoline
	m_pTrampoline = (void *) pCopiedBytes;

	// Create the bridge function
	m_pBridge = CreateBridge();

	// Write a jump to the bridge
	WriteJMP((unsigned char *) pFunc, m_pBridge);
}

CHook::~CHook()
{
	// Copy back the previously copied bytes
	copy_bytes((unsigned char *) m_pTrampoline, (unsigned char *) m_pFunc, JMP_SIZE);

	// Free the trampoline array
	free(m_pTrampoline);

	// Free the asm bridge and new return address
	MemoryManager::getGlobal()->free(m_pBridge);
	MemoryManager::getGlobal()->free(m_pNewRetAddr);

	delete m_pRegisters;
}

void CHook::AddCallback(HookType_t eHookType, HookHandlerFn* pCallback)
{
	if (!pCallback)
		return;

	if (!IsCallbackRegistered(eHookType, pCallback))
		m_hookHandler[eHookType].push_back(pCallback);
}

void CHook::RemoveCallback(HookType_t eHookType, HookHandlerFn* pCallback)
{
	if (IsCallbackRegistered(eHookType, pCallback))
		m_hookHandler[eHookType].remove(pCallback);
}

bool CHook::IsCallbackRegistered(HookType_t eHookType, HookHandlerFn* pCallback)
{
	std::list<HookHandlerFn *> callbacks = m_hookHandler[eHookType];
	for(std::list<HookHandlerFn *>::iterator it=callbacks.begin(); it != callbacks.end(); it++)
	{
		if (*it == pCallback)
			return true;
	}
	return false;
}

bool CHook::HookHandler(HookType_t eHookType)
{
	bool bOverride = false;
	std::list<HookHandlerFn *> callbacks = this->m_hookHandler[eHookType];
	for(std::list<HookHandlerFn *>::iterator it=callbacks.begin(); it != callbacks.end(); it++)
	{
		bool result = ((HookHandlerFn) *it)(eHookType, this);
		if (result)
			bOverride = true;
	}
	return bOverride;
}

void* CHook::CreateBridge()
{
	Assembler a;
	Label label_override = a.newLabel();

	// Write a redirect to the post-hook code
	Write_ModifyReturnAddress(a);
		
	// Call the pre-hook handler and jump to label_override if true was returned
	Write_CallHandler(a, HOOKTYPE_PRE);
	a.cmp(nax, true);
	a.je(label_override);

	// Jump to the trampoline
	a.jmp(m_pTrampoline);

	// This code will be executed if a pre-hook returns true
	a.bind(label_override);

	// Finally, return to the caller
	a.ret(imm(m_iPopSize));

	return a.make();
}

void CHook::Write_ModifyReturnAddress(Assembler& a)
{
	// Store the return address in nax (AsmJit extension)
	a.mov(nax, dword_ptr(esp));

	// Store return address in m_pRetAddr, so we can access it later
	a.mov(dword_ptr_abs(&m_pRetAddr), nax);

	// Override the return address. This is a redirect to our post-hook code
	m_pNewRetAddr = CreatePostCallback();
	a.mov(dword_ptr(esp), imm((sysint_t) m_pNewRetAddr));
}

void* CHook::CreatePostCallback()
{	
	Assembler a;

	// Subtract the previous added bytes, so that we can access the arguments again
	a.sub(esp, imm(m_iPopSize+4));

	// Call the post-hook handler
	Write_CallHandler(a, HOOKTYPE_POST);

	// Add them again to the stack
	a.add(esp, imm(m_iPopSize+4));

	// Jump to the original return address
	a.jmp(dword_ptr_abs(&m_pRetAddr));

	// Generate the code
	return a.make();
}


void CHook::Write_CallHandler(Assembler& a, HookType_t type)
{	
	bool (__cdecl CHook::*HookHandler)(HookType_t) = &CHook::HookHandler;
	
	// Save the registers so that we can access them in our handlers
	Write_SaveRegisters(a);

	a.push(type);
	a.push(imm((sysint_t) this));
	a.call((void *&) HookHandler);
	a.add(esp, 8);

	// Move the return value to the nax register (AsmJit extension)
	a.mov(nax, eax);

	// Restore them, so any changes will be applied
	Write_RestoreRegisters(a);
}

void CHook::Write_SaveRegisters(Assembler& a)
{
	for(std::list<Register_t>::iterator it=m_vecRegistersToSave.begin(); it != m_vecRegistersToSave.end(); it++)
	{
		Register_t reg = *it;
		//printf("Adding register %i...\n", reg);

		switch(reg)
		{
		case EAX: a.mov(dword_ptr_abs(m_pRegisters->m_eax), eax); break;
		case ESP: a.mov(dword_ptr_abs(m_pRegisters->m_esp), esp); break;
		case ECX: a.mov(dword_ptr_abs(m_pRegisters->m_ecx), ecx); break;
		default: puts("Unknown regiser");
		}
	}
}

void CHook::Write_RestoreRegisters(Assembler& a)
{
	for(std::list<Register_t>::iterator it=m_vecRegistersToSave.begin(); it != m_vecRegistersToSave.end(); it++)
	{
		switch(*it)
		{
		case EAX: a.mov(eax, dword_ptr_abs(m_pRegisters->m_eax)); break;
		case ESP: a.mov(esp, dword_ptr_abs(m_pRegisters->m_esp)); break;
		case ECX: a.mov(ecx, dword_ptr_abs(m_pRegisters->m_ecx)); break;
		}
	}
}
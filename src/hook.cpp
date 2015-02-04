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

	// Subtract the previously added bytes, so that we can access the arguments again
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
		switch(*it)
		{
		// ========================================================================
		// >> 8-bit General purpose registers
		// ========================================================================
		case AL: a.mov(byte_ptr_abs(m_pRegisters->m_al), al); break;
		case CL: a.mov(byte_ptr_abs(m_pRegisters->m_cl), cl); break;
		case DL: a.mov(byte_ptr_abs(m_pRegisters->m_dl), dl); break;
		case BL: a.mov(byte_ptr_abs(m_pRegisters->m_bl), bl); break;
	
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case SPL: a.mov(byte_ptr_abs(m_pRegisters->m_spl), spl); break;
		case BPL: a.mov(byte_ptr_abs(m_pRegisters->m_bpl), bpl); break;
		case SIL: a.mov(byte_ptr_abs(m_pRegisters->m_sil), sil); break;
		case DIL: a.mov(byte_ptr_abs(m_pRegisters->m_dil), dil); break;
		case R8B: a.mov(byte_ptr_abs(m_pRegisters->m_r8b), r8b); break;
		case R9B: a.mov(byte_ptr_abs(m_pRegisters->m_r9b), r9b); break;
		case R10B: a.mov(byte_ptr_abs(m_pRegisters->m_r10b), r10b); break;
		case R11B: a.mov(byte_ptr_abs(m_pRegisters->m_r11b), r11b); break;
		case R12B: a.mov(byte_ptr_abs(m_pRegisters->m_r12b), r12b); break;
		case R13B: a.mov(byte_ptr_abs(m_pRegisters->m_r13b), r13b); break;
		case R14B: a.mov(byte_ptr_abs(m_pRegisters->m_r14b), r14b); break;
		case R15B: a.mov(byte_ptr_abs(m_pRegisters->m_r15b), r15b); break;
#endif // ASMJIT_X64
	
		case AH: a.mov(byte_ptr_abs(m_pRegisters->m_ah), ah); break;
		case CH: a.mov(byte_ptr_abs(m_pRegisters->m_ch), ch); break;
		case DH: a.mov(byte_ptr_abs(m_pRegisters->m_dh), dh); break;
		case BH: a.mov(byte_ptr_abs(m_pRegisters->m_bh), bh); break;
	
		// ========================================================================
		// >> 16-bit General purpose registers
		// ========================================================================
		case AX: a.mov(word_ptr_abs(m_pRegisters->m_ax), ax); break;
		case CX: a.mov(word_ptr_abs(m_pRegisters->m_cx), cx); break;
		case DX: a.mov(word_ptr_abs(m_pRegisters->m_dx), dx); break;
		case BX: a.mov(word_ptr_abs(m_pRegisters->m_bx), bx); break;
		case SP: a.mov(word_ptr_abs(m_pRegisters->m_sp), sp); break;
		case BP: a.mov(word_ptr_abs(m_pRegisters->m_bp), bp); break;
		case SI: a.mov(word_ptr_abs(m_pRegisters->m_si), si); break;
		case DI: a.mov(word_ptr_abs(m_pRegisters->m_di), di); break;
	
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8W: a.mov(word_ptr_abs(m_pRegisters->m_r8w), r8w); break;
		case R9W: a.mov(word_ptr_abs(m_pRegisters->m_r9w), r9w); break;
		case R10W: a.mov(word_ptr_abs(m_pRegisters->m_r10w), r10w); break;
		case R11W: a.mov(word_ptr_abs(m_pRegisters->m_r11w), r11w); break;
		case R12W: a.mov(word_ptr_abs(m_pRegisters->m_r12w), r12w); break;
		case R13W: a.mov(word_ptr_abs(m_pRegisters->m_r13w), r13w); break;
		case R14W: a.mov(word_ptr_abs(m_pRegisters->m_r14w), r14w); break;
		case R15W: a.mov(word_ptr_abs(m_pRegisters->m_r15w), r15w); break;
#endif // ASMJIT_X64
	
		// ========================================================================
		// >> 32-bit General purpose registers
		// ========================================================================
		case EAX: a.mov(dword_ptr_abs(m_pRegisters->m_eax), eax); break;
		case ECX: a.mov(dword_ptr_abs(m_pRegisters->m_ecx), ecx); break;
		case EDX: a.mov(dword_ptr_abs(m_pRegisters->m_edx), edx); break;
		case EBX: a.mov(dword_ptr_abs(m_pRegisters->m_ebx), ebx); break;
		case ESP: a.mov(dword_ptr_abs(m_pRegisters->m_esp), esp); break;
		case EBP: a.mov(dword_ptr_abs(m_pRegisters->m_ebp), ebp); break;
		case ESI: a.mov(dword_ptr_abs(m_pRegisters->m_esi), esi); break;
		case EDI: a.mov(dword_ptr_abs(m_pRegisters->m_edi), edi); break;
	
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8D: a.mov(dword_ptr_abs(m_pRegisters->m_r8d), r8d); break;
		case R9D: a.mov(dword_ptr_abs(m_pRegisters->m_r9d), r9d); break;
		case R10D: a.mov(dword_ptr_abs(m_pRegisters->m_r10d), r10d); break;
		case R11D: a.mov(dword_ptr_abs(m_pRegisters->m_r11d), r11d); break;
		case R12D: a.mov(dword_ptr_abs(m_pRegisters->m_r12d), r12d); break;
		case R13D: a.mov(dword_ptr_abs(m_pRegisters->m_r13d), r13d); break;
		case R14D: a.mov(dword_ptr_abs(m_pRegisters->m_r14d), r14d); break;
		case R15D: a.mov(dword_ptr_abs(m_pRegisters->m_r15d), r15d); break;
#endif // ASMJIT_X64
	
		// ========================================================================
		// >> 64-bit General purpose registers
		// ========================================================================
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case RAX: a.mov(qword_ptr_abs(m_pRegisters->m_rax), rax); break;
		case RCX: a.mov(qword_ptr_abs(m_pRegisters->m_rcx), rcx); break;
		case RDX: a.mov(qword_ptr_abs(m_pRegisters->m_rdx), rdx); break;
		case RBX: a.mov(qword_ptr_abs(m_pRegisters->m_rbx), rbx); break;
		case RSP: a.mov(qword_ptr_abs(m_pRegisters->m_rsp), rsp); break;
		case RBP: a.mov(qword_ptr_abs(m_pRegisters->m_rbp), rbp); break;
		case RSI: a.mov(qword_ptr_abs(m_pRegisters->m_rsi), rsi); break;
		case RDI: a.mov(qword_ptr_abs(m_pRegisters->m_rdi), rdi); break;
#endif // ASMJIT_X64
	
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8: a.mov(qword_ptr_abs(m_pRegisters->m_r8), r8); break;
		case R9: a.mov(qword_ptr_abs(m_pRegisters->m_r9), r9); break;
		case R10: a.mov(qword_ptr_abs(m_pRegisters->m_r10), r10); break;
		case R11: a.mov(qword_ptr_abs(m_pRegisters->m_r11), r11); break;
		case R12: a.mov(qword_ptr_abs(m_pRegisters->m_r12), r12); break;
		case R13: a.mov(qword_ptr_abs(m_pRegisters->m_r13), r13); break;
		case R14: a.mov(qword_ptr_abs(m_pRegisters->m_r14), r14); break;
		case R15: a.mov(qword_ptr_abs(m_pRegisters->m_r15), r15); break;
#endif // ASMJIT_X64
	
		// ========================================================================
		// >> 64-bit MM (MMX) registers
		// ========================================================================
		case MM0: a.movq(qword_ptr_abs(m_pRegisters->m_mm0), mm0); break;
		case MM1: a.movq(qword_ptr_abs(m_pRegisters->m_mm1), mm1); break;
		case MM2: a.movq(qword_ptr_abs(m_pRegisters->m_mm2), mm2); break;
		case MM3: a.movq(qword_ptr_abs(m_pRegisters->m_mm3), mm3); break;
		case MM4: a.movq(qword_ptr_abs(m_pRegisters->m_mm4), mm4); break;
		case MM5: a.movq(qword_ptr_abs(m_pRegisters->m_mm5), mm5); break;
		case MM6: a.movq(qword_ptr_abs(m_pRegisters->m_mm6), mm6); break;
		case MM7: a.movq(qword_ptr_abs(m_pRegisters->m_mm7), mm7); break;
	
		// ========================================================================
		// >> 128-bit XMM registers
		// ========================================================================
		// TODO: Also provide movups?
		case XMM0: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm0), xmm0); break;
		case XMM1: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm1), xmm1); break;
		case XMM2: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm2), xmm2); break;
		case XMM3: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm3), xmm3); break;
		case XMM4: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm4), xmm4); break;
		case XMM5: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm5), xmm5); break;
		case XMM6: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm6), xmm6); break;
		case XMM7: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm7), xmm7); break;
	
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case XMM8: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm8), xmm8); break;
		case XMM9: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm9), xmm9); break;
		case XMM10: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm10), xmm10); break;
		case XMM11: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm11), xmm11); break;
		case XMM12: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm12), xmm12); break;
		case XMM13: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm13), xmm13); break;
		case XMM14: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm14), xmm14); break;
		case XMM15: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm15), xmm15); break;
#endif // ASMJIT_X64
	
		// ========================================================================
		// >> 16-bit Segment registers
		// ========================================================================
		case CS: a.mov(word_ptr_abs(m_pRegisters->m_cs), cs); break;
		case SS: a.mov(word_ptr_abs(m_pRegisters->m_ss), ss); break;
		case DS: a.mov(word_ptr_abs(m_pRegisters->m_ds), ds); break;
		case ES: a.mov(word_ptr_abs(m_pRegisters->m_es), es); break;
		case FS: a.mov(word_ptr_abs(m_pRegisters->m_fs), fs); break;
		case GS: a.mov(word_ptr_abs(m_pRegisters->m_gs), gs); break;
	
		// ========================================================================
		// >> 80-bit FPU registers
		// ========================================================================
		case ST0: a.fst(tword_ptr_abs(m_pRegisters->m_st0)); break;
		//case ST1: a.mov(tword_ptr_abs(m_pRegisters->m_st1), st1); break;
		//case ST2: a.mov(tword_ptr_abs(m_pRegisters->m_st2), st2); break;
		//case ST3: a.mov(tword_ptr_abs(m_pRegisters->m_st3), st3); break;
		//case ST4: a.mov(tword_ptr_abs(m_pRegisters->m_st4), st4); break;
		//case ST5: a.mov(tword_ptr_abs(m_pRegisters->m_st5), st5); break;
		//case ST6: a.mov(tword_ptr_abs(m_pRegisters->m_st6), st6); break;
		//case ST7: a.mov(tword_ptr_abs(m_pRegisters->m_st7), st7); break;

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
		// ========================================================================
		// >> 8-bit General purpose registers
		// ========================================================================
		case AL: a.mov(byte_ptr_abs(m_pRegisters->m_al), al); break;
		case CL: a.mov(byte_ptr_abs(m_pRegisters->m_cl), cl); break;
		case DL: a.mov(byte_ptr_abs(m_pRegisters->m_dl), dl); break;
		case BL: a.mov(byte_ptr_abs(m_pRegisters->m_bl), bl); break;
	
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case SPL: a.mov(byte_ptr_abs(m_pRegisters->m_spl), spl); break;
		case BPL: a.mov(byte_ptr_abs(m_pRegisters->m_bpl), bpl); break;
		case SIL: a.mov(byte_ptr_abs(m_pRegisters->m_sil), sil); break;
		case DIL: a.mov(byte_ptr_abs(m_pRegisters->m_dil), dil); break;
		case R8B: a.mov(byte_ptr_abs(m_pRegisters->m_r8b), r8b); break;
		case R9B: a.mov(byte_ptr_abs(m_pRegisters->m_r9b), r9b); break;
		case R10B: a.mov(byte_ptr_abs(m_pRegisters->m_r10b), r10b); break;
		case R11B: a.mov(byte_ptr_abs(m_pRegisters->m_r11b), r11b); break;
		case R12B: a.mov(byte_ptr_abs(m_pRegisters->m_r12b), r12b); break;
		case R13B: a.mov(byte_ptr_abs(m_pRegisters->m_r13b), r13b); break;
		case R14B: a.mov(byte_ptr_abs(m_pRegisters->m_r14b), r14b); break;
		case R15B: a.mov(byte_ptr_abs(m_pRegisters->m_r15b), r15b); break;
#endif // ASMJIT_X64

		case AH: a.mov(byte_ptr_abs(m_pRegisters->m_ah), ah); break;
		case CH: a.mov(byte_ptr_abs(m_pRegisters->m_ch), ch); break;
		case DH: a.mov(byte_ptr_abs(m_pRegisters->m_dh), dh); break;
		case BH: a.mov(byte_ptr_abs(m_pRegisters->m_bh), bh); break;
	
		// ========================================================================
		// >> 16-bit General purpose registers
		// ========================================================================
		case AX: a.mov(word_ptr_abs(m_pRegisters->m_ax), ax); break;
		case CX: a.mov(word_ptr_abs(m_pRegisters->m_cx), cx); break;
		case DX: a.mov(word_ptr_abs(m_pRegisters->m_dx), dx); break;
		case BX: a.mov(word_ptr_abs(m_pRegisters->m_bx), bx); break;
		case SP: a.mov(word_ptr_abs(m_pRegisters->m_sp), sp); break;
		case BP: a.mov(word_ptr_abs(m_pRegisters->m_bp), bp); break;
		case SI: a.mov(word_ptr_abs(m_pRegisters->m_si), si); break;
		case DI: a.mov(word_ptr_abs(m_pRegisters->m_di), di); break;
	
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8W: a.mov(word_ptr_abs(m_pRegisters->m_r8w), r8w); break;
		case R9W: a.mov(word_ptr_abs(m_pRegisters->m_r9w), r9w); break;
		case R10W: a.mov(word_ptr_abs(m_pRegisters->m_r10w), r10w); break;
		case R11W: a.mov(word_ptr_abs(m_pRegisters->m_r11w), r11w); break;
		case R12W: a.mov(word_ptr_abs(m_pRegisters->m_r12w), r12w); break;
		case R13W: a.mov(word_ptr_abs(m_pRegisters->m_r13w), r13w); break;
		case R14W: a.mov(word_ptr_abs(m_pRegisters->m_r14w), r14w); break;
		case R15W: a.mov(word_ptr_abs(m_pRegisters->m_r15w), r15w); break;
#endif // ASMJIT_X64
	
		// ========================================================================
		// >> 32-bit General purpose registers
		// ========================================================================
		case EAX: a.mov(dword_ptr_abs(m_pRegisters->m_eax), eax); break;
		case ECX: a.mov(dword_ptr_abs(m_pRegisters->m_ecx), ecx); break;
		case EDX: a.mov(dword_ptr_abs(m_pRegisters->m_edx), edx); break;
		case EBX: a.mov(dword_ptr_abs(m_pRegisters->m_ebx), ebx); break;
		case ESP: a.mov(dword_ptr_abs(m_pRegisters->m_esp), esp); break;
		case EBP: a.mov(dword_ptr_abs(m_pRegisters->m_ebp), ebp); break;
		case ESI: a.mov(dword_ptr_abs(m_pRegisters->m_esi), esi); break;
		case EDI: a.mov(dword_ptr_abs(m_pRegisters->m_edi), edi); break;
	
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8D: a.mov(dword_ptr_abs(m_pRegisters->m_r8d), r8d); break;
		case R9D: a.mov(dword_ptr_abs(m_pRegisters->m_r9d), r9d); break;
		case R10D: a.mov(dword_ptr_abs(m_pRegisters->m_r10d), r10d); break;
		case R11D: a.mov(dword_ptr_abs(m_pRegisters->m_r11d), r11d); break;
		case R12D: a.mov(dword_ptr_abs(m_pRegisters->m_r12d), r12d); break;
		case R13D: a.mov(dword_ptr_abs(m_pRegisters->m_r13d), r13d); break;
		case R14D: a.mov(dword_ptr_abs(m_pRegisters->m_r14d), r14d); break;
		case R15D: a.mov(dword_ptr_abs(m_pRegisters->m_r15d), r15d); break;
#endif // ASMJIT_X64
	
		// ========================================================================
		// >> 64-bit General purpose registers
		// ========================================================================
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case RAX: a.mov(qword_ptr_abs(m_pRegisters->m_rax), rax); break;
		case RCX: a.mov(qword_ptr_abs(m_pRegisters->m_rcx), rcx); break;
		case RDX: a.mov(qword_ptr_abs(m_pRegisters->m_rdx), rdx); break;
		case RBX: a.mov(qword_ptr_abs(m_pRegisters->m_rbx), rbx); break;
		case RSP: a.mov(qword_ptr_abs(m_pRegisters->m_rsp), rsp); break;
		case RBP: a.mov(qword_ptr_abs(m_pRegisters->m_rbp), rbp); break;
		case RSI: a.mov(qword_ptr_abs(m_pRegisters->m_rsi), rsi); break;
		case RDI: a.mov(qword_ptr_abs(m_pRegisters->m_rdi), rdi); break;
#endif // ASMJIT_X64
	
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8: a.mov(qword_ptr_abs(m_pRegisters->m_r8), r8); break;
		case R9: a.mov(qword_ptr_abs(m_pRegisters->m_r9), r9); break;
		case R10: a.mov(qword_ptr_abs(m_pRegisters->m_r10), r10); break;
		case R11: a.mov(qword_ptr_abs(m_pRegisters->m_r11), r11); break;
		case R12: a.mov(qword_ptr_abs(m_pRegisters->m_r12), r12); break;
		case R13: a.mov(qword_ptr_abs(m_pRegisters->m_r13), r13); break;
		case R14: a.mov(qword_ptr_abs(m_pRegisters->m_r14), r14); break;
		case R15: a.mov(qword_ptr_abs(m_pRegisters->m_r15), r15); break;
#endif // ASMJIT_X64
	
		// ========================================================================
		// >> 64-bit MM (MMX) registers
		// ========================================================================
		case MM0: a.movq(mmword_ptr_abs(m_pRegisters->m_mm0), mm0); break;
		case MM1: a.movq(mmword_ptr_abs(m_pRegisters->m_mm1), mm1); break;
		case MM2: a.movq(mmword_ptr_abs(m_pRegisters->m_mm2), mm2); break;
		case MM3: a.movq(mmword_ptr_abs(m_pRegisters->m_mm3), mm3); break;
		case MM4: a.movq(mmword_ptr_abs(m_pRegisters->m_mm4), mm4); break;
		case MM5: a.movq(mmword_ptr_abs(m_pRegisters->m_mm5), mm5); break;
		case MM6: a.movq(mmword_ptr_abs(m_pRegisters->m_mm6), mm6); break;
		case MM7: a.movq(mmword_ptr_abs(m_pRegisters->m_mm7), mm7); break;
	
		// ========================================================================
		// >> 128-bit XMM registers
		// ========================================================================
		// TODO: Also provide movups?
		case XMM0: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm0), xmm0); break;
		case XMM1: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm1), xmm1); break;
		case XMM2: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm2), xmm2); break;
		case XMM3: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm3), xmm3); break;
		case XMM4: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm4), xmm4); break;
		case XMM5: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm5), xmm5); break;
		case XMM6: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm6), xmm6); break;
		case XMM7: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm7), xmm7); break;
	
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case XMM8: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm8), xmm8); break;
		case XMM9: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm9), xmm9); break;
		case XMM10: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm10), xmm10); break;
		case XMM11: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm11), xmm11); break;
		case XMM12: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm12), xmm12); break;
		case XMM13: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm13), xmm13); break;
		case XMM14: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm14), xmm14); break;
		case XMM15: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm15), xmm15); break;
#endif // ASMJIT_X64
	
		// ========================================================================
		// >> 16-bit Segment registers
		// ========================================================================
		case CS: a.mov(word_ptr_abs(m_pRegisters->m_cs), cs); break;
		case SS: a.mov(word_ptr_abs(m_pRegisters->m_ss), ss); break;
		case DS: a.mov(word_ptr_abs(m_pRegisters->m_ds), ds); break;
		case ES: a.mov(word_ptr_abs(m_pRegisters->m_es), es); break;
		case FS: a.mov(word_ptr_abs(m_pRegisters->m_fs), fs); break;
		case GS: a.mov(word_ptr_abs(m_pRegisters->m_gs), gs); break;
	
		// ========================================================================
		// >> 80-bit FPU registers
		// ========================================================================
		case ST0: a.fld(tword_ptr_abs(m_pRegisters->m_st0)); break;
		//case ST1: a.mov(tword_ptr_abs(m_pRegisters->m_st1), st1); break;
		//case ST2: a.mov(tword_ptr_abs(m_pRegisters->m_st2), st2); break;
		//case ST3: a.mov(tword_ptr_abs(m_pRegisters->m_st3), st3); break;
		//case ST4: a.mov(tword_ptr_abs(m_pRegisters->m_st4), st4); break;
		//case ST5: a.mov(tword_ptr_abs(m_pRegisters->m_st5), st5); break;
		//case ST6: a.mov(tword_ptr_abs(m_pRegisters->m_st6), st6); break;
		//case ST7: a.mov(tword_ptr_abs(m_pRegisters->m_st7), st7); break;
		}
	}
}
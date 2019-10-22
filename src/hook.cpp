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
CHook::CHook(void* pFunc, ICallingConvention* pConvention)
{
	m_pFunc = pFunc;
	m_pRegisters = new CRegisters(pConvention->GetRegisters());
	m_pCallingConvention = pConvention;

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
	delete m_pCallingConvention;
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

void* __cdecl CHook::GetReturnAddress(void* pESP)
{
	if (m_RetAddr.count(pESP) == 0)
		puts("ESP not present.");

	return m_RetAddr[pESP];
}

void __cdecl CHook::SetReturnAddress(void* pRetAddr, void* pESP)
{
	m_RetAddr[pESP] = pRetAddr;
}

void* CHook::CreateBridge()
{
	Assembler a;
	Label label_override = a.newLabel();

	// Write a redirect to the post-hook code
	Write_ModifyReturnAddress(a);

	// Call the pre-hook handler and jump to label_override if true was returned
	Write_CallHandler(a, HOOKTYPE_PRE);
	a.cmp(eax, true);
	
	// Restore the previously saved registers, so any changes will be applied
	Write_RestoreRegisters(a);

	a.je(label_override);

	// Jump to the trampoline
	a.jmp(m_pTrampoline);

	// This code will be executed if a pre-hook returns true
	a.bind(label_override);

	// Finally, return to the caller
	// This will still call post hooks, but will skip the original function.
	a.ret(imm(m_pCallingConvention->GetPopSize()));

	return a.make();
}

void CHook::Write_ModifyReturnAddress(Assembler& a)
{
	// Save scratch registers that are used by SetReturnAddress
	static void* pEAX = NULL;
	static void* pECX = NULL;
	static void* pEDX = NULL;
	a.mov(dword_ptr_abs(&pEAX), eax);
	a.mov(dword_ptr_abs(&pECX), ecx);
	a.mov(dword_ptr_abs(&pEDX), edx);

	// Store the return address in eax
	a.mov(eax, dword_ptr(esp));
	
	// Save the original return address by using the current esp as the key.
	// This should be unique until we have returned to the original caller.
	void (__cdecl CHook::*SetReturnAddress)(void*, void*) = &CHook::SetReturnAddress;
	a.push(esp);
	a.push(eax);
	a.push(imm((sysint_t) this));
	a.call((void *&) SetReturnAddress);
	a.add(esp, 12);
	
	// Restore scratch registers
	a.mov(eax, dword_ptr_abs(&pEAX));
	a.mov(ecx, dword_ptr_abs(&pECX));
	a.mov(edx, dword_ptr_abs(&pEDX));

	// Override the return address. This is a redirect to our post-hook code
	m_pNewRetAddr = CreatePostCallback();
	a.mov(dword_ptr(esp), imm((sysint_t) m_pNewRetAddr));
}

void* CHook::CreatePostCallback()
{
	Assembler a;

	int iPopSize = m_pCallingConvention->GetPopSize();

	// Subtract the previously added bytes (stack size + return address), so
	// that we can access the arguments again
	a.sub(esp, imm(iPopSize+4));

	// Call the post-hook handler
	Write_CallHandler(a, HOOKTYPE_POST);

	// Restore the previously saved registers, so any changes will be applied
	Write_RestoreRegisters(a);

	// Save scratch registers that are used by GetReturnAddress
	static void* pEAX = NULL;
	static void* pECX = NULL;
	static void* pEDX = NULL;
	a.mov(dword_ptr_abs(&pEAX), eax);
	a.mov(dword_ptr_abs(&pECX), ecx);
	a.mov(dword_ptr_abs(&pEDX), edx);
	
	// Get the original return address
	void* (__cdecl CHook::*GetReturnAddress)(void*) = &CHook::GetReturnAddress;
	a.push(esp);
	a.push(imm((sysint_t) this));
	a.call((void *&) GetReturnAddress);
	a.add(esp, 8);

	// Save the original return address
	static void* pRetAddr = NULL;
	a.mov(dword_ptr_abs(&pRetAddr), eax);
	
	// Restore scratch registers
	a.mov(eax, dword_ptr_abs(&pEAX));
	a.mov(ecx, dword_ptr_abs(&pECX));
	a.mov(edx, dword_ptr_abs(&pEDX));

	// Add the bytes again to the stack (stack size + return address), so we
	// don't corrupt the stack.
	a.add(esp, imm(iPopSize+4));

	// Jump to the original return address
	a.jmp(dword_ptr_abs(&pRetAddr));

	// Generate the code
	return a.make();
}

void CHook::Write_CallHandler(Assembler& a, HookType_t type)
{
	bool (__cdecl CHook::*HookHandler)(HookType_t) = &CHook::HookHandler;

	// Save the registers so that we can access them in our handlers
	Write_SaveRegisters(a);

	// Call the global hook handler
	// Subtract 4 bytes to preserve 16-Byte stack alignment for Linux
	a.sub(esp, 4);
	a.push(type);
	a.push(imm((sysint_t) this));
	a.call((void *&) HookHandler);
	a.add(esp, 12);
}

void CHook::Write_SaveRegisters(Assembler& a)
{
	std::list<Register_t> vecRegistersToSave = m_pCallingConvention->GetRegisters();
	for(std::list<Register_t>::iterator it=vecRegistersToSave.begin(); it != vecRegistersToSave.end(); it++)
	{
		switch(*it)
		{
		// ========================================================================
		// >> 8-bit General purpose registers
		// ========================================================================
		case AL: a.mov(byte_ptr_abs(m_pRegisters->m_al->m_pAddress), al); break;
		case CL: a.mov(byte_ptr_abs(m_pRegisters->m_cl->m_pAddress), cl); break;
		case DL: a.mov(byte_ptr_abs(m_pRegisters->m_dl->m_pAddress), dl); break;
		case BL: a.mov(byte_ptr_abs(m_pRegisters->m_bl->m_pAddress), bl); break;

#if defined(ASMJIT_X64)
		// 64-bit mode only
		case SPL: a.mov(byte_ptr_abs(m_pRegisters->m_spl->m_pAddress), spl); break;
		case BPL: a.mov(byte_ptr_abs(m_pRegisters->m_bpl->m_pAddress), bpl); break;
		case SIL: a.mov(byte_ptr_abs(m_pRegisters->m_sil->m_pAddress), sil); break;
		case DIL: a.mov(byte_ptr_abs(m_pRegisters->m_dil->m_pAddress), dil); break;
		case R8B: a.mov(byte_ptr_abs(m_pRegisters->m_r8b->m_pAddress), r8b); break;
		case R9B: a.mov(byte_ptr_abs(m_pRegisters->m_r9b->m_pAddress), r9b); break;
		case R10B: a.mov(byte_ptr_abs(m_pRegisters->m_r10b->m_pAddress), r10b); break;
		case R11B: a.mov(byte_ptr_abs(m_pRegisters->m_r11b->m_pAddress), r11b); break;
		case R12B: a.mov(byte_ptr_abs(m_pRegisters->m_r12b->m_pAddress), r12b); break;
		case R13B: a.mov(byte_ptr_abs(m_pRegisters->m_r13b->m_pAddress), r13b); break;
		case R14B: a.mov(byte_ptr_abs(m_pRegisters->m_r14b->m_pAddress), r14b); break;
		case R15B: a.mov(byte_ptr_abs(m_pRegisters->m_r15b->m_pAddress), r15b); break;
#endif // ASMJIT_X64

		case AH: a.mov(byte_ptr_abs(m_pRegisters->m_ah->m_pAddress), ah); break;
		case CH: a.mov(byte_ptr_abs(m_pRegisters->m_ch->m_pAddress), ch); break;
		case DH: a.mov(byte_ptr_abs(m_pRegisters->m_dh->m_pAddress), dh); break;
		case BH: a.mov(byte_ptr_abs(m_pRegisters->m_bh->m_pAddress), bh); break;

		// ========================================================================
		// >> 16-bit General purpose registers
		// ========================================================================
		case AX: a.mov(word_ptr_abs(m_pRegisters->m_ax->m_pAddress), ax); break;
		case CX: a.mov(word_ptr_abs(m_pRegisters->m_cx->m_pAddress), cx); break;
		case DX: a.mov(word_ptr_abs(m_pRegisters->m_dx->m_pAddress), dx); break;
		case BX: a.mov(word_ptr_abs(m_pRegisters->m_bx->m_pAddress), bx); break;
		case SP: a.mov(word_ptr_abs(m_pRegisters->m_sp->m_pAddress), sp); break;
		case BP: a.mov(word_ptr_abs(m_pRegisters->m_bp->m_pAddress), bp); break;
		case SI: a.mov(word_ptr_abs(m_pRegisters->m_si->m_pAddress), si); break;
		case DI: a.mov(word_ptr_abs(m_pRegisters->m_di->m_pAddress), di); break;

#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8W: a.mov(word_ptr_abs(m_pRegisters->m_r8w->m_pAddress), r8w); break;
		case R9W: a.mov(word_ptr_abs(m_pRegisters->m_r9w->m_pAddress), r9w); break;
		case R10W: a.mov(word_ptr_abs(m_pRegisters->m_r10w->m_pAddress), r10w); break;
		case R11W: a.mov(word_ptr_abs(m_pRegisters->m_r11w->m_pAddress), r11w); break;
		case R12W: a.mov(word_ptr_abs(m_pRegisters->m_r12w->m_pAddress), r12w); break;
		case R13W: a.mov(word_ptr_abs(m_pRegisters->m_r13w->m_pAddress), r13w); break;
		case R14W: a.mov(word_ptr_abs(m_pRegisters->m_r14w->m_pAddress), r14w); break;
		case R15W: a.mov(word_ptr_abs(m_pRegisters->m_r15w->m_pAddress), r15w); break;
#endif // ASMJIT_X64

		// ========================================================================
		// >> 32-bit General purpose registers
		// ========================================================================
		case EAX: a.mov(dword_ptr_abs(m_pRegisters->m_eax->m_pAddress), eax); break;
		case ECX: a.mov(dword_ptr_abs(m_pRegisters->m_ecx->m_pAddress), ecx); break;
		case EDX: a.mov(dword_ptr_abs(m_pRegisters->m_edx->m_pAddress), edx); break;
		case EBX: a.mov(dword_ptr_abs(m_pRegisters->m_ebx->m_pAddress), ebx); break;
		case ESP: a.mov(dword_ptr_abs(m_pRegisters->m_esp->m_pAddress), esp); break;
		case EBP: a.mov(dword_ptr_abs(m_pRegisters->m_ebp->m_pAddress), ebp); break;
		case ESI: a.mov(dword_ptr_abs(m_pRegisters->m_esi->m_pAddress), esi); break;
		case EDI: a.mov(dword_ptr_abs(m_pRegisters->m_edi->m_pAddress), edi); break;

#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8D: a.mov(dword_ptr_abs(m_pRegisters->m_r8d->m_pAddress), r8d); break;
		case R9D: a.mov(dword_ptr_abs(m_pRegisters->m_r9d->m_pAddress), r9d); break;
		case R10D: a.mov(dword_ptr_abs(m_pRegisters->m_r10d->m_pAddress), r10d); break;
		case R11D: a.mov(dword_ptr_abs(m_pRegisters->m_r11d->m_pAddress), r11d); break;
		case R12D: a.mov(dword_ptr_abs(m_pRegisters->m_r12d->m_pAddress), r12d); break;
		case R13D: a.mov(dword_ptr_abs(m_pRegisters->m_r13d->m_pAddress), r13d); break;
		case R14D: a.mov(dword_ptr_abs(m_pRegisters->m_r14d->m_pAddress), r14d); break;
		case R15D: a.mov(dword_ptr_abs(m_pRegisters->m_r15d->m_pAddress), r15d); break;
#endif // ASMJIT_X64

		// ========================================================================
		// >> 64-bit General purpose registers
		// ========================================================================
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case RAX: a.mov(qword_ptr_abs(m_pRegisters->m_rax->m_pAddress), rax); break;
		case RCX: a.mov(qword_ptr_abs(m_pRegisters->m_rcx->m_pAddress), rcx); break;
		case RDX: a.mov(qword_ptr_abs(m_pRegisters->m_rdx->m_pAddress), rdx); break;
		case RBX: a.mov(qword_ptr_abs(m_pRegisters->m_rbx->m_pAddress), rbx); break;
		case RSP: a.mov(qword_ptr_abs(m_pRegisters->m_rsp->m_pAddress), rsp); break;
		case RBP: a.mov(qword_ptr_abs(m_pRegisters->m_rbp->m_pAddress), rbp); break;
		case RSI: a.mov(qword_ptr_abs(m_pRegisters->m_rsi->m_pAddress), rsi); break;
		case RDI: a.mov(qword_ptr_abs(m_pRegisters->m_rdi->m_pAddress), rdi); break;
#endif // ASMJIT_X64

#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8: a.mov(qword_ptr_abs(m_pRegisters->m_r8->m_pAddress), r8); break;
		case R9: a.mov(qword_ptr_abs(m_pRegisters->m_r9->m_pAddress), r9); break;
		case R10: a.mov(qword_ptr_abs(m_pRegisters->m_r10->m_pAddress), r10); break;
		case R11: a.mov(qword_ptr_abs(m_pRegisters->m_r11->m_pAddress), r11); break;
		case R12: a.mov(qword_ptr_abs(m_pRegisters->m_r12->m_pAddress), r12); break;
		case R13: a.mov(qword_ptr_abs(m_pRegisters->m_r13->m_pAddress), r13); break;
		case R14: a.mov(qword_ptr_abs(m_pRegisters->m_r14->m_pAddress), r14); break;
		case R15: a.mov(qword_ptr_abs(m_pRegisters->m_r15->m_pAddress), r15); break;
#endif // ASMJIT_X64

		// ========================================================================
		// >> 64-bit MM (MMX) registers
		// ========================================================================
		case MM0: a.movq(qword_ptr_abs(m_pRegisters->m_mm0->m_pAddress), mm0); break;
		case MM1: a.movq(qword_ptr_abs(m_pRegisters->m_mm1->m_pAddress), mm1); break;
		case MM2: a.movq(qword_ptr_abs(m_pRegisters->m_mm2->m_pAddress), mm2); break;
		case MM3: a.movq(qword_ptr_abs(m_pRegisters->m_mm3->m_pAddress), mm3); break;
		case MM4: a.movq(qword_ptr_abs(m_pRegisters->m_mm4->m_pAddress), mm4); break;
		case MM5: a.movq(qword_ptr_abs(m_pRegisters->m_mm5->m_pAddress), mm5); break;
		case MM6: a.movq(qword_ptr_abs(m_pRegisters->m_mm6->m_pAddress), mm6); break;
		case MM7: a.movq(qword_ptr_abs(m_pRegisters->m_mm7->m_pAddress), mm7); break;

		// ========================================================================
		// >> 128-bit XMM registers
		// ========================================================================
		// TODO: Also provide movups?
		case XMM0: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm0->m_pAddress), xmm0); break;
		case XMM1: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm1->m_pAddress), xmm1); break;
		case XMM2: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm2->m_pAddress), xmm2); break;
		case XMM3: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm3->m_pAddress), xmm3); break;
		case XMM4: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm4->m_pAddress), xmm4); break;
		case XMM5: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm5->m_pAddress), xmm5); break;
		case XMM6: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm6->m_pAddress), xmm6); break;
		case XMM7: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm7->m_pAddress), xmm7); break;

#if defined(ASMJIT_X64)
		// 64-bit mode only
		case XMM8: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm8->m_pAddress), xmm8); break;
		case XMM9: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm9->m_pAddress), xmm9); break;
		case XMM10: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm10->m_pAddress), xmm10); break;
		case XMM11: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm11->m_pAddress), xmm11); break;
		case XMM12: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm12->m_pAddress), xmm12); break;
		case XMM13: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm13->m_pAddress), xmm13); break;
		case XMM14: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm14->m_pAddress), xmm14); break;
		case XMM15: a.movaps(dqword_ptr_abs(m_pRegisters->m_xmm15->m_pAddress), xmm15); break;
#endif // ASMJIT_X64

		// ========================================================================
		// >> 16-bit Segment registers
		// ========================================================================
		case CS: a.mov(word_ptr_abs(m_pRegisters->m_cs->m_pAddress), cs); break;
		case SS: a.mov(word_ptr_abs(m_pRegisters->m_ss->m_pAddress), ss); break;
		case DS: a.mov(word_ptr_abs(m_pRegisters->m_ds->m_pAddress), ds); break;
		case ES: a.mov(word_ptr_abs(m_pRegisters->m_es->m_pAddress), es); break;
		case FS: a.mov(word_ptr_abs(m_pRegisters->m_fs->m_pAddress), fs); break;
		case GS: a.mov(word_ptr_abs(m_pRegisters->m_gs->m_pAddress), gs); break;

		// ========================================================================
		// >> 80-bit FPU registers
		// ========================================================================
		case ST0:
		{
			switch(GetDataTypeSize(this->m_pCallingConvention->m_returnType))
			{
				case SIZE_DWORD: a.fstp(dword_ptr_abs(m_pRegisters->m_st0->m_pAddress)); break;
				case SIZE_QWORD: a.fstp(qword_ptr_abs(m_pRegisters->m_st0->m_pAddress)); break;
				case SIZE_TWORD: a.fstp(tword_ptr_abs(m_pRegisters->m_st0->m_pAddress)); break;
			}
			break;
		}
		//case ST1: a.mov(tword_ptr_abs(m_pRegisters->m_st1->m_pAddress), st1); break;
		//case ST2: a.mov(tword_ptr_abs(m_pRegisters->m_st2->m_pAddress), st2); break;
		//case ST3: a.mov(tword_ptr_abs(m_pRegisters->m_st3->m_pAddress), st3); break;
		//case ST4: a.mov(tword_ptr_abs(m_pRegisters->m_st4->m_pAddress), st4); break;
		//case ST5: a.mov(tword_ptr_abs(m_pRegisters->m_st5->m_pAddress), st5); break;
		//case ST6: a.mov(tword_ptr_abs(m_pRegisters->m_st6->m_pAddress), st6); break;
		//case ST7: a.mov(tword_ptr_abs(m_pRegisters->m_st7->m_pAddress), st7); break;

		default: puts("Unsupported register.");
		}
	}
}

void CHook::Write_RestoreRegisters(Assembler& a)
{
	std::list<Register_t> vecRegistersToSave = m_pCallingConvention->GetRegisters();
	for(std::list<Register_t>::iterator it=vecRegistersToSave.begin(); it != vecRegistersToSave.end(); it++)
	{
		switch(*it)
		{
		// ========================================================================
		// >> 8-bit General purpose registers
		// ========================================================================
		case AL: a.mov(al, byte_ptr_abs(m_pRegisters->m_al->m_pAddress)); break;
		case CL: a.mov(cl, byte_ptr_abs(m_pRegisters->m_cl->m_pAddress)); break;
		case DL: a.mov(dl, byte_ptr_abs(m_pRegisters->m_dl->m_pAddress)); break;
		case BL: a.mov(bl, byte_ptr_abs(m_pRegisters->m_bl->m_pAddress)); break;

#if defined(ASMJIT_X64)
		// 64-bit mode only
		case SPL: a.mov(spl, byte_ptr_abs(m_pRegisters->m_spl->m_pAddress)); break;
		case BPL: a.mov(bpl, byte_ptr_abs(m_pRegisters->m_bpl->m_pAddress)); break;
		case SIL: a.mov(sil, byte_ptr_abs(m_pRegisters->m_sil->m_pAddress)); break;
		case DIL: a.mov(dil, byte_ptr_abs(m_pRegisters->m_dil->m_pAddress)); break;
		case R8B: a.mov(r8b, byte_ptr_abs(m_pRegisters->m_r8b->m_pAddress)); break;
		case R9B: a.mov(r9b, byte_ptr_abs(m_pRegisters->m_r9b->m_pAddress)); break;
		case R10B: a.mov(r10b, byte_ptr_abs(m_pRegisters->m_r10b->m_pAddress)); break;
		case R11B: a.mov(r11b, byte_ptr_abs(m_pRegisters->m_r11b->m_pAddress)); break;
		case R12B: a.mov(r12b, byte_ptr_abs(m_pRegisters->m_r12b->m_pAddress)); break;
		case R13B: a.mov(r13b, byte_ptr_abs(m_pRegisters->m_r13b->m_pAddress)); break;
		case R14B: a.mov(r14b, byte_ptr_abs(m_pRegisters->m_r14b->m_pAddress)); break;
		case R15B: a.mov(r15b, byte_ptr_abs(m_pRegisters->m_r15b->m_pAddress)); break;
#endif // ASMJIT_X64

		case AH: a.mov(ah, byte_ptr_abs(m_pRegisters->m_ah->m_pAddress)); break;
		case CH: a.mov(ch, byte_ptr_abs(m_pRegisters->m_ch->m_pAddress)); break;
		case DH: a.mov(dh, byte_ptr_abs(m_pRegisters->m_dh->m_pAddress)); break;
		case BH: a.mov(bh, byte_ptr_abs(m_pRegisters->m_bh->m_pAddress)); break;

		// ========================================================================
		// >> 16-bit General purpose registers
		// ========================================================================
		case AX: a.mov(ax, word_ptr_abs(m_pRegisters->m_ax->m_pAddress)); break;
		case CX: a.mov(cx, word_ptr_abs(m_pRegisters->m_cx->m_pAddress)); break;
		case DX: a.mov(dx, word_ptr_abs(m_pRegisters->m_dx->m_pAddress)); break;
		case BX: a.mov(bx, word_ptr_abs(m_pRegisters->m_bx->m_pAddress)); break;
		case SP: a.mov(sp, word_ptr_abs(m_pRegisters->m_sp->m_pAddress)); break;
		case BP: a.mov(bp, word_ptr_abs(m_pRegisters->m_bp->m_pAddress)); break;
		case SI: a.mov(si, word_ptr_abs(m_pRegisters->m_si->m_pAddress)); break;
		case DI: a.mov(di, word_ptr_abs(m_pRegisters->m_di->m_pAddress)); break;

#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8W: a.mov(r8w, word_ptr_abs(m_pRegisters->m_r8w->m_pAddress)); break;
		case R9W: a.mov(r9w, word_ptr_abs(m_pRegisters->m_r9w->m_pAddress)); break;
		case R10W: a.mov(r10w, word_ptr_abs(m_pRegisters->m_r10w->m_pAddress)); break;
		case R11W: a.mov(r11w, word_ptr_abs(m_pRegisters->m_r11w->m_pAddress)); break;
		case R12W: a.mov(r12w, word_ptr_abs(m_pRegisters->m_r12w->m_pAddress)); break;
		case R13W: a.mov(r13w, word_ptr_abs(m_pRegisters->m_r13w->m_pAddress)); break;
		case R14W: a.mov(r14w, word_ptr_abs(m_pRegisters->m_r14w->m_pAddress)); break;
		case R15W: a.mov(r15w, word_ptr_abs(m_pRegisters->m_r15w->m_pAddress)); break;
#endif // ASMJIT_X64

		// ========================================================================
		// >> 32-bit General purpose registers
		// ========================================================================
		case EAX: a.mov(eax, dword_ptr_abs(m_pRegisters->m_eax->m_pAddress)); break;
		case ECX: a.mov(ecx, dword_ptr_abs(m_pRegisters->m_ecx->m_pAddress)); break;
		case EDX: a.mov(edx, dword_ptr_abs(m_pRegisters->m_edx->m_pAddress)); break;
		case EBX: a.mov(ebx, dword_ptr_abs(m_pRegisters->m_ebx->m_pAddress)); break;
		case ESP: a.mov(esp, dword_ptr_abs(m_pRegisters->m_esp->m_pAddress)); break;
		case EBP: a.mov(ebp, dword_ptr_abs(m_pRegisters->m_ebp->m_pAddress)); break;
		case ESI: a.mov(esi, dword_ptr_abs(m_pRegisters->m_esi->m_pAddress)); break;
		case EDI: a.mov(edi, dword_ptr_abs(m_pRegisters->m_edi->m_pAddress)); break;

#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8D: a.mov(r8d, dword_ptr_abs(m_pRegisters->m_r8d->m_pAddress)); break;
		case R9D: a.mov(r9d, dword_ptr_abs(m_pRegisters->m_r9d->m_pAddress)); break;
		case R10D: a.mov(r10d, dword_ptr_abs(m_pRegisters->m_r10d->m_pAddress)); break;
		case R11D: a.mov(r11d, dword_ptr_abs(m_pRegisters->m_r11d->m_pAddress)); break;
		case R12D: a.mov(r12d, dword_ptr_abs(m_pRegisters->m_r12d->m_pAddress)); break;
		case R13D: a.mov(r13d, dword_ptr_abs(m_pRegisters->m_r13d->m_pAddress)); break;
		case R14D: a.mov(r14d, dword_ptr_abs(m_pRegisters->m_r14d->m_pAddress)); break;
		case R15D: a.mov(r15d, dword_ptr_abs(m_pRegisters->m_r15d->m_pAddress)); break;
#endif // ASMJIT_X64

		// ========================================================================
		// >> 64-bit General purpose registers
		// ========================================================================
#if defined(ASMJIT_X64)
		// 64-bit mode only
		case RAX: a.mov(rax, qword_ptr_abs(m_pRegisters->m_rax->m_pAddress)); break;
		case RCX: a.mov(rcx, qword_ptr_abs(m_pRegisters->m_rcx->m_pAddress)); break;
		case RDX: a.mov(rdx, qword_ptr_abs(m_pRegisters->m_rdx->m_pAddress)); break;
		case RBX: a.mov(rbx, qword_ptr_abs(m_pRegisters->m_rbx->m_pAddress)); break;
		case RSP: a.mov(rsp, qword_ptr_abs(m_pRegisters->m_rsp->m_pAddress)); break;
		case RBP: a.mov(rbp, qword_ptr_abs(m_pRegisters->m_rbp->m_pAddress)); break;
		case RSI: a.mov(rsi, qword_ptr_abs(m_pRegisters->m_rsi->m_pAddress)); break;
		case RDI: a.mov(rdi, qword_ptr_abs(m_pRegisters->m_rdi->m_pAddress)); break;
#endif // ASMJIT_X64

#if defined(ASMJIT_X64)
		// 64-bit mode only
		case R8: a.mov(r8, qword_ptr_abs(m_pRegisters->m_r8->m_pAddress)); break;
		case R9: a.mov(r9, qword_ptr_abs(m_pRegisters->m_r9->m_pAddress)); break;
		case R10: a.mov(r10, qword_ptr_abs(m_pRegisters->m_r10->m_pAddress)); break;
		case R11: a.mov(r11, qword_ptr_abs(m_pRegisters->m_r11->m_pAddress)); break;
		case R12: a.mov(r12, qword_ptr_abs(m_pRegisters->m_r12->m_pAddress)); break;
		case R13: a.mov(r13, qword_ptr_abs(m_pRegisters->m_r13->m_pAddress)); break;
		case R14: a.mov(r14, qword_ptr_abs(m_pRegisters->m_r14->m_pAddress)); break;
		case R15: a.mov(r15, qword_ptr_abs(m_pRegisters->m_r15->m_pAddress)); break;
#endif // ASMJIT_X64

		// ========================================================================
		// >> 64-bit MM (MMX) registers
		// ========================================================================
		case MM0: a.movq(mm0, mmword_ptr_abs(m_pRegisters->m_mm0->m_pAddress)); break;
		case MM1: a.movq(mm1, mmword_ptr_abs(m_pRegisters->m_mm1->m_pAddress)); break;
		case MM2: a.movq(mm2, mmword_ptr_abs(m_pRegisters->m_mm2->m_pAddress)); break;
		case MM3: a.movq(mm3, mmword_ptr_abs(m_pRegisters->m_mm3->m_pAddress)); break;
		case MM4: a.movq(mm4, mmword_ptr_abs(m_pRegisters->m_mm4->m_pAddress)); break;
		case MM5: a.movq(mm5, mmword_ptr_abs(m_pRegisters->m_mm5->m_pAddress)); break;
		case MM6: a.movq(mm6, mmword_ptr_abs(m_pRegisters->m_mm6->m_pAddress)); break;
		case MM7: a.movq(mm7, mmword_ptr_abs(m_pRegisters->m_mm7->m_pAddress)); break;

		// ========================================================================
		// >> 128-bit XMM registers
		// ========================================================================
		// TODO: Also provide movups?
		case XMM0: a.movaps(xmm0, dqword_ptr_abs(m_pRegisters->m_xmm0->m_pAddress)); break;
		case XMM1: a.movaps(xmm1, dqword_ptr_abs(m_pRegisters->m_xmm1->m_pAddress)); break;
		case XMM2: a.movaps(xmm2, dqword_ptr_abs(m_pRegisters->m_xmm2->m_pAddress)); break;
		case XMM3: a.movaps(xmm3, dqword_ptr_abs(m_pRegisters->m_xmm3->m_pAddress)); break;
		case XMM4: a.movaps(xmm4, dqword_ptr_abs(m_pRegisters->m_xmm4->m_pAddress)); break;
		case XMM5: a.movaps(xmm5, dqword_ptr_abs(m_pRegisters->m_xmm5->m_pAddress)); break;
		case XMM6: a.movaps(xmm6, dqword_ptr_abs(m_pRegisters->m_xmm6->m_pAddress)); break;
		case XMM7: a.movaps(xmm7, dqword_ptr_abs(m_pRegisters->m_xmm7->m_pAddress)); break;

#if defined(ASMJIT_X64)
		// 64-bit mode only
		case XMM8: a.movaps(xmm8, dqword_ptr_abs(m_pRegisters->m_xmm8->m_pAddress)); break;
		case XMM9: a.movaps(xmm9, dqword_ptr_abs(m_pRegisters->m_xmm9->m_pAddress)); break;
		case XMM10: a.movaps(xmm10, dqword_ptr_abs(m_pRegisters->m_xmm10->m_pAddress)); break;
		case XMM11: a.movaps(xmm11, dqword_ptr_abs(m_pRegisters->m_xmm11->m_pAddress)); break;
		case XMM12: a.movaps(xmm12, dqword_ptr_abs(m_pRegisters->m_xmm12->m_pAddress)); break;
		case XMM13: a.movaps(xmm13, dqword_ptr_abs(m_pRegisters->m_xmm13->m_pAddress)); break;
		case XMM14: a.movaps(xmm14, dqword_ptr_abs(m_pRegisters->m_xmm14->m_pAddress)); break;
		case XMM15: a.movaps(xmm15, dqword_ptr_abs(m_pRegisters->m_xmm15->m_pAddress)); break;
#endif // ASMJIT_X64

		// ========================================================================
		// >> 16-bit Segment registers
		// ========================================================================
		case CS: a.mov(cs, word_ptr_abs(m_pRegisters->m_cs->m_pAddress)); break;
		case SS: a.mov(ss, word_ptr_abs(m_pRegisters->m_ss->m_pAddress)); break;
		case DS: a.mov(ds, word_ptr_abs(m_pRegisters->m_ds->m_pAddress)); break;
		case ES: a.mov(es, word_ptr_abs(m_pRegisters->m_es->m_pAddress)); break;
		case FS: a.mov(fs, word_ptr_abs(m_pRegisters->m_fs->m_pAddress)); break;
		case GS: a.mov(gs, word_ptr_abs(m_pRegisters->m_gs->m_pAddress)); break;

		// ========================================================================
		// >> 80-bit FPU registers
		// ========================================================================
		case ST0:
		{
			switch(GetDataTypeSize(this->m_pCallingConvention->m_returnType))
			{
				case SIZE_DWORD: a.fld(dword_ptr_abs(m_pRegisters->m_st0->m_pAddress)); break;
				case SIZE_QWORD: a.fld(qword_ptr_abs(m_pRegisters->m_st0->m_pAddress)); break;
				case SIZE_TWORD: a.fld(tword_ptr_abs(m_pRegisters->m_st0->m_pAddress)); break;
			}
			break;
		}
		//case ST1: a.mov(st1, tword_ptr_abs(m_pRegisters->m_st1->m_pAddress)); break;
		//case ST2: a.mov(st2, tword_ptr_abs(m_pRegisters->m_st2->m_pAddress)); break;
		//case ST3: a.mov(st3, tword_ptr_abs(m_pRegisters->m_st3->m_pAddress)); break;
		//case ST4: a.mov(st4, tword_ptr_abs(m_pRegisters->m_st4->m_pAddress)); break;
		//case ST5: a.mov(st5, tword_ptr_abs(m_pRegisters->m_st5->m_pAddress)); break;
		//case ST6: a.mov(st6, tword_ptr_abs(m_pRegisters->m_st6->m_pAddress)); break;
		//case ST7: a.mov(st7, tword_ptr_abs(m_pRegisters->m_st7->m_pAddress)); break;

		default: puts("Unsupported register.");
		}
	}
}
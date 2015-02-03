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

#ifndef _REGISTERS_H
#define _REGISTERS_H

class CRegisters
{
public:
	CRegisters();
	~CRegisters();

public:
	template<class T>
	T GetValue(void* pPtr, int iOffset=0)
	{
		return *(T *) ((*(unsigned long *) pPtr) + iOffset);
	}

	template<class T>
	void SetValue(void* pPtr, T value, int iOffset=0)
	{
		*(T *) ((*(unsigned long *) pPtr) + iOffset) = value;
	}

public:
	// ========================================================================
	// >> 8-bit General purpose registers
	// ========================================================================
	void* m_al;
	void* m_cl;
	void* m_dl;
	void* m_bl;

	// 64-bit mode only
	void* m_spl;
	void* m_bpl;
	void* m_sil;
	void* m_dil;
	void* m_r8b;
	void* m_r9b;
	void* m_r10b;
	void* m_r11b;
	void* m_r12b;
	void* m_r13b;
	void* m_r14b;
	void* m_r15b;

	void* m_ah;
	void* m_ch;
	void* m_dh;
	void* m_bh;

	// ========================================================================
	// >> 16-bit General purpose registers
	// ========================================================================
	void* m_ax;
	void* m_cx;
	void* m_dx;
	void* m_bx;
	void* m_sp;
	void* m_bp;
	void* m_si;
	void* m_di;

	// 64-bit mode only
	void* m_r8w;
	void* m_r9w;
	void* m_r10w;
	void* m_r11w;
	void* m_r12w;
	void* m_r13w;
	void* m_r14w;
	void* m_r15w;

	// ========================================================================
	// >> 32-bit General purpose registers
	// ========================================================================
	void* m_eax;
	void* m_ecx;
	void* m_edx;
	void* m_ebx;
	void* m_esp;
	void* m_ebp;
	void* m_esi;
	void* m_edi;

	// 64-bit mode only
	void* m_r8d;
	void* m_r9d;
	void* m_r10d;
	void* m_r11d;
	void* m_r12d;
	void* m_r13d;
	void* m_r14d;
	void* m_r15d;

	// ========================================================================
	// >> 64-bit General purpose registers
	// ========================================================================
	// 64-bit mode only
	void* m_rax;
	void* m_rcx;
	void* m_rdx;
	void* m_rbx;
	void* m_rsp;
	void* m_rbp;
	void* m_rsi;
	void* m_rdi;
	
	// 64-bit mode only
	void* m_r8;
	void* m_r9;
	void* m_r10;
	void* m_r11;
	void* m_r12;
	void* m_r13;
	void* m_r14;
	void* m_r15;

	// ========================================================================
	// >> 64-bit MM (MMX) registers
	// ========================================================================
	void* m_mm0;
	void* m_mm1;
	void* m_mm2;
	void* m_mm3;
	void* m_mm4;
	void* m_mm5;
	void* m_mm6;
	void* m_mm7;

	// ========================================================================
	// >> 128-bit XMM registers
	// ========================================================================
	void* m_xmm0;
	void* m_xmm1;
	void* m_xmm2;
	void* m_xmm3;
	void* m_xmm4;
	void* m_xmm5;
	void* m_xmm6;
	void* m_xmm7;

	// 64-bit mode only
	void* m_xmm8;
	void* m_xmm9;
	void* m_xmm10;
	void* m_xmm11;
	void* m_xmm12;
	void* m_xmm13;
	void* m_xmm14;
	void* m_xmm15;

	// ========================================================================
	// >> 16-bit Segment registers
	// ========================================================================
	void* m_cs;
	void* m_ss;
	void* m_ds;
	void* m_es;
	void* m_fs;
	void* m_gs;

	// ========================================================================
	// >> 80-bit FPU registers
	// ========================================================================
	void* m_st0;
	void* m_st1;
	void* m_st2;
	void* m_st3;
	void* m_st4;
	void* m_st5;
	void* m_st6;
	void* m_st7;
};


// ============================================================================
// >> Register_t
// ============================================================================
enum Register_t
{
	// ========================================================================
	// >> 8-bit General purpose registers
	// ========================================================================
	AL,
	CL,
	DL,
	BL,

	// 64-bit mode only
	SPL,
	BPL,
	SIL,
	DIL,
	R8B,
	R9B,
	R10B,
	R11B,
	R12B,
	R13B,
	R14B,
	R15B,

	AH,
	CH,
	DH,
	BH,

	// ========================================================================
	// >> 16-bit General purpose registers
	// ========================================================================
	AX,
	CX,
	DX,
	BX,
	SP,
	BP,
	SI,
	DI,

	// 64-bit mode only
	R8W,
	R9W,
	R10W,
	R11W,
	R12W,
	R13W,
	R14W,
	R15W,

	// ========================================================================
	// >> 32-bit General purpose registers
	// ========================================================================
	EAX,
	ECX,
	EDX,
	EBX,
	ESP,
	EBP,
	ESI,
	EDI,
	
	// 64-bit mode only
	R8D,
	R9D,
	R10D,
	R11D,
	R12D,
	R13D,
	R14D,
	R15D,
	
	// ========================================================================
	// >> 64-bit General purpose registers
	// ========================================================================
	// 64-bit mode only
	RAX,
	RCX,
	RDX,
	RBX,
	RSP,
	RBP,
	RSI,
	RDI,

	// 64-bit mode only
	R8,
	R9,
	R10,
	R11,
	R12,
	R13,
	R14,
	R15,

	// ========================================================================
	// >> 64-bit MM (MMX) registers
	// ========================================================================
	MM0,
	MM1,
	MM2,
	MM3,
	MM4,
	MM5,
	MM6,
	MM7,

	// ========================================================================
	// >> 128-bit XMM registers
	// ========================================================================
	XMM0,
	XMM1,
	XMM2,
	XMM3,
	XMM4,
	XMM5,
	XMM6,
	XMM7,

	// 64-bit mode only
	XMM8,
	XMM9,
	XMM10,
	XMM11,
	XMM12,
	XMM13,
	XMM14,
	XMM15,

	// ========================================================================
	// >> 16-bit Segment registers
	// ========================================================================
	CS,
	SS,
	DS,
	ES,
	FS,
	GS,

	// ========================================================================
	// >> 80-bit FPU registers
	// ========================================================================
	ST0,
	ST1,
	ST2,
	ST3,
	ST4,
	ST5,
	ST6,
	ST7,
};

#endif // _REGISTERS_H
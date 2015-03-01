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

#include "registers.h"

CRegisters::CRegisters()
{	
	// ========================================================================
	// >> 8-bit General purpose registers
	// ========================================================================
	m_al = new CRegister(1);
	m_cl = new CRegister(1);
	m_dl = new CRegister(1);
	m_bl = new CRegister(1);

	// 64-bit mode only
	m_spl = new CRegister(1);
	m_bpl = new CRegister(1);
	m_sil = new CRegister(1);
	m_dil = new CRegister(1);
	m_r8b = new CRegister(1);
	m_r9b = new CRegister(1);
	m_r10b = new CRegister(1);
	m_r11b = new CRegister(1);
	m_r12b = new CRegister(1);
	m_r13b = new CRegister(1);
	m_r14b = new CRegister(1);
	m_r15b = new CRegister(1);

	m_ah = new CRegister(1);
	m_ch = new CRegister(1);
	m_dh = new CRegister(1);
	m_bh = new CRegister(1);
	
	// ========================================================================
	// >> 16-bit General purpose registers
	// ========================================================================
	m_ax = new CRegister(2);
	m_cx = new CRegister(2);
	m_dx = new CRegister(2);
	m_bx = new CRegister(2);
	m_sp = new CRegister(2);
	m_bp = new CRegister(2);
	m_si = new CRegister(2);
	m_di = new CRegister(2);

	// 64-bit mode only
	m_r8w = new CRegister(2);
	m_r9w = new CRegister(2);
	m_r10w = new CRegister(2);
	m_r11w = new CRegister(2);
	m_r12w = new CRegister(2);
	m_r13w = new CRegister(2);
	m_r14w = new CRegister(2);
	m_r15w = new CRegister(2);

	// ========================================================================
	// >> 32-bit General purpose registers
	// ========================================================================
	m_eax = new CRegister(4);
	m_ecx = new CRegister(4);
	m_edx = new CRegister(4);
	m_ebx = new CRegister(4);
	m_esp = new CRegister(4);
	m_ebp = new CRegister(4);
	m_esi = new CRegister(4);
	m_edi = new CRegister(4);

	// 64-bit mode only
	m_r8d = new CRegister(4);
	m_r9d = new CRegister(4);
	m_r10d = new CRegister(4);
	m_r11d = new CRegister(4);
	m_r12d = new CRegister(4);
	m_r13d = new CRegister(4);
	m_r14d = new CRegister(4);
	m_r15d = new CRegister(4);

	// ========================================================================
	// >> 64-bit General purpose registers
	// ========================================================================
	// 64-bit mode only
	m_rax = new CRegister(8);
	m_rcx = new CRegister(8);
	m_rdx = new CRegister(8);
	m_rbx = new CRegister(8);
	m_rsp = new CRegister(8);
	m_rbp = new CRegister(8);
	m_rsi = new CRegister(8);
	m_rdi = new CRegister(8);
	
	// 64-bit mode only
	m_r8 = new CRegister(8);
	m_r9 = new CRegister(8);
	m_r10 = new CRegister(8);
	m_r11 = new CRegister(8);
	m_r12 = new CRegister(8);
	m_r13 = new CRegister(8);
	m_r14 = new CRegister(8);
	m_r15 = new CRegister(8);

	// ========================================================================
	// >> 64-bit MM (MMX) registers
	// ========================================================================
	m_mm0 = new CRegister(8);
	m_mm1 = new CRegister(8);
	m_mm2 = new CRegister(8);
	m_mm3 = new CRegister(8);
	m_mm4 = new CRegister(8);
	m_mm5 = new CRegister(8);
	m_mm6 = new CRegister(8);
	m_mm7 = new CRegister(8);

	// ========================================================================
	// >> 128-bit XMM registers
	// ========================================================================
	m_xmm0 = new CRegister(16);
	m_xmm1 = new CRegister(16);
	m_xmm2 = new CRegister(16);
	m_xmm3 = new CRegister(16);
	m_xmm4 = new CRegister(16);
	m_xmm5 = new CRegister(16);
	m_xmm6 = new CRegister(16);
	m_xmm7 = new CRegister(16);

	// 64-bit mode only
	m_xmm8 = new CRegister(16);
	m_xmm9 = new CRegister(16);
	m_xmm10 = new CRegister(16);
	m_xmm11 = new CRegister(16);
	m_xmm12 = new CRegister(16);
	m_xmm13 = new CRegister(16);
	m_xmm14 = new CRegister(16);
	m_xmm15 = new CRegister(16);

	// ========================================================================
	// >> 16-bit Segment registers
	// ========================================================================
	m_cs = new CRegister(2);
	m_ss = new CRegister(2);
	m_ds = new CRegister(2);
	m_es = new CRegister(2);
	m_fs = new CRegister(2);
	m_gs = new CRegister(2);
	
	// ========================================================================
	// >> 80-bit FPU registers
	// ========================================================================
	m_st0 = new CRegister(10);
	m_st1 = new CRegister(10);
	m_st2 = new CRegister(10);
	m_st3 = new CRegister(10);
	m_st4 = new CRegister(10);
	m_st5 = new CRegister(10);
	m_st6 = new CRegister(10);
	m_st7 = new CRegister(10);
}

CRegisters::~CRegisters()
{
	// ========================================================================
	// >> 8-bit General purpose registers
	// ========================================================================
	delete m_al;
	delete m_cl;
	delete m_dl;
	delete m_bl;

	// 64-bit mode only
	delete m_spl;
	delete m_bpl;
	delete m_sil;
	delete m_dil;
	delete m_r8b;
	delete m_r9b;
	delete m_r10b;
	delete m_r11b;
	delete m_r12b;
	delete m_r13b;
	delete m_r14b;
	delete m_r15b;

	delete m_ah;
	delete m_ch;
	delete m_dh;
	delete m_bh;
	
	// ========================================================================
	// >> 16-bit General purpose registers
	// ========================================================================
	delete m_ax;
	delete m_cx;
	delete m_dx;
	delete m_bx;
	delete m_sp;
	delete m_bp;
	delete m_si;
	delete m_di;

	// 64-bit mode only
	delete m_r8w;
	delete m_r9w;
	delete m_r10w;
	delete m_r11w;
	delete m_r12w;
	delete m_r13w;
	delete m_r14w;
	delete m_r15w;

	// ========================================================================
	// >> 32-bit General purpose registers
	// ========================================================================
	delete m_eax;
	delete m_ecx;
	delete m_edx;
	delete m_ebx;
	delete m_esp;
	delete m_ebp;
	delete m_esi;
	delete m_edi;

	// 64-bit mode only
	delete m_r8d;
	delete m_r9d;
	delete m_r10d;
	delete m_r11d;
	delete m_r12d;
	delete m_r13d;
	delete m_r14d;
	delete m_r15d;

	// ========================================================================
	// >> 64-bit General purpose registers
	// ========================================================================
	// 64-bit mode only
	delete m_rax;
	delete m_rcx;
	delete m_rdx;
	delete m_rbx;
	delete m_rsp;
	delete m_rbp;
	delete m_rsi;
	delete m_rdi;
	
	// 64-bit mode only
	delete m_r8;
	delete m_r9;
	delete m_r10;
	delete m_r11;
	delete m_r12;
	delete m_r13;
	delete m_r14;
	delete m_r15;

	// ========================================================================
	// >> 64-bit MM (MMX) registers
	// ========================================================================
	delete m_mm0;
	delete m_mm1;
	delete m_mm2;
	delete m_mm3;
	delete m_mm4;
	delete m_mm5;
	delete m_mm6;
	delete m_mm7;

	// ========================================================================
	// >> 128-bit XMM registers
	// ========================================================================
	delete m_xmm0;
	delete m_xmm1;
	delete m_xmm2;
	delete m_xmm3;
	delete m_xmm4;
	delete m_xmm5;
	delete m_xmm6;
	delete m_xmm7;

	// 64-bit mode only
	delete m_xmm8;
	delete m_xmm9;
	delete m_xmm10;
	delete m_xmm11;
	delete m_xmm12;
	delete m_xmm13;
	delete m_xmm14;
	delete m_xmm15;

	// ========================================================================
	// >> 2-bit Segment registers
	// ========================================================================
	delete m_cs;
	delete m_ss;
	delete m_ds;
	delete m_es;
	delete m_fs;
	delete m_gs;
	
	// ========================================================================
	// >> 80-bit FPU registers
	// ========================================================================
	delete m_st0;
	delete m_st1;
	delete m_st2;
	delete m_st3;
	delete m_st4;
	delete m_st5;
	delete m_st6;
	delete m_st7;
}
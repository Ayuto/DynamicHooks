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
#include <stdlib.h>

CRegisters::CRegisters()
{	
	// ========================================================================
	// >> 8-bit General purpose registers
	// ========================================================================
	m_al = malloc(1);
	m_cl = malloc(1);
	m_dl = malloc(1);
	m_bl = malloc(1);

	// 64-bit mode only
	m_spl = malloc(1);
	m_bpl = malloc(1);
	m_sil = malloc(1);
	m_dil = malloc(1);
	m_r8b = malloc(1);
	m_r9b = malloc(1);
	m_r10b = malloc(1);
	m_r11b = malloc(1);
	m_r12b = malloc(1);
	m_r13b = malloc(1);
	m_r14b = malloc(1);
	m_r15b = malloc(1);

	m_ah = malloc(1);
	m_ch = malloc(1);
	m_dh = malloc(1);
	m_bh = malloc(1);
	
	// ========================================================================
	// >> 16-bit General purpose registers
	// ========================================================================
	m_ax = malloc(2);
	m_cx = malloc(2);
	m_dx = malloc(2);
	m_bx = malloc(2);
	m_sp = malloc(2);
	m_bp = malloc(2);
	m_si = malloc(2);
	m_di = malloc(2);

	// 64-bit mode only
	m_r8w = malloc(2);
	m_r9w = malloc(2);
	m_r10w = malloc(2);
	m_r11w = malloc(2);
	m_r12w = malloc(2);
	m_r13w = malloc(2);
	m_r14w = malloc(2);
	m_r15w = malloc(2);

	// ========================================================================
	// >> 32-bit General purpose registers
	// ========================================================================
	m_eax = malloc(4);
	m_ecx = malloc(4);
	m_edx = malloc(4);
	m_ebx = malloc(4);
	m_esp = malloc(4);
	m_ebp = malloc(4);
	m_esi = malloc(4);
	m_edi = malloc(4);

	// 64-bit mode only
	m_r8d = malloc(4);
	m_r9d = malloc(4);
	m_r10d = malloc(4);
	m_r11d = malloc(4);
	m_r12d = malloc(4);
	m_r13d = malloc(4);
	m_r14d = malloc(4);
	m_r15d = malloc(4);

	// ========================================================================
	// >> 64-bit General purpose registers
	// ========================================================================
	// 64-bit mode only
	m_rax = malloc(8);
	m_rcx = malloc(8);
	m_rdx = malloc(8);
	m_rbx = malloc(8);
	m_rsp = malloc(8);
	m_rbp = malloc(8);
	m_rsi = malloc(8);
	m_rdi = malloc(8);
	
	// 64-bit mode only
	m_r8 = malloc(8);
	m_r9 = malloc(8);
	m_r10 = malloc(8);
	m_r11 = malloc(8);
	m_r12 = malloc(8);
	m_r13 = malloc(8);
	m_r14 = malloc(8);
	m_r15 = malloc(8);

	// ========================================================================
	// >> 64-bit MM (MMX) registers
	// ========================================================================
	m_mm0 = malloc(8);
	m_mm1 = malloc(8);
	m_mm2 = malloc(8);
	m_mm3 = malloc(8);
	m_mm4 = malloc(8);
	m_mm5 = malloc(8);
	m_mm6 = malloc(8);
	m_mm7 = malloc(8);

	// ========================================================================
	// >> 128-bit XMM registers
	// ========================================================================
	m_xmm0 = malloc(16);
	m_xmm1 = malloc(16);
	m_xmm2 = malloc(16);
	m_xmm3 = malloc(16);
	m_xmm4 = malloc(16);
	m_xmm5 = malloc(16);
	m_xmm6 = malloc(16);
	m_xmm7 = malloc(16);

	// 64-bit mode only
	m_xmm8 = malloc(16);
	m_xmm9 = malloc(16);
	m_xmm10 = malloc(16);
	m_xmm11 = malloc(16);
	m_xmm12 = malloc(16);
	m_xmm13 = malloc(16);
	m_xmm14 = malloc(16);
	m_xmm15 = malloc(16);

	// ========================================================================
	// >> 16-bit Segment registers
	// ========================================================================
	m_cs = malloc(2);
	m_ss = malloc(2);
	m_ds = malloc(2);
	m_es = malloc(2);
	m_fs = malloc(2);
	m_gs = malloc(2);
	
	// ========================================================================
	// >> 80-bit FPU registers
	// ========================================================================
	m_st0 = malloc(10);
	m_st1 = malloc(10);
	m_st2 = malloc(10);
	m_st3 = malloc(10);
	m_st4 = malloc(10);
	m_st5 = malloc(10);
	m_st6 = malloc(10);
	m_st7 = malloc(10);
}

CRegisters::~CRegisters()
{
	// ========================================================================
	// >> 8-bit General purpose registers
	// ========================================================================
	free(m_al);
	free(m_cl);
	free(m_dl);
	free(m_bl);

	// 64-bit mode only
	free(m_spl);
	free(m_bpl);
	free(m_sil);
	free(m_dil);
	free(m_r8b);
	free(m_r9b);
	free(m_r10b);
	free(m_r11b);
	free(m_r12b);
	free(m_r13b);
	free(m_r14b);
	free(m_r15b);

	free(m_ah);
	free(m_ch);
	free(m_dh);
	free(m_bh);
	
	// ========================================================================
	// >> 16-bit General purpose registers
	// ========================================================================
	free(m_ax);
	free(m_cx);
	free(m_dx);
	free(m_bx);
	free(m_sp);
	free(m_bp);
	free(m_si);
	free(m_di);

	// 64-bit mode only
	free(m_r8w);
	free(m_r9w);
	free(m_r10w);
	free(m_r11w);
	free(m_r12w);
	free(m_r13w);
	free(m_r14w);
	free(m_r15w);

	// ========================================================================
	// >> 32-bit General purpose registers
	// ========================================================================
	free(m_eax);
	free(m_ecx);
	free(m_edx);
	free(m_ebx);
	free(m_esp);
	free(m_ebp);
	free(m_esi);
	free(m_edi);

	// 64-bit mode only
	free(m_r8d);
	free(m_r9d);
	free(m_r10d);
	free(m_r11d);
	free(m_r12d);
	free(m_r13d);
	free(m_r14d);
	free(m_r15d);

	// ========================================================================
	// >> 64-bit General purpose registers
	// ========================================================================
	// 64-bit mode only
	free(m_rax);
	free(m_rcx);
	free(m_rdx);
	free(m_rbx);
	free(m_rsp);
	free(m_rbp);
	free(m_rsi);
	free(m_rdi);
	
	// 64-bit mode only
	free(m_r8);
	free(m_r9);
	free(m_r10);
	free(m_r11);
	free(m_r12);
	free(m_r13);
	free(m_r14);
	free(m_r15);

	// ========================================================================
	// >> 64-bit MM (MMX) registers
	// ========================================================================
	free(m_mm0);
	free(m_mm1);
	free(m_mm2);
	free(m_mm3);
	free(m_mm4);
	free(m_mm5);
	free(m_mm6);
	free(m_mm7);

	// ========================================================================
	// >> 128-bit XMM registers
	// ========================================================================
	free(m_xmm0);
	free(m_xmm1);
	free(m_xmm2);
	free(m_xmm3);
	free(m_xmm4);
	free(m_xmm5);
	free(m_xmm6);
	free(m_xmm7);

	// 64-bit mode only
	free(m_xmm8);
	free(m_xmm9);
	free(m_xmm10);
	free(m_xmm11);
	free(m_xmm12);
	free(m_xmm13);
	free(m_xmm14);
	free(m_xmm15);

	// ========================================================================
	// >> 2-bit Segment registers
	// ========================================================================
	free(m_cs);
	free(m_ss);
	free(m_ds);
	free(m_es);
	free(m_fs);
	free(m_gs);
	
	// ========================================================================
	// >> 80-bit FPU registers
	// ========================================================================
	free(m_st0);
	free(m_st1);
	free(m_st2);
	free(m_st3);
	free(m_st4);
	free(m_st5);
	free(m_st6);
	free(m_st7);
}
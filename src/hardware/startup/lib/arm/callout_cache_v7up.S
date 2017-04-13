#
# Copyright 2015, QNX Software Systems. 
# 
# Licensed under the Apache License, Version 2.0 (the "License"). You 
# may not reproduce, modify or distribute this software except in 
# compliance with the License. You may obtain a copy of the License 
# at: http://www.apache.org/licenses/LICENSE-2.0 
# 
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" basis, 
# WITHOUT WARRANTIES OF ANY KIND, either express or implied.
#
# This file may contain contributions from others, either as 
# contributors under the License or as licensors under other terms.  
# Please review this entire file for other proprietary rights or license 
# notices, as well as the QNX Development Suite License Guide at 
# http://licensing.qnx.com/license-guide/ for other information.
#

/*
 * Generic ARMv7 uniprocessor cache callouts
 *
 * unsigned control(paddr32_t base,
 *					unsigned num_lines,
 *					int flags,
 *					struct cacheattr_entry *cache,
 *					volatile struct syspage_entry * )
 */

#include "callout.ah"

#define	MAX_LINES	32
#define ILINE_LIMIT	1024

.macro	ilinesz	reg mask
	mrc		p15, 0, \mask, c0, c0, 1
	and		\mask, \mask, #0xf
	mov		\reg, #4
	mov		\reg, \reg, lsl \mask		// linesize = 4 << Iminline
	sub		\mask, \reg, #1
.endm

.macro	dlinesz	reg mask
	mrc		p15, 0, \mask, c0, c0, 1
	ubfx	\mask, \mask, #16, #4
	mov		\reg, #4
	mov		\reg, \reg, lsl \mask		// linesize = 4 << Dminline
	sub		\mask, \reg, #1
.endm

CALLOUT_START(cache_v7up_i, 0, 0)
	/*
	 * For large ranges, just flush the whole cache
	 */
	cmp		r1, #ILINE_LIMIT
	blo		0f
	mov		r0, #0
	mcr		p15, 0, r0, c7, c5, 0		// ICIALLU
	dsb	
	isb
	mcr		p15, 0, r1, c7, c1, 6		// BPIALLIS  (broadcast)
	dsb	
	isb
	mov		pc, lr

0:	ilinesz	ip, r3

	/*
	 * Trim the address to a cache line boundary and stop at 32 lines
	 * to avoid having to re-issue the whole flush if we get preempted
	 */
	bic		r3, r0, r3
	cmp		r1, #MAX_LINES
	movhi	r1, #MAX_LINES
	mov		r0, r1
0:	mcr		p15, 0, r3, c7, c5, 1		// ICIMVAU
	add		r3, r3, ip
	subs	r1, r1, #1
	bne		0b
	dsb	
	isb
	mcr		p15, 0, r1, c7, c5, 6		// BPIALL
	dsb	
	isb
	mov		pc, lr
CALLOUT_END(cache_v7up_i)

CALLOUT_START(cache_v7up_d, 0, 0)
	/*
	 * Trim the address to a cache line boundary and strop at 32 lines
	 * to avoid having to re-issue the whole flush if we get preempted
	 */
	dlinesz	ip, r3
	bic		r3, r0, r3
	cmp		r1, #MAX_LINES
	movhi	r1, #MAX_LINES
	mov		r0, r1

	tst		r2, #MS_INVALIDATE
	bne		1f

	/*
	 * Clean lines by address
	 */
0:	mcr		p15, 0, r3, c7, c10, 1		// DCCMVAC
	add		r3, r3, ip
	subs	r1, r1, #1
	bne		0b
	dsb	
	isb
	mov		pc, lr

	/*
	 * Clean and invalidate lines by address
	 */
1:	mcr		p15, 0, r3, c7, c14, 1		// DCCIMVAC
	add		r3, r3, ip
	subs	r1, r1, #1
	bne		1b
	dsb
	isb
	mov		pc, lr
CALLOUT_END(cache_v7up_d)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/callout_cache_v7up.S $ $Rev: 781278 $";
.previous
#endif
#endif 

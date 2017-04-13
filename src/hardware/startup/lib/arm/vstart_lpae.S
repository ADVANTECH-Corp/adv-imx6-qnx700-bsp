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

	.text
	.align 2
	.globl vstart_lpae

/*
 * void	vstart_lpae(uintptr_t syspageptr, unsigned entry_point, unsigned cpunum)
 *
 * Enable the mmu and jump to the next program's entry point
 * The next program is responsible for tearing down the 1-1 section
 * mapping of the startup program set up by init_mmu().
 */
vstart_lpae:
	mov		r4, r1
	mov		r5, r2

	/*
	 * Set MAIR0 and MAIR1
	 */
	ldr		ip, =lpae_mair0
	ldr		ip, [ip]
	mcr		p15, 0, ip, c10, c2, 0

	ldr		ip, =lpae_mair1
	ldr		ip, [ip]
	mcr		p15, 0, ip, c10, c2, 1

	/*
	 * Set TTBCR and TTBR0/1
	 */
	ldr		ip, =lpae_ttbcr
	ldr		ip, [ip]
	mcr		p15, 0, ip, c2, c0, 2		// TTBCR

	ldr		ip, =lpae_ttbr0
	add		ip, ip, r5, lsl #3			// &ttbr0[cpu]
	ldrd	r6, r7, [ip]
.ifdef	VARIANT_le
	mcrr	p15, 0, r6, r7, c2			// TTBR0
.else
	mcrr	p15, 0, r7, r6, c2			// TTBR0
.endif

	ldr		ip, =lpae_ttbr1
	add		ip, ip, r5, lsl #3			// &ttbr1[cpu]
	ldrd	r6, r7, [ip]
.ifdef	VARIANT_le
	mcrr	p15, 1, r6, r7, c2			// TTBR1
.else
	mcrr	p15, 1, r7, r6, c2			// TTBR1
.endif

	/*
	 * Enable the MMU, using read-modify-write to preserve reserved bits.
	 */
	ldr		r2, =sctlr_clr
	ldr		r3, =sctlr_set
	ldr		r2, [r2]
	ldr		r3, [r3]
	mrc		p15, 0, lr, c1, c0, 0
	bic		ip, lr, r2
	orr		ip, ip, r3
	dsb
	isb
	mcr		p15, 0, ip, c1, c0, 0
	dsb
	isb
	mov		ip, #0

	/*
	 * Invalidate the caches and TLBs
	 */
	.align 5
	mcr		p15, 0, ip, c7, c5, 0	// invalidate instruction caches
	mcr		p15, 0, ip, c8, c7, 0	// invalidate TLBs
	dsb
	isb

	/*
	 * Call entry_point(_syspage_ptr, cpunum)
	 */
	mov		r1, r5
	mov		pc, r4

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/vstart_lpae.S $ $Rev: 784257 $";
.previous
#endif
#endif 

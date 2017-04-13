#
# Copyright 2007, 2008, QNX Software Systems.
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
 * cstart.S
 *
 *	Initial code to get Neutrino started
 *
 * This code runs as the first instructions after control has transferred
 * from any bootup monitor.  This module defines a minimal bootup stack,
 * and sets SP to this stack, and then starts running C code in _main().
 */

	.data
	.globl	stack
	.globl	stack_addr_lo
	.globl	stack_addr_hi
	.globl	boot_args
	.globl	boot_regs

    .align 8
boot_args:
	.ascii	"ddpvbskr"	/* signature for mkifs */
stack_addr_lo:
	.space	64*1024
stack_addr_hi:
stack:

	.text
	.extern	_main
	.globl	cstart

cstart:
	/*
	 * NOTE: No absolute code/data references until we hit the
	 * "boot_verify_load_addr" label!
	 */
	bl		boot_verify_load_addr
	bl		boot_switch_to_phys
	bl		boot_save_parameters
	bl		boot_switch_to_svc
	bl		boot_switch_to_stack

	bl		_main

oops:
	b		oops



	/*
	 * These are done as a global routines so that _start label overrides
	 * can call them earlier if need be.
	 */

	/*
	 * Make sure image has been loaded at the correct address in memory
	 *
	 */
	.global	boot_verify_load_addr
boot_verify_load_addr:
	sub     r5,pc,#8
	ldr		r4,=boot_verify_load_addr
	cmp		r5,r4
1:
	bne		1b
	mov		pc,lr


	 /*
	  * Turn off interrupts and make sure we are in SVC mode
	  */
	.global boot_switch_to_svc
boot_switch_to_svc:
	cpsie   a
    mrs		ip, cpsr
	bic		ip, ip, #0x1f
	orr		ip, ip, #0xd3
	msr		cpsr, ip
	mov		pc,lr

	/*
	 * Turn off MMU and data cache if necessary.
	 * WARNING: assumes we are running with a 1-1 mapping if MMU is enabled.
	 */
	.global boot_switch_to_phys
boot_switch_to_phys:
	mrc     p15, 0, ip, c1, c0, 0
	tst     ip, #0x05       // MMU/D-cache enabled?
	moveq	pc,lr			// return if not
	mov		r4,lr			// save return value
	// load stack pointer with something safe - avoid bringing in cache
	// lines that might have other stuff in them.
	ldr		sp, =(stack - 1024)
	bl	    arm_v7_dcache_flush
	dsb
	mrc		p15, 0, ip, c1, c0, 0
	bic		ip, ip, #0x00000007			// CAM bits
	mcr		p15, 0, ip, c1, c0, 0
	isb
	mov		r0, #0
	mov		r0, r0
	mov		r0, r0

	/*
	 * Invalidate instruction cache and TLBs.
	 * WARNING: assumes data caches are clean.
	 */
	mcr		p15, 0, r0, c7, c5, 0		// invalidate instruction caches
	mcr		p15, 0, r0, c8, c7, 0		// invalidate TLBs
	isb

	mov		pc,r4						// return to caller

	/*
	 * Switch to the startup stack
	 */
	.global	boot_switch_to_stack
boot_switch_to_stack:
	ldr		sp, =stack
	mov		pc, lr

    /*
     * Save away the parameter registers passed into startup
     */
	.global	boot_save_parameters
boot_save_parameters:
	ldr		r4,parms_saved
	cmp		r4,#0
	movne	r4,lr			// return if we've already stored the regs
	str		lr,parms_saved	// write a non-zero value so we know they've been saved
    ldr     r4,=boot_regs
    stmia   r4,{r0-r3}
	mov		pc,lr
parms_saved: .word 0

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/cstart.S $ $Rev: 816038 $";
.previous
#endif
#endif

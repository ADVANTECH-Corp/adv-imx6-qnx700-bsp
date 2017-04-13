#
# Copyright 2016, QNX Software Systems.
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
 * spin_smp_start.S
 *
 * - cpu0 returns to perform the startup initialisation
 * - other cpus spin until they are woken up by board_smp_start
 */

	.text
	.align 2

	.global spin_smp_init

spin_smp_init:
	/* NOTE:
	   	Do NOT modify registers R0-R3 before returning on the bootstrap
		processor.
		These registers may contain information provided by the IPL and
		cstart will save them in the boot_regs variable for later perusal
		by other portions of startup.
	*/
	mrc		p15, 0, r9, c0, c0, 5
	tst		r9, #(1 << 30)
	movne	pc, lr	// return if a uniprocessor system
	bic		r9, #0xff000000		// Clear the MT & M bits
	ldr		r8, =spin_bootstrap_id
	ldr		r8, [r8]
	cmp		r9, r8
	moveq	pc, lr
	/*
	 * increment the number of processors so spin_smp_num_cpus()
	 * knows how many processors are present.
	 * This assumes all secondary processors get through this code
	 * before spin_smp_num_cpus() executes on the boot processor.
	 */
	ldr		r1, =spin_num_cpu
0:	ldrex	r9, [r1]
	add		r2, r9, #1
	strex	r3, r2, [r1]
	teq		r3, #1
	beq		0b
	dsb

	/*
	 * Wait until spin_smp_start sets spin_start_addr to our core index
	 */
	ldr		r1,=spin_start_addr
0:	wfe
	dmb		sy
	ldr		r2, [r1]
	lsr		r3, r2, #24	// extract the cpu index
	cmp		r3, r9
	bne		0b
	mov		r3,#0
	str		r3,[r1]				// Say we've seen it
	dmb		sy
	bic		r2, #0xff000000		// Clear the cpu index
	ldr		r1,=_btext			// get base of startup
	add		pc,r2,r1				// turn offset from start into address

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/spin_smp_init.S $ $Rev: 805507 $";
.previous
#endif
#endif

/*
 * $QNXLicenseC:
 * Copyright 2015, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

 #include "callout.ah"
/*
 * --------------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *  r0 - physical address of syspage
 *  r1 - virtual  address of syspage
 *  r2 - offset from start of syspage to start of the callout routine
 *  r3 - offset from start of syspage to read/write data used by callout
 * --------------------------------------------------------------------------
 */
patch_timer:
	stmdb   sp!,{r4,lr}
	add		r4, r0, r2				// address of callout routine

	/*
	 * Map registers
	 */
	mov		r0, #0x240				// timer at offset 0x200
	ldr		r1, =mpcore_scu_base
	bl		callout_io_map_indirect

	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH   r4, r0, r1, r2, ip

	ldmia   sp!,{r4,pc}

/*
 * --------------------------------------------------------------------------
 * Set clock resolution, and enable interrupt triggering as a side effect.
 * The interrupt remains masked until it is enabled via intr_unmask_mpcore
 *
 * On entry:
 *  r0 - pointer to syspage_entry
 *  r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_load_a9gt, 0, patch_timer)
	/*
	 * Get the address of the mpcore scu_base (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000
	add		ip, ip, #0x200			// location of timer registers

	/*
	 * Disable timer
	 */
	ldr		r3, [ip, #0x08]			// control register
	bic		r3, r3, #0x0f
	str		r3, [ip, #0x08]

	mov		r0, #1
	str		r0, [ip, #0x0c]			// clear interrupt status

	/*
	 * Set auto-increment to qtp->timer_load value
	 */
	ldr		r0, [r1, #QT_TIMER_LOAD]
	sub		r0, r0, #1
	str		r0, [ip, #0x18]			// auto increment

	/*
	 * Set comparator to counter + auto_increment
	 */
	ldr		r1, [ip, #0x00]			// counter low
	ldr		r2, [ip, #0x04]			// counter high
	adds	r1, r1, r0
	adc		r2, r2, #0
	str		r1, [ip, #0x10]			// comparator low
	str		r2, [ip, #0x14]			// comparator high

	/*
	 * Enable timer, comparator, interrupt and auto load
	 */
 	orr		r3, r3, #0x0f
 	str		r3, [ip, #0x08]

	mov		r0, #0
	mov		pc, lr
CALLOUT_END(timer_load_a9gt)


/*
 * --------------------------------------------------------------------------
 * Read the current timer value, relative to the last clock tick
 *
 * On entry:
 *  r0 - pointer to syspage_entry
 *  r1 - pointer to qtime_entry
 *
 * FIXME: this doesn't deal with the counter wrapping, eg. ClockCycles just
 *        at the point where the clock interrupt is triggerred.
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_value_a9gt, 0, patch_timer)
	/*
	 * Get the address of the mpcore scu_base (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000
	add		ip, ip, #0x200			// location of timer registers

	/*
	 * FIXME: need to tweak the kernel/startup interface so we can directly
	 *        return the 64-bit counter value to ClockCycles
	 */
0:	ldr		r1, [ip, #0x04]			// counter high
	ldr		r0, [ip, #0x00]			// counter low
	ldr		r2, [ip, #0x04]
	teq		r2, r1
	bne		0b						// counter high value changed - resample
	ldr		r2, [ip, #0x18]			// auto increment
	add		r0, r0, r2				// counter low += auto increment
	ldr		r3, [ip, #0x14]			// comparator high
	ldr		r2, [ip, #0x10]			// comparator low
	ldr		r1, [ip, #0x14]
	teq		r1, r3
	bne		0b						// comparator high changed - resample
	rsb		r0, r2, r0
	mov		pc, lr
CALLOUT_END(timer_value_a9gt)


/*
 * --------------------------------------------------------------------------
 * Clear timer interrupt.
 *
 * On entry:
 *  r0 - pointer to syspage_entry
 *  r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_reload_a9gt, 0, patch_timer)
	/*
	 * Get the address of the mpcore scu_base (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000
	add		ip, ip, #0x200			// location of timer registers

	mov		r0, #1
	str		r0, [ip, #0x0c]			// clear interrupt status

	mov		pc, lr
CALLOUT_END(timer_reload_a9gt)

/*
 * --------------------------------------------------------------------------
 * Return 64-bit counter value:
 * - for LE r0=lo word, r1=hi word
 * - for BE r0=hi word, r1=lo word
 */
CALLOUT_START(clock_cycles_a9gt, 0, patch_timer)
	/*
	 * Get the address of the mpcore scu_base (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000
	add		ip, ip, #0x200			// location of timer registers

.ifdef VARIANT_le
0:	ldr		r1, [ip, #0x04]			// counter high
	ldr		r0, [ip, #0x00]			// counter low
	ldr		r2, [ip, #0x04]
	teq		r2, r1
	bne		0b						// counter high value changed - resample
.else
0:	ldr		r0, [ip, #0x04]			// counter high
	ldr		r1, [ip, #0x00]			// counter low
	ldr		r2, [ip, #0x04]
	teq		r2, r0
	bne		0b						// counter high value changed - resample
.endif

	mov		pc, lr
CALLOUT_END(clock_cycles_a9gt)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/callout_timer_a9gt.S $ $Rev: 781278 $";
.previous
#endif
#endif 

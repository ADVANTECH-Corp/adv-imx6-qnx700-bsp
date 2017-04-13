/*
 * $QNXLicenseC:
 * Copyright 2016, QNX Software Systems.
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


/*
 * Panasonic mn2ws0310 specific timer support using 16-bit Timer7.
 */

#include "callout.ah"
#include <arm/mpcore.h>

.extern	mpcore_scu_base

/*
 * --------------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 * --------------------------------------------------------------------------
 */
patch_timer:
	stmdb	sp!, {r4, lr}
	add		r4, r0, r2					// address of callout routine

	ldr		r0, Lvaddr
	cmp		r0, #-1
	bne		1f

	/*
	 * Map registers
	 */
	mov		r0, #0x1000					// size of Private Memory Area registers
	ldr		r1, Lpaddr
	ldr		r1, [r1]
	bl		callout_io_map
	str		r0, Lvaddr

1:
	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH	r4, r0, r1, r2, ip
	ldmia	sp!, {r4, pc}

Lpaddr:	.word	mpcore_scu_base
Lvaddr:	.word	-1

/*
 * --------------------------------------------------------------------------
 * Set clock resolution, and enable interrupt triggering as a side effect.
 * Clock interval is:
 *
 *   (PRESCALER_value+1) x (AutoIncrement_value)
 *   ------------------------------------
 *              PERIPHCLK
 *
 * The interrupt remains masked until it is enabled via intr_unmask_*
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_load_v7glob, 0, patch_timer)
	/*
	 * Get the address of the Private Memory Area registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * Disable Timer, Comparator, Interrupt and clear PRESCALER_value
	 */
	mov		r3, #0
	str		r3, [ip, #GTIM_CONTROL]

	/*
	 * load counter value
	 */
	ldr		r0, [r1, #QT_TIMER_LOAD]
	str		r0, [ip, #GTIM_AUTOINC]

	/*
	 * set comparator value (current counter + autoload)
	 */
	ldr		r2, [ip, #GTIM_COUNTERL]
	ldr		r3, [ip, #GTIM_COUNTERH]
	adds	r2, r2, r0
	adc		r3, r3, #0
	str		r2, [ip, #GTIM_COMPAREL]
	str		r3, [ip, #GTIM_COMPAREH]

	/*
	 * Restart Timer, Enable autoload, compare, interrupt
	 */
	mov		r3, #0x0f
	str		r3, [ip, #GTIM_CONTROL]

	mov		pc, lr
CALLOUT_END(timer_load_v7glob)


/*
 * --------------------------------------------------------------------------
 * Read the current timer value, relative to the last clock tick
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 *
 * NOTE: this doesn't deal with the counter wrapping, eg. ClockCycles just
 *		  at the point where the clock interrupt is triggerred.
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_value_v7glob, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * Return current timer counter value
	 */
1:	ldr		r0, [ip, #GTIM_COMPAREL]
	ldr		r2, [ip, #GTIM_COUNTERL]
	ldr		r3, [ip, #GTIM_COMPAREL]
	cmp		r0, r3
	bne		1b			// reload if comparator(low) has changed (from an autoload)
	sub		r0, r0, r2	// CompareL - CounterL
	ldr		r2, [r1, #QT_TIMER_LOAD]
	sub		r0, r2, r0	// QT_TIMER_LOAD - (CompareL - CounterL)

	mov		pc, lr
CALLOUT_END(timer_value_v7glob)


/*
 * --------------------------------------------------------------------------
 * Clear timer interrupt.
 *
 * On entry:
 *	r0 - pointer to syspage_entry
 *	r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_reload_v7glob, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * Clear interrupt event
	 */
	ldr		r2, [ip, #GTIM_STATUS]
	str		r2, [ip, #GTIM_STATUS]

	mov		pc, lr
CALLOUT_END(timer_reload_v7glob)

/*
 * --------------------------------------------------------------------------
 * Return 64-bit counter value:
 * - for LE r0=lo word, r1=hi word
 * - for BE r0=hi word, r1=lo word
 */
CALLOUT_START(clock_cycles_v7glob, 0, patch_timer)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

.ifdef VARIANT_le
1:	ldr	r1, [ip, #GTIM_COUNTERH]
	ldr	r0, [ip, #GTIM_COUNTERL]
	ldr	r2, [ip, #GTIM_COUNTERH]
	cmp	r1, r2
	bne	1b
.else
1:	ldr	r0, [ip, #GTIM_COUNTERH]
	ldr	r1, [ip, #GTIM_COUNTERL]
	ldr	r2, [ip, #GTIM_COUNTERH]
	cmp	r0, r2
	bne	1b
.endif
	mov		pc, lr
CALLOUT_END(clock_cycles_v7glob)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/callout_timer_v7glob.S $ $Rev: 811885 $";
.previous
#endif
#endif

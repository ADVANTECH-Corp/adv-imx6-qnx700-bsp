/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems. 
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

#define OMAP44XX_GPTIMER1_GPT_TIDR 0x4a318000
#define OMAP44XX_GPT_OFF_TCRR      0x28

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
patch_clock_cycles:
	stmdb   sp!,{r4,lr}
	add		r4, r0, r2				// address of callout routine

	/*
	 * Map registers
	 */
	mov		r0, #0x1000
	adr		r1, Lbase
	bl		callout_io_map_indirect

	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH   r4, r0, r1, r2, ip

	ldmia   sp!,{r4,pc}

		.align	3
Lbase:	.quad	OMAP44XX_GPTIMER1_GPT_TIDR

/*
 * --------------------------------------------------------------------------
 * Return 64-bit counter value:
 * - for LE r0=lo word, r1=hi word
 * - for BE r0=hi word, r1=lo word
 */
CALLOUT_START(clock_cycles_omap4430, 0, patch_clock_cycles)
	/*
	 * Get the address of the timer registers (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

.ifdef VARIANT_le
0:	ldr		r1, [ip, #OMAP44XX_GPT_OFF_TCRR]	// counter high
	mov		r0, #0								// counter low	
.else
0:	ldr		r0, [ip, #OMAP44XX_GPT_OFF_TCRR]	// counter high
	mov		r1, #0								// counter low	
.endif

	mov		pc, lr
CALLOUT_END(clock_cycles_omap4430)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/callout_clock_cycles_omap4430.S $ $Rev: 781278 $";
.previous
#endif
#endif 

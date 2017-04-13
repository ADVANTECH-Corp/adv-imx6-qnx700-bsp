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


/*
 * TI OMAP4430 general-purpose timer support
 */

#include "callout.ah"

#define OMAP44XX_GPTIMER1_BASE     0x4a318000
#define OMAP44XX_GPT_SIZE         0x100

#define OMAP44XX_GPT_OFF_TISR      0x18
#define OMAP44XX_GPT_OFF_TCLR      0x24
#define OMAP44XX_GPT_OFF_TCRR      0x28
#define OMAP44XX_GPT_OFF_TMAR      0x38

#define OMAP44XX_GPT_TCLR_CE       0x40
#define OMAP44XX_GPT_TISR_MATCH    0x01

/*
 * Data in RW storage
 */
timer_rw: .word 12
#define OFF_LAST_TMAR       0
#define OFF_TIMER_BASE      4
#define OFF_LOST_TICK_COUNT 8

#define MIN_TMAR_DELTA      2

/*
 * --------------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *    r0 - physical address of syspage
 *    r1 - virtual  address of syspage
 *    r2 - offset from start of syspage to start of the callout routine
 *    r3 - offset from start of syspage to read/write data used by callout
 * --------------------------------------------------------------------------
 */
patch_timer_omap4430:
    stmdb    sp!, {r4-r6, lr}
    add      r4, r0, r2               // address of callout routine
    add      r5, r0, r3               // addr of r/w data
    mov      r6, r3

    ldr      r0, Lvaddr
    cmp      r0, #-1
    bne      1f

  /*
   * Map registers
   */
    mov      r0, #OMAP44XX_GPT_SIZE   // size of registers
    adr      r1, Lpaddr
    bl       callout_io_map_indirect
    str      r0, Lvaddr
    str      r0, [r5, #OFF_TIMER_BASE]

1:
  /*
   * Patch the callout routine with the offset to the callout read/write data
   */
    CALLOUT_PATCH  r4, r6, r1, r2, ip

    ldmia    sp!, {r4-r6, pc}

		.align	3
Lpaddr: .quad	OMAP44XX_GPTIMER1_BASE
Lvaddr: .word	-1

/*
 * --------------------------------------------------------------------------
 * Set clock resolution, and enable interrupt triggering as a side effect.
 * The interrupt remains masked until it is enabled via intr_unmask_omap
 *
 * On entry:
 *    r0 - pointer to syspage_entry
 *    r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_load_omap4430, timer_rw, patch_timer_omap4430)
  /*
   * Get the address of the timer registers (patched)
   */
    mov      ip,     #0x000000ff
    orr      ip, ip, #0x0000ff00
    orr      ip, ip, #0x00ff0000
    orr      ip, ip, #0xff000000

  /*
   * Compute R/W area and get timer base
   */
    add      ip, r0, ip
    ldr      r2, [ip, #OFF_TIMER_BASE]

  /*
   * Turn off matching so that no interrupts will fire, clear match status
   */
    ldr      r0, [r2, #OMAP44XX_GPT_OFF_TCLR]
    bic      r0, r0,  #OMAP44XX_GPT_TCLR_CE
    str      r0, [r2, #OMAP44XX_GPT_OFF_TCLR]
    mov      r0, #OMAP44XX_GPT_TISR_MATCH
    str      r0, [r2, #OMAP44XX_GPT_OFF_TISR]

  /*
   * Time starts now on a load
   */
    ldr      r0, [r2, #OMAP44XX_GPT_OFF_TCRR]
    str      r0, [ip, #OFF_LAST_TMAR]

  /*
   * Interrupt goes off QT_TIMER_LOAD counts later
   */
    ldr      r1, [r1, #QT_TIMER_LOAD]
    cmp      r1, #MIN_TMAR_DELTA   // force at least two counts to be requested
    addlt    r1, r1, #MIN_TMAR_DELTA
    add      r1, r0, r1
    str      r1, [r2, #OMAP44XX_GPT_OFF_TMAR]

  /*
   * Enable matching, and enable timer if this is the first time
   */
    ldr      r0, [r2, #OMAP44XX_GPT_OFF_TCLR]
    orr      r0, r0,  #OMAP44XX_GPT_TCLR_CE
    str      r0, [r2, #OMAP44XX_GPT_OFF_TCLR]

    mov      pc, lr
CALLOUT_END(timer_load_omap4430)


/*
 * --------------------------------------------------------------------------
 * Read the current timer value, relative to the last clock tick
 *
 * On entry:
 *    r0 - pointer to syspage_entry
 *    r1 - pointer to qtime_entry
 *
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_value_omap4430, timer_rw, patch_timer_omap4430)
  /*
   * Get the address of the timer registers (patched)
   */
    mov      ip,     #0x000000ff
    orr      ip, ip, #0x0000ff00
    orr      ip, ip, #0x00ff0000
    orr      ip, ip, #0xff000000

  /*
   * Compute R/W area and get timer base
   */
    add      ip, r0, ip
    ldr      r2, [ip, #OFF_TIMER_BASE]

  /*
   * Return number of counts since the last tick.
   */
    ldr      r0, [r2, #OMAP44XX_GPT_OFF_TCRR]
    ldr      r1, [ip, #OFF_LAST_TMAR]
    sub      r0, r0, r1

    mov      pc, lr
CALLOUT_END(timer_value_omap4430)


/*
 * --------------------------------------------------------------------------
 * Clear timer interrupt.
 *
 * On entry:
 *    r0 - pointer to syspage_entry
 *    r1 - pointer to qtime_entry
 * --------------------------------------------------------------------------
 */
CALLOUT_START(timer_reload_omap4430, timer_rw, patch_timer_omap4430)
  /*
   * Get the address of the timer registers (patched)
   */
    mov      ip,     #0x000000ff
    orr      ip, ip, #0x0000ff00
    orr      ip, ip, #0x00ff0000
    orr      ip, ip, #0xff000000

  /*
   * Compute R/W area and get timer base
   */
    add      ip, r0, ip
    ldr      r2, [ip, #OFF_TIMER_BASE]

  /*
   * Turn off matching so that no interrupts will fire, clear match status
   */
    ldr      r0, [r2, #OMAP44XX_GPT_OFF_TCLR]
    bic      r0, r0,  #OMAP44XX_GPT_TCLR_CE
    str      r0, [r2, #OMAP44XX_GPT_OFF_TCLR]
    mov      r0,      #OMAP44XX_GPT_TISR_MATCH
    str      r0, [r2, #OMAP44XX_GPT_OFF_TISR]

  /*
   * Get the timestamp for the next tick. 
   */
    ldr      r3, [r1, #QT_TIMER_LOAD]
    ldr      r0, [r2, #OMAP44XX_GPT_OFF_TMAR]
    str      r0, [ip, #OFF_LAST_TMAR]
    add      r1, r0, r3
    str      r1, [r2, #OMAP44XX_GPT_OFF_TMAR]

  /*
   * r1 is when the timer should go off next.  Is it in the past?
   */
    ldr      r0, [r2, #OMAP44XX_GPT_OFF_TCRR]
    sub      r1, r1, r0
    sub      r1, r1, #MIN_TMAR_DELTA     // give 2 ticks of buffer for TMAR
    cmp      r1, #0
    bpl      1f

  /*
   * we've lost a tick
   */
    ldr      r1, [ip, #OFF_LOST_TICK_COUNT]
    add      r1, r1, #1
    str      r1, [ip, #OFF_LOST_TICK_COUNT]

  /*
   * reset next tick for current time plus TIMER_LOAD
   */
    str      r0, [ip, #OFF_LAST_TMAR]   // last tick is now
    add      r0, r0, r3
    str      r0, [r2, #OMAP44XX_GPT_OFF_TMAR]

1:

  /*
   * Enable matching
   */
    ldr      r0, [r2, #OMAP44XX_GPT_OFF_TCLR]
    orr      r0, r0,  #OMAP44XX_GPT_TCLR_CE
    str      r0, [r2, #OMAP44XX_GPT_OFF_TCLR]

    mov      pc, lr
CALLOUT_END(timer_reload_omap4430)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/callout_timer_omap4430.S $ $Rev: 781278 $";
.previous
#endif
#endif 

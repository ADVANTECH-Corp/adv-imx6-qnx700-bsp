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

/*
 * Freescale LPUART specific polled serial I/O
 * Note: CPU must be little-endian, break character detection is not supported
 */

#include "callout.ah"

#define LPUART_SIZE (32)

#define LPUART_STAT (0x04)
#define LPUART_DATA (0x0C)
#define LPUART_FIFO (0x18)

#define LPUART_STAT_TDRE   (1<<23)
#define LPUART_FIFO_RXEMPT (1<<22)

/*
 * -----------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *  r0 - physical address of syspage
 *  r1 - virtual  address of syspage
 *  r2 - offset from start of syspage to start of the callout routine
 *  r3 - offset from start of syspage to read/write data used by callout
 *  [sp]   - data registered with callout via callout_register_data()
 *  [sp+4] - address of callout definition (CALLOUT_START)
 * -----------------------------------------------------------------------
 */
patch_debug:
    stmdb   sp!, {r4, lr}
    add     r4, r0, r2                  /* address of callout routine */

    /* Map registers */
    ldr     r1, [sp, #8]
    add     r1, r1, #DDI_BASE
    mov     r0, #LPUART_SIZE
    bl      callout_io_map_indirect

    /* Patch the callout routine */
    CALLOUT_PATCH   r4, r0, r1, r2, ip
    ldmia   sp!, {r4, pc}

/*
 * -----------------------------------------------------------------------
 * void display_char_lpuart_le(struct sypage_entry *, char)
 * -----------------------------------------------------------------------
 */
CALLOUT_START(display_char_lpuart_le, 0, patch_debug)
    /* Register base address (patched) */
    mov     ip,     #0x000000ff
    orr     ip, ip, #0x0000ff00
    orr     ip, ip, #0x00ff0000
    orr     ip, ip, #0xff000000

0:
    ldr     r3, [ip, #LPUART_STAT]
    tst     r3, #LPUART_STAT_TDRE
    beq     0b

    str     r1, [ip, #LPUART_DATA]
    mov     pc, lr
CALLOUT_END(display_char_lpuart_le)

/*
 * -----------------------------------------------------------------------
 * void display_char_lpuart_be(struct sypage_entry *, char)
 * -----------------------------------------------------------------------
 */
CALLOUT_START(display_char_lpuart_be, 0, patch_debug)
    /* Register base address (patched) */
    mov     ip,     #0x000000ff
    orr     ip, ip, #0x0000ff00
    orr     ip, ip, #0x00ff0000
    orr     ip, ip, #0xff000000

0:
    ldr     r3, [ip, #LPUART_STAT]
    rev     r3, r3
    tst     r3, #LPUART_STAT_TDRE
    beq     0b

    rev     r1, r1
    str     r1, [ip, #LPUART_DATA]
    mov     pc, lr
CALLOUT_END(display_char_lpuart_be)


/*
 * -----------------------------------------------------------------------
 * char poll_key_lpuart_le(struct sypage_entry *)
 * -----------------------------------------------------------------------
 */
CALLOUT_START(poll_key_lpuart_le, 0, patch_debug)
    /* Register base address (patched) */
    mov     ip,     #0x000000ff
    orr     ip, ip, #0x0000ff00
    orr     ip, ip, #0x00ff0000
    orr     ip, ip, #0xff000000

    /* Is there something in the data register? */
    mvn     r0, #1
    ldr     r3, [ip, #LPUART_FIFO]
    tst     r3, #LPUART_FIFO_RXEMPT
    movne   pc, lr

    /* Load data */
    ldr     r3, [ip, #LPUART_DATA]
    and     r0, r3, #0xFF

    mov     pc, lr
CALLOUT_END(poll_key_lpuart_le)

/*
 * -----------------------------------------------------------------------
 * char poll_key_lpuart_be(struct sypage_entry *)
 * -----------------------------------------------------------------------
 */
CALLOUT_START(poll_key_lpuart_be, 0, patch_debug)
    /* Register base address (patched) */
    mov     ip,     #0x000000ff
    orr     ip, ip, #0x0000ff00
    orr     ip, ip, #0x00ff0000
    orr     ip, ip, #0xff000000

    /* Is there something in the data register? */
    mvn     r0, #1
    ldr     r3, [ip, #LPUART_FIFO]
    rev     r3, r3
    tst     r3, #LPUART_FIFO_RXEMPT
    movne   pc, lr

    /* Load data */
    ldr     r3, [ip, #LPUART_DATA]
    rev     r3, r3
    and     r0, r3, #0xFF

    mov     pc, lr
CALLOUT_END(poll_key_lpuart_be)

/*
 * -----------------------------------------------------------------------
 * int break_detect_lpuart_le(struct syspage_entry *)
 *
 * Return 1 if break detected, 0 otherwise
 * -----------------------------------------------------------------------
 */
CALLOUT_START(break_detect_lpuart_le, 0, patch_debug)
    /* Register base address (patched) */
    mov     ip,     #0x000000ff
    orr     ip, ip, #0x0000ff00
    orr     ip, ip, #0x00ff0000
    orr     ip, ip, #0xff000000

    /* break detect is not supported in this driver */
    mov     r0, #0
    mov     pc, lr
CALLOUT_END(break_detect_lpuart_le)

/*
 * -----------------------------------------------------------------------
 * int break_detect_lpuart_be(struct syspage_entry *)
 *
 * Return 1 if break detected, 0 otherwise
 * -----------------------------------------------------------------------
 */
CALLOUT_START(break_detect_lpuart_be, 0, patch_debug)
    /* Register base address (patched) */
    mov     ip,     #0x000000ff
    orr     ip, ip, #0x0000ff00
    orr     ip, ip, #0x00ff0000
    orr     ip, ip, #0xff000000

    /* break detect is not supported in this driver */
    mov     r0, #0
    mov     pc, lr
CALLOUT_END(break_detect_lpuart_be)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/callout_debug_lpuart.S $ $Rev: 781278 $";
.previous
#endif
#endif

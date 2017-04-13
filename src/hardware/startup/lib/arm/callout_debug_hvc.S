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
 * hvc polled serial I/O
 */

#include "callout.ah"

#define HVC			0x20
#define CONSOLE		18
#define PUTC		2
#define GETC		3
#define HVC_IMM		0x764d

/*
 * -----------------------------------------------------------------------
 * void	display_char_hvc(struct sypage_entry *, char)
 * -----------------------------------------------------------------------
 */
CALLOUT_START(display_char_hvc, 0, 0)
.Lretry:
	mov		r0,#PUTC
	mov		r12,#CONSOLE
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
	.arch_extension virt
	hvc		#HVC_IMM
#else
	.word	0xe147647d	// hvc #HVC_IMM
#endif
    cmp     r0,#0
    bne     .Lretry
	mov		pc, lr
CALLOUT_END(display_char_hvc)


/*
 * -----------------------------------------------------------------------
 * char	poll_key_hvc(struct sypage_entry *)
 * -----------------------------------------------------------------------
 */
CALLOUT_START(poll_key_hvc, 0, 0)
	mov		r0,#GETC
	mov		r12,#CONSOLE
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
	.arch_extension virt
	hvc		#HVC_IMM
#else
	.word	0xe147647d	// hvc #HVC_IMM
#endif
	mov		pc, lr
CALLOUT_END(poll_key_hvc)


/*
 * -----------------------------------------------------------------------
 * int break_detect(struct syspage_entry *)
 *
 * Return 1 if break detected, 0 otherwise
 * -----------------------------------------------------------------------
 */
CALLOUT_START(break_detect_hvc, 0, 0)
	mov	r0, #0
	mov pc, lr
CALLOUT_END(break_detect_hvc)


#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/callout_debug_hvc.S $ $Rev: 774853 $";
.previous
#endif
#endif 

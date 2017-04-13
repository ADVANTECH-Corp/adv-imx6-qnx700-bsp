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
#include "startup.h"

void
init_hvc(unsigned channel, const char *init, const char *defaults) {
	//Nothing happening
}

void
put_hvc(int c) {
    asm volatile (
		"mov	r1,%0\n"
		"mov	r0,#2\n"	// PUTC
		"mov	ip,#18\n"	// CONSOLE
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)
		".arch_extension virt\n"
		"hvc	#0x764d\n"
#else
		".word	0xe147647d\n"	// hvc #HVC_IMM
#endif
		: : "r" (c));
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/hw_hvc.c $ $Rev: 774853 $")
#endif

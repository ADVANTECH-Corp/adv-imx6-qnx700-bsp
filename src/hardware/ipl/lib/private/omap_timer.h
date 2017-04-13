/*
 * $QNXLicenseC:
 * Copyright 2013, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0

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

#ifndef	__OMAP_TIMER_H
#define __OMAP_TIMER_H

#define TIMING_MICRO_SECOND	1	/* Timing unit in miscro second */
#define TIMING_MILLI_SECOND	0

extern void omap_timer_enable(unsigned long, unsigned long);
extern void omap_timer_disable(void);
extern void omap_timer_start(void);
extern void omap_timer_curr(const char *, int);
extern void omap_usec_delay(unsigned); 	// micro seconds
extern void omap_nano_delay(unsigned);	// nano seconds, minimum 50ns

#endif /* __OMAP_TIMER_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/private/omap_timer.h $ $Rev: 723412 $")
#endif

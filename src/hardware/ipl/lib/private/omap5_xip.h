/*
 * $QNXLicenseC:
 * Copyright 2013, QNX Software Systems.
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

#ifndef _OMAP5_XIP_INCLUDED
#define _OMAP5_XIP_INCLUDED

typedef struct omap5_xip_dev {
	unsigned dma_pbase;		/* DMA base address */
	unsigned dma_chnl;		/* DMA channel */
	unsigned dma_bw;		/* Data bus width */
	unsigned granularity;	/* Number of words in one EDMA transfer */
	void (*dcache_flush)();
} omap5_xip_dev_t;

extern unsigned omap5_load_xip(void *ext, unsigned long dst, unsigned long src, int size, int verbose);

#endif /* #ifndef _OMAP5_XIP_INCLUDED */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/private/omap5_xip.h $ $Rev: 723846 $")
#endif

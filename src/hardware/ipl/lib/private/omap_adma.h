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

#ifndef OMAP_ADMA_INCLUDED
#define OMAP_ADMA_INCLUDED

#define MAX_BLKCNT	(32 * 1024)

// The maximum buffer which the host controller can handle is 32MB - 1
// We use 32KB chunk for each descriptor, allocate maximum descriptors we might need
#define ADMA_CHUNK_SIZE (32 * 1024)

// 32 bit ADMA descriptor definition
typedef struct _omap_adma32_t {
	unsigned short	attr; 
	unsigned short	len;
	unsigned		addr;
} omap_adma32_t;

#define ADMA2_VALID (1 << 0)	// valid
#define ADMA2_END	(1 << 1)	// end of descriptor, transfer complete interrupt will be generated
#define ADMA2_INT	(1 << 2)	// generate DMA interrupt, will not be used
#define ADMA2_NOP	(0 << 4)	// no OP, go to the next desctiptor
#define ADMA2_TRAN	(2 << 4)	// transfer data
#define ADMA2_LINK	(3 << 4)	// link to another descriptor

#endif /* #ifndef OMAP_ADMA_INCLUDED */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/private/omap_adma.h $ $Rev: 730393 $")
#endif

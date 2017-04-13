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

#ifndef _OMAP3_EDMA_INCLUDED
#define _OMAP3_EDMA_INCLUDED

#include <hw/inout.h>

#define	OMAP3_SHADOW0_OFF	0x2000
#define	OMAP3_PARAM_OFF		0x4000
#define	OMAP3_PARAM_SIZE	0x20
#define	EDMA_PARAM(ch)		(OMAP3_PARAM_OFF + (OMAP3_PARAM_SIZE * ch))

/*
 * EDMA registers, offset from EDMA register base
 */
#define	OMAP3_EDMA_ER		0x0000		/* ER Event Register */
#define	OMAP3_EDMA_ERH		0x0004		/* ER Event Register High */
#define	OMAP3_EDMA_ECR		0x0008		/* ER Event Clear Register */
#define	OMAP3_EDMA_ECRH		0x000C		/* ER Event Clear Register High */
#define	OMAP3_EDMA_ESR		0x0010		/* ER Event Set Register */
#define	OMAP3_EDMA_ESRH		0x0014		/* ER Event Set Register High */
#define	OMAP3_EDMA_EER		0x0020		/* ER Event Enable Register */
#define	OMAP3_EDMA_EERH		0x0024		/* ER Event Enable Register High */
#define	OMAP3_EDMA_EECR		0x0028		/* ER Event Enable Clear Register */
#define	OMAP3_EDMA_EECRH	0x002C		/* ER Event Enable Clear Register High */
#define	OMAP3_EDMA_EESR		0x0030		/* ER Event Enable Set Register */
#define	OMAP3_EDMA_EESRH	0x0034		/* ER Event Enable Set Register High */
#define	OMAP3_EDMA_IPR		0x0068		/* Interrupt Pending Register */
#define	OMAP3_EDMA_IPRH		0x006C		/* Interrupt Pending Register High */
#define	OMAP3_EDMA_ICR		0x0070		/* Interrupt Clear Register */
#define	OMAP3_EDMA_ICRH		0x0074		/* Interrupt Clear Register High */

typedef struct {
	volatile unsigned	opt;
	volatile unsigned	src;
	volatile unsigned	abcnt;
	volatile unsigned	dst;
	volatile unsigned	srcdstbidx;
	volatile unsigned	linkbcntrld;
	volatile unsigned	srcdstcidx;
	volatile unsigned	ccnt;
} edma_param;

static inline void omap3_edma_set_bit(unsigned base, unsigned reg, int channel)
{
    base += OMAP3_SHADOW0_OFF;

    if (channel > 31) {
        reg += 4;
        channel -= 32;
    }

    out32(base + reg, 1 << channel);
}

static inline unsigned omap3_edma_read_bit(unsigned base, unsigned reg, int channel)
{
    base += OMAP3_SHADOW0_OFF;

    if (channel > 31) {
        reg += 4;
        channel -= 32;
    }

    return (in32(base + reg) & (1 << channel));
}

#endif /* #ifndef _OMAP3_EDMA_INCLUDED */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/private/omap3_edma.h $ $Rev: 745292 $")
#endif

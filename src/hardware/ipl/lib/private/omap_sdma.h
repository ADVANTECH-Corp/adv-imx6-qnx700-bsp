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

#ifndef OMAP_SDMA_INCLUDED
#define OMAP_SDMA_INCLUDED

#define DMA4_IRQSTATUS(j)			(0x08 + (j) * 4)	// j = 0 - 3
#define DMA4_IRQENABLE(j)			(0x18 + (j) * 4)	// j = 0 - 3
#define DMA4_SYSSTATUS				(0x28)
#define DMA4_OCP_SYSCONFIG			(0x2C)
#define DMA4_CAPS_0					(0x64)
#define DMA4_CAPS_2					(0x6C)
#define DMA4_CAPS_3					(0x70)
#define DMA4_CAPS_4					(0x74)
#define DMA4_GCR					(0x78)
#define DMA4_CCR(i)					(0x80 + (i) * 0x60) // i = 0 - 31
#define DMA4_CLNK_CTRL(i)			(0x84 + (i) * 0x60)
#define DMA4_CICR(i)				(0x88 + (i) * 0x60)
#define DMA4_CSR(i)					(0x8C + (i) * 0x60)
#define DMA4_CSDP(i)				(0x90 + (i) * 0x60)
#define DMA4_CEN(i)					(0x94 + (i) * 0x60)
#define DMA4_CFN(i)					(0x98 + (i) * 0x60)
#define DMA4_CSSA(i)				(0x9C + (i) * 0x60)
#define DMA4_CDSA(i)				(0xA0 + (i) * 0x60)
#define DMA4_CSE(i)					(0xA4 + (i) * 0x60)
#define DMA4_CSF(i)					(0xA8 + (i) * 0x60)
#define DMA4_CDE(i)					(0xAC + (i) * 0x60)
#define DMA4_CDF(i)					(0xB0 + (i) * 0x60)
#define DMA4_CSAC(i)				(0xB4 + (i) * 0x60)
#define DMA4_CDAC(i)				(0xB8 + (i) * 0x60)
#define DMA4_CCEN(i)				(0xBC + (i) * 0x60)
#define DMA4_CCFN(i)				(0xC0 + (i) * 0x60)
#define DMA4_COLOR(i)				(0xC4 + (i) * 0x60)

#define DMA4_GPMC					4
#define DMA4_MMC3_RX				78
#define DMA4_MMC1_TX				61
#define DMA4_MMC1_RX				62
#define DMA4_MMC2_TX				47
#define DMA4_MMC2_RX				48
#define DMA4_MMC3_TX				77
#define DMA4_MMC3_RX				78

#define DMA4_CCR_SYNCHRO_CONTROL(s)	 (((s) & 0x1F) | (((s) >> 5) << 19))

typedef struct {
	volatile unsigned	ccr;		// 0x00
	volatile unsigned	clnk_ctrl;	// 0x04
	volatile unsigned	cicr;		// 0x08
#define DMA4_CSR_EVT_ALL		(0x5FFE)
#define DMA4_CSR_EVT_EOF		(1 << 3)	// End of frame event
	volatile unsigned	csr;		// 0x0C
	volatile unsigned	csdp;		// 0x10
	volatile unsigned	cen;		// 0x14
	volatile unsigned	cfn;		// 0x18
	volatile unsigned	cssa;		// 0x1C
	volatile unsigned	cdsa;		// 0x20
	volatile unsigned	cse;		// 0x24
	volatile unsigned	csf;		// 0x28
	volatile unsigned	cde;		// 0x2C
	volatile unsigned	cdf;		// 0x30
	volatile unsigned	csac;		// 0x34
	volatile unsigned	cdac;		// 0x38
	volatile unsigned	ccen;		// 0x3C
	volatile unsigned	ccfn;		// 0x40
	volatile unsigned	color;		// 0x44
} dma4_param;

#endif /* #ifndef OMAP_SDMA_INCLUDED */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/private/omap_sdma.h $ $Rev: 730393 $")
#endif

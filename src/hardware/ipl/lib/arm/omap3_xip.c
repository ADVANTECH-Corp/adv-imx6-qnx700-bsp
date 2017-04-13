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

#include "ipl.h"
#include <hw/inout.h>
#include <stdlib.h>
#include <omap3_xip.h>

static void omap3_edma_done(omap3_xip_dev_t *omap3_dev, edma_param *param)
{
	int chnl = omap3_dev->dma_chnl;
	unsigned base = omap3_dev->dma_pbase + OMAP3_SHADOW0_OFF;

	if (chnl > 31) {
		base += 4;
		chnl -= 32;
	}

	/* Wait for the end of the transfer */
	while (!omap3_edma_read_bit(omap3_dev->dma_pbase, OMAP3_EDMA_IPR, omap3_dev->dma_chnl));

	/* Clear IPR bit */
	omap3_edma_set_bit(omap3_dev->dma_pbase, OMAP3_EDMA_ICR, omap3_dev->dma_chnl);

	/* Disable this EDMA event */
	omap3_edma_set_bit(omap3_dev->dma_pbase, OMAP3_EDMA_EECR, omap3_dev->dma_chnl);
}

static void omap3_setup_dma_read(omap3_xip_dev_t *omap3_dev, edma_param *param, unsigned dst, unsigned src, unsigned size)
{
	int chnl = omap3_dev->dma_chnl;

	/* In case there is a pending event */
	omap3_edma_set_bit(omap3_dev->dma_pbase, OMAP3_EDMA_ECR, omap3_dev->dma_chnl);

	/* Initialize Rx DMA channel */
	param->opt = (0 << 23)		/* ITCCHEN = 0 */
				| (0 << 22)		/* TCCHEN = 0 */
				| (0 << 21)		/* ITCINTEN = 0 */
				| (1 << 20)		/* TCINTEN = 1 */
				| (chnl << 12)	/* TCC */
				| (0 << 11)		/* Normal completion */
				| (2 << 8)		/* FIFO width: 32-bit */
				| (1 << 3)		/* PaRAM set is static */
				| (1 << 2)		/* AB-synchronizad */
				| (0 << 1)		/* Destination address increment mode */
				| (0 << 0);		/* Source address increment mode */

	param->src			= src;

	/* We assume omap3_dev->dma_bw <= 4 */
	param->abcnt		= ((size >> (omap3_dev->dma_bw >> 1)) << 16) | omap3_dev->dma_bw;
	param->dst			= dst;
	param->srcdstbidx	= (omap3_dev->dma_bw << 16) | omap3_dev->dma_bw;
	param->linkbcntrld	= 0xFFFF;
	param->srcdstcidx	= 0;
	param->ccnt		 = 1;

	/* Enable event */
	omap3_edma_set_bit(omap3_dev->dma_pbase, OMAP3_EDMA_EESR, omap3_dev->dma_chnl);
}

unsigned omap3_load_xip(void *ext, unsigned long dst, unsigned long src, int size, int verbose)
{
	edma_param	*param;
	omap3_xip_dev_t *omap3_dev = (omap3_xip_dev_t *)ext;

	unsigned checksum = 0;
	unsigned loading_addr = dst;
	int cnt = 0;
	int offset = 0;
	int rdy_len = 0;

	if (omap3_dev->dcache_flush)
		omap3_dev->dcache_flush();

	if (verbose) {
		ser_putstr("Loading image @ XIP device from 0x");
		ser_puthex(src);
		ser_putstr(" to 0x");
		ser_puthex(dst);
		ser_putstr(" size 0x");
		ser_puthex(size);
		ser_putstr("\n");
	}

	/* 
	 * Reassure that the size is integral multiple of the bus width to make EDMA setup easier 
	 * It's really doesn't matter if we load few extra bytes
	 */
	size = (size + omap3_dev->dma_bw - 1) & ~(omap3_dev->dma_bw - 1);

	/* Initialize Rx DMA channel */
	param = (edma_param *)(omap3_dev->dma_pbase + EDMA_PARAM(omap3_dev->dma_chnl));

	while (size) {
		/* EDMA's ACNT/BCNT register has restriction on maximum number of transfered bytes */
		int len = min(size, omap3_dev->granularity * omap3_dev->dma_bw);

		/* setup Rx DMA channel for data read */
		omap3_setup_dma_read(omap3_dev, param, dst, src, len);

		/* Manually enable EDMA transfer */
		omap3_edma_set_bit(omap3_dev->dma_pbase, OMAP3_EDMA_ESR, omap3_dev->dma_chnl);

		if (!cnt) {
			cnt = 1;
			omap3_edma_done(omap3_dev, param);
			offset = 0;
			rdy_len = len;
		} else {
			/*
 			 * By calculating checksum while waiting for EDMA events,
			 * it can significantly improve the loading performance
			 */
			checksum += checksum_2(loading_addr + offset, rdy_len);

			omap3_edma_done(omap3_dev, param);
			offset += rdy_len;
			rdy_len = len;
		}

		size -= len;
		src += len;
		dst += len;
	}

	checksum += checksum_2(loading_addr + offset, rdy_len);
	return checksum;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/arm/omap3_xip.c $ $Rev: 798438 $")
#endif

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
#include <omap5_xip.h>
#include <omap_sdma.h>

static void dma4_xfer_done(dma4_param *param)
{
	/* Wait for the end of the transfer */
	while (!(param->csr & DMA4_CSR_EVT_EOF));

	/* Clear End of Frame bit */
	param->csr = DMA4_CSR_EVT_EOF;
}

static void dma4_setup_xfer(omap5_xip_dev_t *omap5_dev, dma4_param *param, unsigned dst, unsigned src, unsigned size)
{
	/* Clear all status bits */
	param->csr	= DMA4_CSR_EVT_ALL;
	param->cen	= (size / omap5_dev->dma_bw);	/* BITS[0:24] - Number of elements within a frame */
	param->cfn	= 1;			/* Number of frames */
	param->cse	= 1;			/* Channel Source Elements Index */
	param->cde	= 1;			/* Channel Destination Element Index */
	param->cicr = DMA4_CSR_EVT_EOF;	/* End of frame interrupt */

	/* setup receive SDMA channel */
	param->csdp = (2 <<	0)	 // DATA_TYPE = 0x2:	32 bit element
				| (0 <<	2)	 // RD_ADD_TRSLT = 0: Not used
				| (0 <<	6)	 // SRC_PACKED = 0x0: No packing
				| (3 <<	7)	 // DST_BURST_EN = 0x3: Burst at 16x32 bits
				| (0 <<	9)	 // WR_ADD_TRSLT = 0: Undefined
				| (0 << 13)	 // DST_PACKED = 0x0: No packing
				| (3 << 14)	 // DST_BURST_EN = 0x3: Burst at 16x32 bits
				| (1 << 16)	 // WRITE_MODE = 0x1: Write posted, actually it doesn't matter
				| (0 << 18)	 // DST_ENDIAN_LOCK = 0x0: Endianness adapt
				| (0 << 19)	 // DST_ENDIAN = 0x0: Little Endian type at destination
				| (0 << 20)	 // SRC_ENDIAN_LOCK = 0x0: Endianness adapt
				| (0 << 21);	// SRC_ENDIAN = 0x0: Little endian type at source

	param->ccr	= DMA4_CCR_SYNCHRO_CONTROL(0) // Software triggered transfer
				| (1 <<	5)	 // FS = 1: Packet mode with BS = 0x1
				| (0 <<	6)	 // READ_PRIORITY = 0x0: Low priority on read side
				| (0 <<	7)	 // ENABLE = 0x0: The logical channel is disabled.
				| (0 <<	8)	 // SUSPEND_SENSITIVE
				| (1 << 12)	 // SRC_AMODE: Post-incremented address mode
				| (1 << 14)	 // DST_AMODE: Post-incremented address mode
				| (1 << 18)	 // BS: Packet mode with FS = 0x1
				| (1 << 23)	 // PREFETC
				| (0 << 24)	 // SEL_SRC_DST_SYNC: Transfer is triggered by the source. This bit doesn't matter
				| (1 << 25)		// BUFFERING_DISABLE
				| (0 << 26);	// WRITE_PRIORITY = 0x1

	param->cssa = src;
	param->cdsa = dst;

	/* Enable DMA transfer */
	param->ccr |= (1 << 7);
}

unsigned omap5_load_xip(void *ext, unsigned long dst, unsigned long src, int size, int verbose)
{
	dma4_param *param;
	omap5_xip_dev_t *omap5_dev = (omap5_xip_dev_t *)ext;

	unsigned checksum = 0;
	unsigned loading_addr = dst;
	int cnt = 0;
	int offset = 0;
	int rdy_len = 0;

	if (omap5_dev->dcache_flush)
		omap5_dev->dcache_flush();

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
	size = (size + omap5_dev->dma_bw - 1) & ~(omap5_dev->dma_bw - 1);

	/* Initialize Rx DMA channel */
	param = (dma4_param *)(omap5_dev->dma_pbase + DMA4_CCR(omap5_dev->dma_chnl));

	while (size) {
		int len = min(size, omap5_dev->granularity * omap5_dev->dma_bw);

		/* setup Rx DMA channel for data read */
		dma4_setup_xfer(omap5_dev, param, dst, src, len);

		if (!cnt) {
			cnt = 1;
			dma4_xfer_done(param);
			offset = 0;
			rdy_len = len;
		} else {
			/*
 			 * By calculating checksum while waiting for SDMA events,
			 * it can significantly improve the loading performance
			 */
			checksum += checksum_2(loading_addr + offset, rdy_len);
			dma4_xfer_done(param);
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
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/arm/omap5_xip.c $ $Rev: 798438 $")
#endif

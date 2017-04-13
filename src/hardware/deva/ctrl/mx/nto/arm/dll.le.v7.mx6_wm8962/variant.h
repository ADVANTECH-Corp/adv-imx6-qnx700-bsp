/*
 * $QNXLicenseC:
 * Copyright 2014, QNX Software Systems.
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

#ifndef VARIANT_H_INCLUDED
#define VARIANT_H_INCLUDED

#define VARIANT_MX6X

#define DEFAULT_SSI         2
#define DEFAULT_CLK_MODE    CLK_MODE_SLAVE
#define SSI_CLK             16500000
#define SAMPLE_SIZE         2
#define SAMPLE_RATE_MIN     48000
#define SAMPLE_RATE_MAX     48000

#define SSI1_BASE_ADDR      0x02028000
#define SSI2_BASE_ADDR      0x0202C000
#define SSI3_BASE_ADDR      0x02030000

#define SSI1_IRQ            78
#define SSI2_IRQ            79
#define SSI3_IRQ            80

#define SSI1_TX_DMA_EVENT   38
#define SSI1_RX_DMA_EVENT   37
#define SSI2_TX_DMA_EVENT   42
#define SSI2_RX_DMA_EVENT   41
#define SSI3_TX_DMA_EVENT   46
#define SSI3_RX_DMA_EVENT   45

#define SSI1_TX_DMA_CTYPE   9	/* SDMA_CHTYPE_MCU_2_SSISH from private header sdma.h */
#define SSI1_RX_DMA_CTYPE   10	/* SDMA_CHTYPE_SSISH_2_MCU from private header sdma.h */
#define SSI2_TX_DMA_CTYPE   9	/* SDMA_CHTYPE_MCU_2_SSISH from private header sdma.h */
#define SSI2_RX_DMA_CTYPE   10	/* SDMA_CHTYPE_SSISH_2_MCU from private header sdma.h */
#define SSI3_TX_DMA_CTYPE   9	/* SDMA_CHTYPE_MCU_2_SSISH from private header sdma.h */
#define SSI3_RX_DMA_CTYPE   10	/* SDMA_CHTYPE_SSISH_2_MCU from private header sdma.h */

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/deva/ctrl/mx/nto/arm/dll.le.v7.mx6_wm8962/variant.h $ $Rev: 796343 $")
#endif

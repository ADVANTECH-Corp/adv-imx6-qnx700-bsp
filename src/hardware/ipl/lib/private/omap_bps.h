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


/*
 * TI J5/J5ECO Boot Parameters Structure
 */

#ifndef	__J5_BPS_H_INCLUDED_
#define __J5_BPS_H_INCLUDED_

typedef struct {
	unsigned int rsv1;
	unsigned int mbdda;	 	/* Memory booting device descriptor address */
	unsigned char boot_dev; /* Current boot device */
	unsigned char reset_reason;
	unsigned char rsv2;
	unsigned char rsv3;
} __attribute__((__packed__)) j5_bps_t;

#define BOOT_VOID				0x00
#define BOOT_XIP				0x01
#define BOOT_WAIT_XIP			0x02
#define BOOT_NAND				0x05
#define BOOT_NAND_I2C			0x06
#define BOOT_SD1				0x09
#define BOOT_SD2				0x0A
#define BOOT_SPI				0x15
#define BOOT_UART0				0x41
#define BOOT_USB				0x44
#define BOOT_CPGMAC0			0x46		

#define RST_GLOBAL_COLD			(1 << 0)
#define RST_GLOBAL_WARM_SW		(1 << 1)
#define RST_MPU_SECURITY_VIOL	(1 << 2)
#define RST_MPU_WDT				(1 << 3)
#define RST_SECURE_WDT			(1 << 4)
#define RST_EXTERNAL_WARM		(1 << 5)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/private/omap_bps.h $ $Rev: 723412 $")
#endif

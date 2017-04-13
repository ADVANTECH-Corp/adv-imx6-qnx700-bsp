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



#ifndef _VARIANT_H
#define _VARIANT_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#include <hw/spi-master.h>

/* weilun@adv - begin */

//#define CONFIG_MACH_MX6Q_RSB_4410
//#define CONFIG_MACH_MX6Q_ROM_5420
//#define CONFIG_MACH_MX6Q_ROM_7420
//#define CONFIG_MACH_MX6Q_DMS_BA16
/* weilun@adv - end */

/* weilun@adv - begin */
#if 0
#define DEVICE_IDENT		"M25P32-VMW6TG Serial NOR Flash"
#else
#define DEVICE_IDENT		"M25P32-N25Q Serial NOR Flash"
#endif
/* weilun@adv - end */


#define CLOCK_RATE		60000000	// frequency of ECSPI reference clock

/*
 * M25P32-VMW6TG Flash chip on sabre smart: 64 sectors, each containing 256 pages.
 * Each page is 256 bytes wide.
 */
#define PAGE_SIZE		256		// bytes
#define SECTOR_SIZE		256		// number of pages in one sector
#define TOTAL_SIZE		64		// sectors
#define ADDR_LEN		3
#define CMD_LEN			1		// one byte commands

/* According to the i.MX6x Data Sheet (IMX6AEC) the minimum SCLK Cycle time for a read is 30ns,
 * therefore the max acceptable SCLK frequency is 33.333333MHz.
 */
#define SCLK_FREQ		20000000 // On sabresmart we have to run at 20Mhz, otherwise it's unstable.
#define SPI_MODE		0
/* weilun@adv - begin */
#if 0
#define DEVICE_ID		0x20
#else
#define DEVICE_ID		0xba
#endif
/* weilun@adv - end */
#define MAN_ID			0x20
#define NUM_ERASE_UNITS		TOTAL_SIZE   // Number of sectors, this is the erase unit
#define PWR2_SIZE_UNIT		16      // Size of one sectors: 4K bytes

/* weilun@adv - begin */
/* rsb4410 SPI_DEV = 0, rom5420 SPI_DEV = 0, rom7420 SPI_DEV = 1, dmsba16 SPI_DEV = 0 */
/* ROM-7420 */
#if defined(CONFIG_MACH_MX6Q_ROM_7420)
#define SPI_DEV			1 	    // Slave Select 1
#else
#define SPI_DEV			0	    // Slave Select 0
#endif
/* weilun@adv - end */


#define MX_ECSPI_BURST_MAX	0x100
#define MAX_BURST		MX_ECSPI_BURST_MAX   // the maximum number of bytes for a SPI data exchange

#define MX_ECSPI_BASE		0x02008000
#define MX_ECSPI_SIZE		0x4000
#define MX_ECSPI_IRQ		63

#endif

//each returns EOK if success, -1 on error, with errno set
int spi_cmd_read(int fd, uint8_t *cmd, uint8_t* buf, int len);
int spi_cmd_write(int fd, uint8_t *cmd, uint8_t const* buf, int len);
int spi_nor_flash_write(int fd, uint8_t const* buf, int len);
int spi_write_1byte(int fd, uint8_t cmd);
int spi_word_exchange(int fd, uint32_t* buf);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/flash/boards/norspi/ecspi/arm/le.v7.mx6_sabresmart/variant.h $ $Rev: 796498 $")
#endif

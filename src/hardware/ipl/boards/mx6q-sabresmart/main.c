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

#include "ipl.h"
#include "ipl_mx6x.h"
#include <hw/inout.h>
#include "private/fat-fs.h"
#include <private/mx_sdhc.h>
#include <sys/srcversion.h>
#include <arm/mx6x_iomux.h>

/*
 * The buffer used by fat-fs.c as the common buffer,
 */
static unsigned char fat_buf2[FAT_COMMON_BUF_SIZE];
/*
 * The buffer used by fat-fs.c as the FAT info
 */
static unsigned char fat_buf1[FAT_FS_INFO_BUF_SIZE];

/*
 *  The address into which you load the IFS image MUST NOT
 *  overlap the area it will be run from.  The image will
 *  be placed into the correct location by 'image_setup'.
 *
 *  The image is configured to be uncompressed to, and
 *  run from, 0x10800000.  The compressed image is loaded
 *  higher up, and we must leave a big enough gap so they
 *  do not overlap when being decompressed.
 *  Loading the image to 0x18000000 leaves a 120MB gap.
 */
#define QNX_LOAD_ADDR	0x18000000

extern  fs_info_t       fs_info;

extern	void init_aips();
extern	void init_clocks();
extern	void init_pinmux();
extern	void enableTZASC(unsigned tzasc_base_addr);


void delay(unsigned dly)
{
	volatile int j;

	while (dly--) {
		for (j = 0; j < 32; j++)
			;
	}
}

static inline int
sdmmc_load_file (unsigned address, const char *fn)
{
	sdmmc_t	sdmmc;
	int	status;

	/* initialize the sdmmc interface and card */
	mx_sdmmc_init_hc(&sdmmc, MX6X_USDHC3_BASE, MX_SDMMC_CLK_DEFAULT, SDMMC_VERBOSE_LVL_0);

	if (sdmmc_init_sd(&sdmmc)) {
		ser_putstr("SDMMC card init failed\n");
		status = SDMMC_ERROR;
		goto done;
	}

	fat_sdmmc_t fat = {
		.ext = &sdmmc,
		.buf1 = fat_buf1,
		.buf1_len = FAT_FS_INFO_BUF_SIZE,
		.buf2 = fat_buf2,
		.buf2_len = FAT_COMMON_BUF_SIZE,
		.verbose = 0
	};

	if (fat_init(&fat)) {
		ser_putstr("Failed to init fat-fs\n");
		status = SDMMC_ERROR;
		goto done;
	}

	status = fat_copy_named_file((unsigned char *)address, (char *)fn);

done:
	sdmmc_fini(&sdmmc);

	return status;
}

int main()
{
	unsigned image = QNX_LOAD_ADDR;
	char c = 'M'; /* default: load ifs from SD card */

	/* Allow access to the AIPS registers */
	init_aips();

	/* Initialise the system clocks */
	init_clocks();

	init_pinmux();

	/* Init serial interface */
	/* 115200bps, 80MHz clk, divisor (RFDIV 1-7) 2 */
	init_sermx6(MX6X_UART1_BASE, 115200, 80000000, 2);

#if defined (VARIANT_enableTZASC)
	enableTZASC(MX6X_TZASC1);
#endif

	ser_putstr("\nQNX Neutrino Initial Program Loader for the NXP i.MX6Q Sabre-Smart (ARM Cortex-A9 MPCore)\n");


	ser_putstr("ARM TrustZone ASC is ");
#if defined (VARIANT_enableTZASC)
	ser_putstr("enabled");
#else
	ser_putstr("disabled");
#endif
	ser_putstr("\nIPL compiled ");
	ser_putstr(__DATE__);
	ser_putstr(" ");
	ser_putstr(__TIME__);
	ser_putstr("\n");

	while (1) {
		if (!c) {
			ser_putstr("Command:\n");
			ser_putstr("Press 'D' for serial download, using the 'sendnto' utility\n");
			ser_putstr("Press 'M' for SDMMC download, IFS filename MUST be 'QNX-IFS'.\n");
			c = ser_getchar();
		}
		switch (c) {
		case 'D':
		case 'd':
			ser_putstr("send image now...\n");
			if (image_download_ser(image)) {
				ser_putstr("download failed...\n");
				c = 0;
			}
			else
				ser_putstr("download OK...\n");
			break;
		case 'M':
		case 'm':
			ser_putstr("SDMMC download...\n");
			if (sdmmc_load_file(image, "QNX-IFS") == 0) {
				ser_putstr("load image done.\n");
				/* Proceed to image scan */
			}
			else {
				ser_putstr("Load image failed.\n");
				c = 0;
			}
			break;
		default:
			ser_putstr("Unknown command.\n");
			c = 0;
		}
		if (!c) continue;


		image = image_scan_2(image, image + 0x200,1);

		if (image != 0xffffffff) {
			ser_putstr("Found image               @ 0x");
			ser_puthex(image);
			ser_putstr("\n");
			image_setup(image);

			ser_putstr("Jumping to startup        @ 0x");
			ser_puthex(startup_hdr.startup_vaddr);
			ser_putstr("\n\n");
			image_start(image);

			/* Never reach here */
			return 0;
		}
		ser_putstr("Image_scan failed...\n");
	}

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/boards/mx6q-sabresmart/main.c $ $Rev: 818268 $")
#endif



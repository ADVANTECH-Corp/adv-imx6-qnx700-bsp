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


/*
 * The i.MX6 series SOCs contain 4 USB controllers:
 * One OTG controller which can function in Host or Device mode.  The OTG module uses an on chip UTMI PHY
 * One Host controller which uses an on chip UTMI phy
 * Two Host controllers which use on chip HS-IC PHYs
 */

#include "startup.h"
#include "board.h"

void init_sata(void)
{

#if defined(CONFIG_MACH_MX6Q_DMS_BA16)
#else
	/* SATA_DEVSLP as GPIO4[15] output.
	 * We drive this LOW to exit DEVICE SLEEP mode. */
	pinmux_set_swmux(SWMUX_KEY_ROW4, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO4_BASE, 15, 0);
#endif
	/* initialise the SATA subsystem */
	mx6x_init_sata(ANATOP_PLL8_ENET_REF_ENET_50M);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/startup/boards/imx6x/smart-device/init_sata.c $ $Rev: 753817 $")
#endif

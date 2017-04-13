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

/*
 * The i.MX6 Q Sabre-Smart BSP currently supports the OTG controller.
 * The USB Host controller (H1) is connected to the mini PCIe slot.
 * We have enabled it here but it's untested and is not an 'officially'
 * supported part of this BSP.
 */

#include "startup.h"
#include "board.h"

void mx6q_usb_otg_host_init(void)
{
	/* ID pin muxing */
	pinmux_set_swmux(SWMUX_GPIO_1, MUX_CTL_MUX_MODE_ALT3);
	pinmux_set_padcfg(SWPAD_GPIO_1, MX6X_PAD_SETTINGS_USB);

#if defined(CONFIG_MACH_MX6Q_DMS_BA16)
        pinmux_set_swmux(SWMUX_KEY_ROW4, MUX_CTL_MUX_MODE_ALT5);
        pinmux_set_padcfg(SWMUX_KEY_ROW4, MX6X_PAD_SETTINGS_GPIO);
        mx6x_set_gpio_output(MX6X_GPIO4_BASE, 15, 1 );
#else
	/* Enable power to MX6 USB OTG controller by driving a GPIO high.
	 * USB_OTG_VBUS
	 * USB_OTG_PWR_EN = EIM_D22(ALT5) = GPIO[22] of instance: gpio3 */
	pinmux_set_swmux(SWMUX_EIM_D22, MUX_CTL_MUX_MODE_ALT5);
	//gpio_drive_output( 3, 22, 1 );
	mx6x_set_gpio_output(MX6X_GPIO3_BASE, 22, 1);
#endif

	/* USB OTG select GPIO_1 */
	out32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR1, in32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR1) | (1 << 13));

	/* Initialize OTG core */
	mx6x_init_usb_otg();

	/* OTG Host connects to PHY0  */
	mx6x_init_usb_phy(MX6X_USBPHY0_BASE);
}

void mx6q_usb_host1_init(void)
{
	/* Enable power to iMX6 USB Host controller H1 for PCIe slot by driving a GPIO high.
	 * USB_H1_VBUS
	 * USB_H1_PWR_EN = ENET_TXD1(ALT5) = GPIO[29] of instance: gpio1 */
	pinmux_set_swmux(SWMUX_ENET_TXD1, MUX_CTL_MUX_MODE_ALT5);
	//gpio_drive_output( 1, 29, 1 );
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 29, 1);

	/* Initialize Host1 */
	mx6x_init_usb_host1();

	/* USB Host1 connects to PHY1  */
	mx6x_init_usb_phy(MX6X_USBPHY1_BASE);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/imx6x/smart-device/init_usb.c $ $Rev: 742242 $")
#endif

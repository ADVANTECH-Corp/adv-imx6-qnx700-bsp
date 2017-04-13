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

#include "startup.h"
#include <arm/mx6x.h>

#define SRC_TXFS				0
#define SRC_TXCLK				0
#define SRC_RXFS				1
#define SRC_RXCLK				1

#define RXDSRC(_PORT_)			((((_PORT_) - 1) & 0x7) << 13)
#define TXFS(_DIR_, _PORT_)		((((_DIR_) & 1) << 30) | ((((_PORT_) - 1) & 0x7) << 27))
#define TXCLK(_DIR_, _PORT_)	((((_DIR_) & 1) << 25) | ((((_PORT_) - 1) & 0x7) << 22))

#define PTCR(_pno_)				(((_pno_) - 1) * 8)
#define PDCR(_pno_)				((((_pno_) - 1) * 8) + 4)

static void init_audiomux(uint32_t port_master, uint32_t port_slave)
{
	uint32_t master_ptcr, slave_ptcr, master_pdcr, slave_pdcr;

	master_ptcr = (AUDMUX_SYNC);
	slave_ptcr = (AUDMUX_SYNC);

	master_pdcr = RXDSRC(port_slave);
	slave_pdcr = RXDSRC(port_master);

	/* Set TxFS direction & source */
	slave_ptcr |= (AUDMUX_TFS_DIROUT);
	slave_ptcr |= TXFS(SRC_TXFS, port_master);

	/* Set TxClk direction and source */
	slave_ptcr |= (AUDMUX_TCLK_DIROUT);
	slave_ptcr |= TXCLK(SRC_TXCLK, port_master);

	out32 (MX6X_AUDMUX_BASE + PTCR(port_master), master_ptcr);
	out32 (MX6X_AUDMUX_BASE + PDCR(port_master), master_pdcr);
	out32 (MX6X_AUDMUX_BASE + PTCR(port_slave), slave_ptcr);
	out32 (MX6X_AUDMUX_BASE + PDCR(port_slave), slave_pdcr);
}

static void init_ssi2_clk(void)
{
	volatile uint32_t val;

	/* Divide Audio PLL clock frequency down to generate 12.288MHz for SSI2 */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CS2CDR);
	val &= ~(SSI1_CLK_DIV_MASK);
	val |= SSI2_CLK_PRED(2);
	val |= SSI2_CLK_PODF(28);
	out32(MX6X_CCM_BASE + MX6X_CCM_CS2CDR, val);

	/* Select the Audio PLL (PLL4) as the clock source for SSI2 main/root clock */
	val = in32(MX6X_CCM_BASE + MX6X_CCM_CSCMR1);
	val &= ~(CSCMR1_SSI2_CLK_SEL_MASK);
	val |= CSCMR1_SSI2_CLK_SEL_PLL4;
	out32(MX6X_CCM_BASE + MX6X_CCM_CSCMR1, val);
}

#define CCM_ANALOG_PLL_AUDIO_CTRL				0x70
#define CCM_ANALOG_PLL_AUDIO_CTRL_SET			0x74
#define CCM_ANALOG_PLL_AUDIO_CTRL_CLR			0x78
	/*
	 * Note that bits [20:19] are defined differently by the i.MX6 Quad/Dual compared to the i.MX6 DualLite/Solo
	 * The i.MX6 Solo/DualLite use bits 20:19 to control a PLL post-divider, whereas the Quad/Dual use bits 20:19
	 * for testing purposes.
	 */
	#define PLL_AUDIO_CTRL_POST_DIV_1			(0x3 << 19)
	#define PLL_AUDIO_CTRL_BYPASS				(1 << 16)
	#define PLL_AUDIO_CTRL_ENABLE				(1 << 13)
	#define PLL_AUDIO_CTRL_POWERDOWN			(1 << 12)
	#define PLL_AUDIO_CTRL_MFI_MASK				(0x7F << 0)	/* In the docs these bits are labeled DIV_SELECT */
#define CCM_ANALOG_PLL_AUDIO_NUM				0x80
#define CCM_ANALOG_PLL_AUDIO_DENOM				0x90


/* weilun@adv - begin */
#if 0
/*
 * Connect external port 3 to internal Port 2. Port 2 is always connected to SSI-2.
 * The codec (wm8962) acts as the clock master, providing both the frame clock, and
 * the bit clock to the i.MX6 Q.
 */
void init_audio (void)
{
	/* Configure PLL4 (Audio PLL) to generate a 688.128Mhz Clock (over 650Mhz minimum PLL_OUT frequencey and evenly divisible to 12.288Mhz)
	 * PLL_OUT = Fref * (MFI + MFN/MFD)
	 *		= 24Mhz * (28 + 672/1000)
	 *		= 688.128MHz
	 */
	out32(MX6X_ANATOP_BASE + CCM_ANALOG_PLL_AUDIO_NUM, 0x2a0);
	out32(MX6X_ANATOP_BASE + CCM_ANALOG_PLL_AUDIO_DENOM, 0x3e8);
	out32(MX6X_ANATOP_BASE + CCM_ANALOG_PLL_AUDIO_CTRL_CLR, PLL_AUDIO_CTRL_MFI_MASK);
	out32(MX6X_ANATOP_BASE + CCM_ANALOG_PLL_AUDIO_CTRL_SET, (0x1c & PLL_AUDIO_CTRL_MFI_MASK) | PLL_AUDIO_CTRL_POST_DIV_1);

	/* Power up and enable Audio PLL (PLL4) */
	out32(MX6X_ANATOP_BASE + CCM_ANALOG_PLL_AUDIO_CTRL_CLR, PLL_AUDIO_CTRL_BYPASS | PLL_AUDIO_CTRL_POWERDOWN);
	out32(MX6X_ANATOP_BASE + CCM_ANALOG_PLL_AUDIO_CTRL_SET, PLL_AUDIO_CTRL_ENABLE);

	init_ssi2_clk();

	/*
	 * Connect external port 3 to internal Port 2. Port 2 is always connected to SSI-2.
	 * The i.MX6 Q acts as the clock slave, providing both the frame clock, and
	 * the bit clock to the codec (WM8962).
	 */
	init_audiomux(2, 3);
}
#else
/*
 * Connect external port 4 to internal Port 2.  Port 2 is always connected to SSI-2.
 * The codec (SGTL5000) acts as the clock master, providing both the frame clock, and
 * the bit clock to the i.MX6 Q.
 */
void init_audio (void)
{
#if defined(CONFIG_MACH_MX6Q_DMS_BA16)
	init_audiomux(4,1);
#else
	init_audiomux(3,2);
#endif
}
#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/imx6x/smart-device/init_audio.c $ $Rev: 780610 $")
#endif

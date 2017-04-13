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
 * Routines to initialize the various hardware subsystems
 * on the i.MX6Q Sabre-Smart
 */

#include "startup.h"
#include "board.h"


/* weilun@adv - begin */
//#define CONFIG_MACH_MX6Q_RSB_4410
//#define CONFIG_MACH_MX6Q_ROM_5420
//#define CONFIG_MACH_MX6Q_ROM_7420
//#define CONFIG_MACH_MX6Q_DMS_BA16
/* weilun@adv - end */

#define MX6DQ_GPR1_MIPI_IPU1_MUX_MASK	(0x1 << 19)
#define MX6DQ_GPR1_MIPI_IPU1_PARALLEL	(0x1 << 19)

// No pull-up/down.  Average drive strength
#define MX6X_PAD_SETTINGS_GPIO_OUT ( PAD_CTL_SPEED_LOW | PAD_CTL_DSE_80_OHM | PAD_CTL_SRE_SLOW )

#define MX6X_PAD_SETTINGS_USDHC_CDWP	(PAD_CTL_PKE_DISABLE | PAD_CTL_SPEED_LOW | \
										PAD_CTL_DSE_DISABLE | PAD_CTL_HYS_ENABLE)

#define MX6Q_PAD_SETTINGS_UART (PAD_CTL_HYS_ENABLE | PAD_CTL_PKE_ENABLE | \
								PAD_CTL_PUS_100K_PU | PAD_CTL_SPEED_MEDIUM | \
								PAD_CTL_DSE_40_OHM | PAD_CTL_SRE_FAST | PAD_CTL_PUE_PULL)
#define CCM_CCOSR_CLKO1_EN	(0x1 << 7)
#define CCM_CCOSR_CLKO1_DIV_8	(0x7 << 4)
#define CCM_CCOSR_CLKO1_AHB	(0xb << 0)

#define PLL_ENET_CTRL_ENABLE      (1 << 13)
#define PLL_ENET_CTRL_DIV_SELECT  (3 << 0)

#define GPIO_LOW		0
#define GPIO_HIGH		1

#define GPIO_IRQ_LEVEL_LOW		0
#define GPIO_IRQ_LEVEL_HIGH		1
#define GPIO_IRQ_EDGE_RISE		2
#define GPIO_IRQ_EDGE_FALL		3


/* weilun@adv - begin */
void mx6q_init_debug(void)
{
#if 0
	#include <arm/mx6x.h>

	mx6x_usleep(500000);
	unsigned int reg6;
	unsigned int reg7;
	unsigned int reg1 = in32(MX6X_IOMUXC_BASE + 0x1f8);
	unsigned int reg2 = in32(MX6X_IOMUXC_BASE + 0x1fc);
	mx6x_get_gpio_value(MX6X_GPIO4_BASE, 6, &reg6);
	mx6x_get_gpio_value(MX6X_GPIO4_BASE, 7, &reg7);

	kprintf("weilun@adv debug IOMUX_KEY_COL0 = 0x%x, gpio_value=%d\n", reg1, reg6);
	kprintf("weilun@adv debug IOMUX_KEY_ROW0 = 0x%x, gpio_alue=%d\n", reg2, reg7);
#endif
}
/* weilun@adv - end */


void mx6q_init_i2c(void)
{
	/* I2C1  SCL */
	pinmux_set_swmux(SWMUX_CSI0_DAT9, MUX_CTL_MUX_MODE_ALT4 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_CSI0_DAT9, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C1_IPP_SCL_IN, 0x1);

	/* I2C1  SDA */
	pinmux_set_swmux(SWMUX_CSI0_DAT8, MUX_CTL_MUX_MODE_ALT4 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_CSI0_DAT8, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C1_IPP_SDA_IN, 0x1);

	/* I2C2  SCL */
	pinmux_set_swmux(SWMUX_KEY_COL3, MUX_CTL_MUX_MODE_ALT4 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_KEY_COL3, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C2_IPP_SCL_IN, 0x1);

	/* I2C2  SDA */
	pinmux_set_swmux(SWMUX_KEY_ROW3, MUX_CTL_MUX_MODE_ALT4 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_KEY_ROW3, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C2_IPP_SDA_IN, 0x1);

	/* I2C3 SCL */
	pinmux_set_swmux(SWMUX_GPIO_3, MUX_CTL_MUX_MODE_ALT2 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_GPIO_3, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C3_IPP_SCL_IN, 0x1);

	/* I2C3  SDA */
	pinmux_set_swmux(SWMUX_GPIO_6, MUX_CTL_MUX_MODE_ALT2 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_GPIO_6, MX6X_PAD_SETTINGS_I2C);
	pinmux_set_input(SWINPUT_I2C3_IPP_SDA_IN, 0x1);
}

void mx6q_init_audmux_pins(void)
{
	// CCM CLKO pin muxing
#if defined(CONFIG_MACH_MX6Q_ROM_5420) || defined(CONFIG_MACH_MX6Q_RSB_4410) || defined(CONFIG_MACH_MX6Q_ROM_7420)
	pinmux_set_swmux(SWMUX_GPIO_0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_GPIO_0, MX6X_PAD_SETTINGS_CLKO);

	// AUDMUX pin muxing
	// Note: the AUDMUX3 pins are not daisy chained
	// AUD3_RXD
	pinmux_set_swmux(SWMUX_CSI0_DAT7, MUX_CTL_MUX_MODE_ALT4);

	// AUD3_TXC
	pinmux_set_swmux(SWMUX_CSI0_DAT4, MUX_CTL_MUX_MODE_ALT4);

	// AUD3_TXD
	pinmux_set_swmux(SWMUX_CSI0_DAT5, MUX_CTL_MUX_MODE_ALT4);

	// AUD3_TXFS
	pinmux_set_swmux(SWMUX_CSI0_DAT6, MUX_CTL_MUX_MODE_ALT4);
#endif
#if defined(CONFIG_MACH_MX6Q_DMS_BA16)
	// AUD4_TXC -> AUD_I2S_SCLK
	pinmux_set_swmux(SWMUX_DISP0_DAT20, MUX_CTL_MUX_MODE_ALT3);
	pinmux_set_input(SWINPUT_AUDMUX_P4_TXCLK_AMX, 0x0);

	//AUD4_TXD _-> AUD_I2S_DOUT
	pinmux_set_swmux(SWMUX_DISP0_DAT21, MUX_CTL_MUX_MODE_ALT3);
	pinmux_set_input(SWINPUT_AUDMUX_P4_DB_AMX, 0x1);

	//AUD4_TXFS -> AUD_I2S_LRCLK
	pinmux_set_swmux(SWMUX_DISP0_DAT22, MUX_CTL_MUX_MODE_ALT3);
	pinmux_set_input(SWINPUT_AUDMUX_P4_TXFS_AMX, 0x1);

	//AUD4_RXD -> AUD_I2S_DIN
	pinmux_set_swmux(SWMUX_DISP0_DAT23, MUX_CTL_MUX_MODE_ALT3);
	pinmux_set_input(SWINPUT_AUDMUX_P4_DA_AMX, 0x1);
#endif

	// Configure CLKO to produce 16.5MHz master clock. If you modify this, you will also have to adjust the FLL N.K value for the audio codec.
	out32(MX6X_CCM_BASE + MX6X_CCM_CCOSR, CCM_CCOSR_CLKO1_EN | CCM_CCOSR_CLKO1_AHB | CCM_CCOSR_CLKO1_DIV_8);
}

void mx6q_init_enet(void)
{
	uint32_t reg;
	// RGMII MDIO - transfers control info between MAC and PHY
	pinmux_set_swmux(SWMUX_ENET_MDIO, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC0_MDIO,0);

	// RGMII MDC - output from MAC to PHY, provides clock reference for MDIO
	pinmux_set_swmux(SWMUX_ENET_MDC, MUX_CTL_MUX_MODE_ALT1);

	// RGMII TXC - output from MAC, provides clock used by RGMII_TXD[3:0], RGMII_TX_CTL
	pinmux_set_swmux(SWMUX_RGMII_TXC, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TXC, MX6X_PAD_SETTINGS_ENET);

	// RGMII TXD[3:0] - Transmit Data Output
	pinmux_set_swmux(SWMUX_RGMII_TD0, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TD0, MX6X_PAD_SETTINGS_ENET);

	pinmux_set_swmux(SWMUX_RGMII_TD1, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TD1, MX6X_PAD_SETTINGS_ENET);

	pinmux_set_swmux(SWMUX_RGMII_TD2, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TD2, MX6X_PAD_SETTINGS_ENET);

	pinmux_set_swmux(SWMUX_RGMII_TD3, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TD3, MX6X_PAD_SETTINGS_ENET);

	// RGMII TX_CTL - contains TXEN on TXC rising edge, TXEN XOR TXERR on TXC falling edge
	pinmux_set_swmux(SWMUX_RGMII_TX_CTL, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_TX_CTL, MX6X_PAD_SETTINGS_ENET);

	// set ENET_REF_CLK to mux mode 1 - TX_CLK, this is a 125MHz input which is driven by the PHY
	pinmux_set_swmux(SWMUX_ENET_REF_CLK, MUX_CTL_MUX_MODE_ALT1);	

	pinmux_set_swmux(SWMUX_RGMII_RXC, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RXC, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC0_RXCLK,0);

	pinmux_set_swmux(SWMUX_RGMII_RD0, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RD0, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC_RXDATA_0,0);

	pinmux_set_swmux(SWMUX_RGMII_RD1, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RD1, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC_RXDATA_1,0);

	pinmux_set_swmux(SWMUX_RGMII_RD2, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RD2, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC_RXDATA_2,0);

	pinmux_set_swmux(SWMUX_RGMII_RD3, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RD3, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC_RXDATA_3,0);

	pinmux_set_swmux(SWMUX_RGMII_RX_CTL, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_RGMII_RX_CTL, MX6X_PAD_SETTINGS_ENET);
	pinmux_set_input(SWINPUT_ENET_IPP_IND_MAC0_RXEN,0);

	/* RGMII Phy interrupt set to GPIO1[26] and configure it as an input */
	pinmux_set_swmux(SWMUX_ENET_RXD1, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_input(MX6X_GPIO1_BASE, 26);

	/* RGMII_NRST is set to GPIO1[25] and configure it as an output*/
	pinmux_set_swmux(SWMUX_ENET_CRS_DV, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 25, 1);

	/* ETH_WOL_INT is set to GPIO1[28] and configure it as an input*/
	pinmux_set_swmux(SWMUX_ENET_TX_EN, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_input(MX6X_GPIO1_BASE, 28);

	/* set up GPIO_16 for timestamps */
	pinmux_set_swmux(SWMUX_GPIO_16, MUX_CTL_MUX_MODE_ALT2 | MUX_CTL_SION);
	pinmux_set_input(SWINPUT_ENET_IPG_CLK_RMII, 1);

	/* set ENET_REF_CLK to PLL (not in RevC docs, new definition */
	reg = in32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR1);
	reg |= (1<<21);
	out32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR1, reg);

}


void mx6q_init_usdhc(void)
{
	/***********
	 * SD 2 used for SD cards or WiFi adapters such as WL18xxCOM82SDMMC
	***********/

	mx6_set_default_usdhc_clk(2);

	/* SD2 CLK */
	pinmux_set_swmux(SWMUX_SD2_CLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD2_CLK, MX6X_PAD_SETTINGS_USDHC);

	/* SD2 CMD */
	pinmux_set_swmux(SWMUX_SD2_CMD, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_SD2_CMD, MX6X_PAD_SETTINGS_USDHC);

	/* SD2 DAT0 */
	pinmux_set_swmux(SWMUX_SD2_DAT0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD2_DAT0, MX6X_PAD_SETTINGS_USDHC);

	/* SD2 DAT1 */
	pinmux_set_swmux(SWMUX_SD2_DAT1, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD2_DAT1, MX6X_PAD_SETTINGS_USDHC);

	/* SD2 DAT2 */
	pinmux_set_swmux(SWMUX_SD2_DAT2, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD2_DAT2, MX6X_PAD_SETTINGS_USDHC);

	/* SD2 DAT3 */
	pinmux_set_swmux(SWMUX_SD2_DAT3, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD2_DAT3, MX6X_PAD_SETTINGS_USDHC);

	/* SD2 DAT4 */
	pinmux_set_swmux(SWMUX_NANDF_D4, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_NANDF_D4, MX6X_PAD_SETTINGS_USDHC);

	/* SD2 DAT5 */
	pinmux_set_swmux(SWMUX_NANDF_D5, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_NANDF_D5, MX6X_PAD_SETTINGS_USDHC);

	/* SD2 DAT6 */
	pinmux_set_swmux(SWMUX_SD3_DAT6, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_NANDF_D6, MX6X_PAD_SETTINGS_USDHC);

	/* SD2 DAT7 */
	pinmux_set_swmux(SWMUX_SD3_DAT7, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_NANDF_D7, MX6X_PAD_SETTINGS_USDHC);

	/* SD2 Write Protect - configure GPIO2[3] as an input */
	pinmux_set_swmux(SWMUX_NANDF_D3, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_NANDF_D3, MX6X_PAD_SETTINGS_USDHC_CDWP);
	mx6x_set_gpio_input(MX6X_GPIO2_BASE, 3);

	/* SD2 Card Detect - configure GPIO2[2] as an input */
	pinmux_set_swmux(SWMUX_NANDF_D2, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_NANDF_D2, MX6X_PAD_SETTINGS_USDHC_CDWP);
	mx6x_set_gpio_input(MX6X_GPIO2_BASE, 2);


	/***********
	 * SD 3
	***********/

	mx6_set_default_usdhc_clk(3);

	/* SD3 CLK */
	pinmux_set_swmux(SWMUX_SD3_CLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_CLK, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 CMD */
	pinmux_set_swmux(SWMUX_SD3_CMD, MUX_CTL_MUX_MODE_ALT0 | MUX_CTL_SION);
	pinmux_set_padcfg(SWPAD_SD3_CMD, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT0 */
	pinmux_set_swmux(SWMUX_SD3_DAT0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT0, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT1 */
	pinmux_set_swmux(SWMUX_SD3_DAT1, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT1, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT2 */
	pinmux_set_swmux(SWMUX_SD3_DAT2, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT2, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT3 */
	pinmux_set_swmux(SWMUX_SD3_DAT3, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT3, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT4 */
	pinmux_set_swmux(SWMUX_SD3_DAT4, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT4, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT5 */
	pinmux_set_swmux(SWMUX_SD3_DAT5, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT5, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT6 */
	pinmux_set_swmux(SWMUX_SD3_DAT6, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT6, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 DAT7 */
	pinmux_set_swmux(SWMUX_SD3_DAT7, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_padcfg(SWPAD_SD3_DAT7, MX6X_PAD_SETTINGS_USDHC);

	/* SD3 Write Protect - configure GPIO2[1] as an input */
	pinmux_set_swmux(SWMUX_NANDF_D1, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_NANDF_D1, MX6X_PAD_SETTINGS_USDHC_CDWP);
	mx6x_set_gpio_input(MX6X_GPIO2_BASE, 1);

	/* SD3 Card Detect - configure GPIO2[0] as an input */
	pinmux_set_swmux(SWMUX_NANDF_D0, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_NANDF_D0, MX6X_PAD_SETTINGS_USDHC_CDWP);
	mx6x_set_gpio_input(MX6X_GPIO2_BASE, 0);



	/***********
	 * SD4: eMMC
	 ***********/
	mx6_set_default_usdhc_clk(4);
	// default  SD4 Pin MUX are correct

}

/*
 * WiFi and BT support for optional TI WL18xxCOM82SDMMC adapter board (connects to J13 connector
 * and J500 (SD2) connector on Sabre Smart Devices Board)
 * Note that WiFi/BT conflict with NOR SPI, and the BT enable signal is shared with the PCIe
 * 
 */
void mx6q_init_wifi(void)
{
	/* weilun@adv, begin */
#if 0
	/*
	 * SPINOR_MOSI / CSPI1_MOSI / KEY_ROW0 on Sabre Smart is WL_EN on COMQ adapter board 
	 * Configure as GPIO4[7]
	 */
	pinmux_set_swmux(SWMUX_KEY_ROW0, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWMUX_KEY_ROW0, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO4_BASE, 7, 0);

	/*
	 * SPINOR_CLK / CSPI1_CLK / KEL_COL0 on Sabre Smart is WL_IRQ on COMQ adapter board
	 * Configure as GPIO4[6]
	 */
	pinmux_set_swmux(SWMUX_KEY_COL0, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_input(MX6X_GPIO4_BASE, 6);
#endif
	/* weilun@adv, end */
}

void mx6q_init_bt(void)
{
	/*
	 * KEY_ROW6 / BT_PWD_L / GPIO_2 - BT Reset line. Disable BT chip, HCI driver will enable BT.
	 */
	pinmux_set_swmux(SWMUX_GPIO_2, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWMUX_GPIO_2, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 2, 0);

	/*
	 * SPINOR_CS0 / CSPI1_CS0 / KEY_ROW1 - BT_UART_RXD
	 */
	pinmux_set_swmux(SWMUX_KEY_ROW1, MUX_CTL_MUX_MODE_ALT4);
	pinmux_set_padcfg(SWMUX_KEY_ROW1, MX6Q_PAD_SETTINGS_UART);
	pinmux_set_input(SWINPUT_UART5_IPP_UART_RXD_MUX, 0x1);
	
	/*
	 * KEY_ROW4 - BT_UART_CTS
	 */
#if defined(CONFIG_MACH_MX6Q_DMS_BA16)
#else
	pinmux_set_swmux(SWMUX_KEY_ROW4, MUX_CTL_MUX_MODE_ALT4);
	pinmux_set_padcfg(SWMUX_KEY_ROW4, MX6Q_PAD_SETTINGS_UART);
#endif
	/*
	 * SPINOR_MISO / CSPI1_MISO / KEY_COL1 - BT_UART_TXD
	 */
	pinmux_set_swmux(SWMUX_KEY_COL1, MUX_CTL_MUX_MODE_ALT4);
	pinmux_set_padcfg(SWMUX_KEY_COL1, MX6Q_PAD_SETTINGS_UART);

	/*
	 * KEY_COL4 - BT_UART_RTS - conflicts with PCIe W_DISABLE#
	 */
	pinmux_set_swmux(SWMUX_KEY_COL4, MUX_CTL_MUX_MODE_ALT4);
	pinmux_set_padcfg(SWMUX_KEY_COL4, MX6Q_PAD_SETTINGS_UART);
	pinmux_set_input(SWINPUT_UART5_IPP_UART_RTS_B, 0x0);
}


void mx6q_init_ecspi(void)
{
	/* SPI SCLK */
	pinmux_set_swmux(SWMUX_EIM_D16, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_EIM_D16, MX6X_PAD_SETTINGS_ECSPI);
	pinmux_set_input(SWINPUT_ECSPI1_IPP_CSPI_CLK, 0x0); /* Mode ALT1 */

	/* SPI MISO */
	pinmux_set_swmux(SWMUX_EIM_D17, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_EIM_D17, MX6X_PAD_SETTINGS_ECSPI);
	pinmux_set_input(SWINPUT_ECSPI1_IPP_IND_MISO, 0x0); /* Mode ALT1 */

	/* SPI MOSI */
	pinmux_set_swmux(SWMUX_EIM_D18, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_EIM_D18, MX6X_PAD_SETTINGS_ECSPI);
	pinmux_set_input(SWINPUT_ECSPI1_IPP_IND_MOSI, 0x0); /* Mode ALT1 */

	/* Select mux mode ALT1 for SS0 */
	/* ROM-5420, RSB-4410 and DMS-BA16 */
#if defined(CONFIG_MACH_MX6Q_ROM_5420) || defined(CONFIG_MACH_MX6Q_RSB_4410) || defined(CONFIG_MACH_MX6Q_DMS_BA16)
	pinmux_set_swmux(SWMUX_EIM_EB2, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_EIM_EB2, MX6X_PAD_SETTINGS_ECSPI);
	pinmux_set_input(SWINPUT_ECSPI1_IPP_IND_SS_B_0, 0x0); /* Mode ALT1 */
#endif
	/* Select mux mode ALT1 for SS1 */
	/* ROM-7420 */
#if defined(CONFIG_MACH_MX6Q_ROM_7420)
	pinmux_set_swmux(SWMUX_EIM_D19, MUX_CTL_MUX_MODE_ALT1);
	pinmux_set_padcfg(SWPAD_EIM_D19, MX6X_PAD_SETTINGS_ECSPI);
	pinmux_set_input(SWINPUT_ECSPI1_IPP_IND_SS_B_1, 0x0); /* Mode ALT1 */
#endif
}

void mx6q_init_lvds(void)
{
/* ROM-5420 && RSB-4410 */
#if defined(CONFIG_MACH_MX6Q_ROM_5420) || defined(CONFIG_MACH_MX6Q_RSB_4410)
#if defined(CONFIG_MACH_MX6Q_RSB_4410)
	/* Enable PWM */
	pinmux_set_swmux(SWMUX_GPIO_1, MUX_CTL_MUX_MODE_ALT4);
#endif
	/* LVDS_VDD_EN */
	pinmux_set_swmux(SWMUX_KEY_ROW0, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWMUX_KEY_ROW0, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO4_BASE, 7, GPIO_HIGH);

	/* LVDS_BKLT_EN */
	pinmux_set_swmux(SWMUX_KEY_COL0, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWMUX_KEY_COL0, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO4_BASE, 6, GPIO_HIGH);
#endif
/* ROM-7420 */
#if defined(CONFIG_MACH_MX6Q_ROM_7420)
	/* Enable PWM */
	pinmux_set_swmux(SWMUX_SD1_DAT3, MUX_CTL_MUX_MODE_ALT3);
	
	/* LVDS_VDD_EN */
	pinmux_set_swmux(SWMUX_NANDF_CLE, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWMUX_NANDF_CLE, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO6_BASE, 7, GPIO_HIGH );

	/* LVDS_BKLT_EN */
	pinmux_set_swmux(SWMUX_NANDF_WP_B, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWMUX_NANDF_WP_B, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO6_BASE, 9, GPIO_HIGH );
#endif
/* DMS-BA16 */
#if defined(CONFIG_MACH_MX6Q_DMS_BA16)

    	/* disable the split mode */
    	#define IOMUXC_GPR2_SPLIT_MODE_EN_OFFSET                4
    	#define IOMUXC_GPR2_SPLIT_MODE_EN_MASK                  (1<<IOMUXC_GPR2_SPLIT_MODE_EN_OFFSET)
    	uint32_t gpr2;

    	gpr2 = in32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR2);
    	gpr2 &= ~IOMUXC_GPR2_SPLIT_MODE_EN_MASK;
    	out32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR2, gpr2);

	/* Enable PWM */
	/* ATL3 for PWM1_OUT */
	pinmux_set_swmux(SWMUX_SD1_DAT3, MUX_CTL_MUX_MODE_ALT3);

	/* LVDS_VDD_EN */
	pinmux_set_swmux(SWMUX_EIM_D22, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWMUX_EIM_D22, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO3_BASE, 22, GPIO_HIGH );

	/* LVDS_BKLT_EN */
	pinmux_set_swmux(SWMUX_GPIO_0, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWMUX_GPIO_0, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 0, GPIO_HIGH );
#endif
}

void mx6q_init_lcd_panel(void)
{
	/* IPU1 Display Interface 0 clock */
	pinmux_set_swmux(SWMUX_DI0_DISP_CLK, MUX_CTL_MUX_MODE_ALT0);

	/* LCD EN */
	pinmux_set_swmux(SWMUX_DI0_PIN15, MUX_CTL_MUX_MODE_ALT0);

	/* LCD HSYNC */
	pinmux_set_swmux(SWMUX_DI0_PIN2, MUX_CTL_MUX_MODE_ALT0);

	/* LCD VSYNC */
	pinmux_set_swmux(SWMUX_DI0_PIN3, MUX_CTL_MUX_MODE_ALT0);

	/* Data Lines */
	pinmux_set_swmux(SWMUX_DISP0_DAT0, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT1, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT2, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT3, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT4, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT5, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT6, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT7, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT8, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT9, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT10, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT11, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT12, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT13, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT14, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT15, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT16, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT17, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT18, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT19, MUX_CTL_MUX_MODE_ALT0);
#if defined(CONFIG_MACH_MX6Q_ROM_5420) || defined(CONFIG_MACH_MX6Q_ROM_7420) || defined(CONFIG_MACH_MX6Q_RSB_4410) 
	pinmux_set_swmux(SWMUX_DISP0_DAT20, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT21, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT22, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_DISP0_DAT23, MUX_CTL_MUX_MODE_ALT0);
#endif

#if 0
	/* Configure pin as GPIO1_30 (Power Enable)
	 * Force LCD_EN (ENET_TXD0) HIGH to enable LCD */
	pinmux_set_swmux(SWMUX_ENET_TXD0, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 30, GPIO_HIGH);

	/* Note: the touchscreen is configured by [mx6q_init_sensors] */
#endif

}

/*
 * Initialise all the GPIO lines used to control the mini PCIe slot
 *
 * - MPCIE_3V3
 *   PCIE_PWR_EN = EIM_D19(ALT5) = GPIO[19] of instance: gpio3
 *
 * - WAKE# => PCIE_WAKE_B => CSI0_DATA_EN
 * - W_DISABLE# => PCIE_DIS_B => KEY_COL4
 * - PERST# => PCIE_RST_B => SPDIF_OUT => GPIO_17
 * - CLKREQ# is unconnected on the SabreSmart
 *
 * I2C bus 3 connects to the PCIe SMB channel
 * - SMB_CLK = PCIe_SMB_CLK
 * - SMB_DATA = PCIe_SMB_DATA
 */
void mx6q_init_pcie( void )
{
	/* Disable power to PCIe device by driving a GPIO low.
	 * The driver will enable power itself at the right time
	 * MPCIE_3V3
	 * PCIE_PWR_EN = EIM_D19(ALT5) = GPIO[19] of instance: gpio3 */

/* ROM-7420 */
#if defined(CONFIG_MACH_MX6Q_ROM_7420)
	pinmux_set_swmux(SWMUX_EIM_D20, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_EIM_D20, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO3_BASE, 20, GPIO_LOW );
#else
	pinmux_set_swmux(SWMUX_EIM_D19, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_EIM_D19, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO3_BASE, 19, GPIO_LOW);
#endif

	/* Wake-up line is an input */
#if defined(CONFIG_MACH_MX6Q_DMS_BA16)
	pinmux_set_swmux(SWMUX_GPIO_5, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_input(MX6X_GPIO1_BASE, 5);
#else
	/* WAKE# => PCIE_WAKE_B => CSI0_DATA_EN(ALT5) = GPIO[20] of instance: gpio5. */
	pinmux_set_swmux(SWMUX_CSI0_DATA_EN, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_input(MX6X_GPIO5_BASE, 20);
#endif

	/* Assert disable by driving LOW
	 * W_DISABLE# => PCIE_DIS_B => KEY_COL4(ALT5) = GPIO[14] of instance: gpio4. */
	pinmux_set_swmux(SWMUX_KEY_COL4, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_KEY_COL4, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO4_BASE, 14, GPIO_LOW);

	/* De-assert reset by driving HIGH
	 * PERST# => PCIE_RST_B => SPDIF_OUT => GPIO_17(ALT5) = GPIO[12] of instance: gpio7. */
	pinmux_set_swmux(SWMUX_GPIO_17, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_GPIO_17, MX6X_PAD_SETTINGS_GPIO_OUT);
	mx6x_set_gpio_output(MX6X_GPIO7_BASE, 12, GPIO_HIGH);

}


/* The Sabre Smart is fitted with various sensors:
 * - 3 axis accelerometer  I2C_1 = MMA8451
 * - 3 axis magentometer   I2C_3 = MAG3110
 * - ambient light sensor  I2C_3 = ISL29023
 * - touchscreen           I2C_2 = MAX11800
 *
 * Each of these can generate an interrupt to the iMX6
 * over a GPIO line.  This function sets up the pinmuxes
 * for the interrupt lines.
 */
void mx6q_init_sensors(void)
{
	/* Accelerometer ACCL_INT_IN */
	pinmux_set_swmux(SWMUX_SD1_CMD, MUX_CTL_MUX_MODE_ALT5);

	/* Ambient Light Sensor ALS_INT */
	pinmux_set_swmux(SWMUX_EIM_DA9, MUX_CTL_MUX_MODE_ALT5);

	/* Compass COMP_INT */
	pinmux_set_swmux(SWMUX_EIM_D16, MUX_CTL_MUX_MODE_ALT5);

	/* Touchscreen TS_INT : EIM_D26 pin as GPIO_3[26] for touchscreen IRQ */
	pinmux_set_swmux(SWMUX_EIM_D26, MUX_CTL_MUX_MODE_ALT5);
	pinmux_set_padcfg(SWPAD_EIM_D26, PAD_CTL_PKE_DISABLE | PAD_CTL_SPEED_LOW | PAD_CTL_DSE_DISABLE );
	mx6x_set_gpio_input(MX6X_GPIO3_BASE, 26);
	mx6x_gpio_set_irq_mode(MX6X_GPIO3_BASE, 26, GPIO_IRQ_LEVEL_LOW);
}


/*
 * Enable various external power domains on the SabreSmart:
 * - AUD_1V8, AUD_3V3
 *   CODEC_PWR_EN = KEY_COL2(ALT5) = GPIO[10] of instance: gpio4
 *     - Audio CODEC
 *
 * - SEN_1V8, SEN_3V3
 *   SENSOR_PWR_EN = EIM_EB3(ALT5) = GPIO[31] of instance: gpio2
 *     - Accelerometer
 *     - Magnetometer
 *     - Ambient Light Sensor
 *     - Barometer
 *
 * - AUX_5V
 *   AUX_5V_EN = NANDF_RB0(ALT5) = GPIO[10] of instance: gpio6
 *     - SATA
 *     - LVDS_0 (note: LVDS_1 is powered by PMIC_5V)
 *     - CAN 1
 *
 * - Pin 31 of J11
 *   DISP_PWR_EN = NANDF_CS1(ALT5) = GPIO[14] of instance: gpio6
 *
 * - Pin 79 of J504
 *   DISP0_PWR_EN = ENET_TXD0(ALT5) = GPIO[30] of instance: gpio1
 *
 * - Pin 1 of J503 (LVDS_0 output)
 *   CABC_EN0 = NANDF_CS2(ALT5) = GPIO[15] of instance: gpio6
 *
 * - Pin 1 of of J502 (LVDS_1 output)
 *   CABC_EN1 = NANDF_CS3(ALT5) = GPIO[16] of instance: gpio6.
 *
 * - GPS_3V15, GPS_1V5
 *   GPS_PWREN = EIM_DA0(ALT5) = GPIO[0] of instance: gpio3.
 *
 * =================================================================
 * Note: These items are initialised in [init_usb.c]
 *
 * - USB_OTG_VBUS
 *   USB_OTG_PWR_EN = EIM_D22(ALT5) = GPIO[22] of instance: gpio3
 *      - Power to iMX6 USB OTG controller
 *
 * - USB_H1_VBUS
 *   USB_H1_PWR_EN = ENET_TXD1(ALT5) = GPIO[29] of instance: gpio1
 *      - Power to iMX6 USB Host controller H1 for PCIe slot
 *
 */
void mx6q_init_external_power_rails(void)
{
	/* Note: The Sensors and Audio CODEC must be powered before we
	 * can communicate on I2C busses 1 and 3! */

	/* Enable power to Audio CODEC by driving a GPIO high.
	 * CODEC_PWR_EN = KEY_COL2(ALT5) = GPIO[10] of instance: gpio4. */
	pinmux_set_swmux(SWMUX_KEY_COL2, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO4_BASE, 10, GPIO_HIGH);

	/* Enable power to all sensors by driving a GPIO high.
	 * SENSOR_PWR_EN = EIM_EB3(ALT5) = GPIO[31] of instance: gpio2. */
	pinmux_set_swmux(SWMUX_EIM_EB3, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO2_BASE, 31, GPIO_HIGH);

	/* Enable Auxiliary 5V supply of J11 by driving a GPIO high.
	 * AUX_5V
	 * AUX_5V_EN = NANDF_RB0(ALT5) = GPIO[10] of instance: gpio6
	 *  - SATA
	 *  - LVDS_0 (note: LVDS_1 is powered by PMIC_5V)
	 *  - CAN 1 */
	pinmux_set_swmux(SWMUX_NANDF_RB0, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO6_BASE, 10, GPIO_HIGH);

	/* Enable power to Pin 31 of J11 by driving a GPIO high.
	 * DISP_PWR_EN = NANDF_CS1(ALT5) = GPIO[14] of instance: gpio6 */
	pinmux_set_swmux(SWMUX_NANDF_CS1, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO6_BASE, 14, GPIO_HIGH);

	/* Enable power to Pin 79 of J504 by driving a GPIO high.
	 * DISP0_PWR_EN = ENET_TXD0(ALT5) = GPIO[30] of instance: gpio1 */
	pinmux_set_swmux(SWMUX_ENET_TXD0, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 30, GPIO_HIGH);

	/* Enable power to Pin 1 of J503 (LVDS_0 output) by driving a GPIO high.
	 * CABC_EN0 = NANDF_CS2(ALT5) = GPIO[15] of instance: gpio6 */
	pinmux_set_swmux(SWMUX_NANDF_CS2, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO6_BASE, 15, GPIO_HIGH);

	/* Enable power to Pin 1 of of J502 (LVDS_1 output) by driving a GPIO high.
	 * CABC_EN1 = NANDF_CS3(ALT5) = GPIO[16] of instance: gpio6. */
	pinmux_set_swmux(SWMUX_NANDF_CS3, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO6_BASE, 16, GPIO_HIGH);

	/* Enable power to GPS by driving a GPIO high.
	 * GPS_3V15, GPS_1V5
	 * GPS_PWREN = EIM_DA0(ALT5) = GPIO[0] of instance: gpio3. */
	pinmux_set_swmux(SWMUX_EIM_DA0, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO3_BASE, 0, GPIO_HIGH);

	/* Note: PCIe power is configured in mx6q_init_pcie() but controlled by the PCI server */
}

#define CCM_CCOSR_CLKO2_EN		(0x1 << 24)
#define CCM_CCOSR_CLKO2_DIV_1		(0x0 << 21)
#define CCM_CCOSR_CLKO2_OSC_CLK		(0xe << 16)
#define CCM_CCOSR_CLK_OUT_SEL_CLKO2	(0x1 << 8)


void mx6q_init_mipi_camera(void)
{
	// GPIO pin muxing
	pinmux_set_swmux(SWMUX_GPIO_0, MUX_CTL_MUX_MODE_ALT0);		/* camera CLK */
	pinmux_set_swmux(SWMUX_SD1_DAT2, MUX_CTL_MUX_MODE_ALT5);	/* camera PWDN  GPIO1[19] */
	pinmux_set_swmux(SWMUX_SD1_CLK, MUX_CTL_MUX_MODE_ALT5);		/* camera RESET GPIO1[20] */

	// set a direction as an output
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR) | (0x1 << 19));
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_GDIR) | (0x1 << 20));
	mx6x_usleep(5000);

	// reset and power down camera
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) & ~(0x1 << 19));
	mx6x_usleep(5000);
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) & ~(0x1 << 20));
	mx6x_usleep(1000);
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) | (0x1 << 20));
	mx6x_usleep(5000);
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) | (0x1 << 19));

	//power up camera
	out32(MX6X_GPIO1_BASE + MX6X_GPIO_DR, in32(MX6X_GPIO1_BASE + MX6X_GPIO_DR) & ~(0x1 << 19));
}

void mx6q_init_parallel_camera(void)
{
	/* Configure CLKO to generate 24 MHz clock and provide clock to OV5462 CMOS sensor */
	pinmux_set_swmux(SWMUX_GPIO_0, MUX_CTL_MUX_MODE_ALT0);
	out32(MX6X_CCM_BASE + MX6X_CCM_CCOSR, CCM_CCOSR_CLKO2_EN | CCM_CCOSR_CLKO2_OSC_CLK | CCM_CCOSR_CLKO2_DIV_1 |
		CCM_CCOSR_CLK_OUT_SEL_CLKO2);

	/* CSI0_RST_B - GPIO1[17] */
	pinmux_set_swmux(SWMUX_SD1_DAT1, MUX_CTL_MUX_MODE_ALT5);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 17, GPIO_HIGH);

	/* CSI0_PWDN - GPIO1[16] */
	pinmux_set_swmux(SWMUX_SD1_DAT0, MUX_CTL_MUX_MODE_ALT5);

	/* Power up OV5642 using sequence in Freescale sample code */
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 17, GPIO_HIGH);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 16, GPIO_HIGH);
	mx6x_usleep(5000);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 16, GPIO_LOW);
	mx6x_usleep(5000);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 17, GPIO_LOW);
	mx6x_usleep(1000);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 17, GPIO_HIGH);
	mx6x_usleep(5000);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 16, GPIO_HIGH);
	mx6x_set_gpio_output(MX6X_GPIO1_BASE, 16, GPIO_LOW);


	pinmux_set_swmux(SWMUX_CSI0_PIXCLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_MCLK, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_VSYNC, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DATA_EN, MUX_CTL_MUX_MODE_ALT0);

	/* Data Lines */
	/* Sabre Smart only uses 8 data lines? */
	pinmux_set_swmux(SWMUX_CSI0_DAT12, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT13, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT14, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT15, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT16, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT17, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT18, MUX_CTL_MUX_MODE_ALT0);
	pinmux_set_swmux(SWMUX_CSI0_DAT19, MUX_CTL_MUX_MODE_ALT0);


	/* Route ADV7180 parallel video input to IPU's Camera Sensor Input (CSI) 0*/
	uint32_t gpr1;
	gpr1 = in32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR1);
	gpr1 &= ~MX6DQ_GPR1_MIPI_IPU1_MUX_MASK;
	gpr1 |= MX6DQ_GPR1_MIPI_IPU1_PARALLEL;
	out32(MX6X_IOMUXC_BASE + MX6X_IOMUX_GPR1, gpr1);
}



void mx6q_init_displays(void)
{
	mx6q_init_lcd_panel();
	mx6q_init_lvds();
	/* weilun@adv - start */
#if 0
	mx6q_init_mipi_camera();
	mx6q_init_parallel_camera();
#endif
	/* weilun@adv - end */
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/imx6x/smart-device/mx6q_gpio.c $ $Rev: 824103 $")
#endif

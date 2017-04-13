/*
 * $QNXLicenseC: 
 * Copyright 2012, QNX Software Systems.  
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



#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <hw/inout.h>
#include <arm/mx6x.h>
#include "imx_pwm.h"

#define	ONE_MHZ					1000000

/* Out-of-reset PFDs and clock source definitions */
#define MX6X_HCLK_FREQ			24000000
#define MX6X_PLL2_PFD0_FREQ  	352000000
#define MX6X_PLL2_PFD2_FREQ  	396000000
#define MX6X_PLL2_PFD2_DIV_FREQ 198000000
#define MX6X_PLL3_PFD1_FREQ  	540000000
#define MX6X_PLL3_80M    		80000000
#define MX6X_PLL3_60M    		60000000

#define MX6X_ENET_FREQ_0 		25000000
#define MX6X_ENET_FREQ_1 		50000000
#define MX6X_ENET_FREQ_2 		100000000
#define MX6X_ENET_FREQ_3 		125000000

/* enable spectrum */
#define PLL2_528_OFFSET		 	0x30
#define PLL_528_SS_OFFSET		0x10
#define PLL_528_DENOM_DIV_OFFSET	0x30
#define ANADIG_PLL_528_SYS_SS_ENABLE 	(1 << 15)

extern struct pwm_device *pwm_ptr;

/* Tried with denom=0x190, sys_ss=0xFA0001 and enabled=1 (which gives a spread spectrum range of 15 MHz, modulation frequency of 48 kHz and frequency step of 60 kHz). */
void mx6x_enable_spread_spectrum(uint32_t denom, uint32_t sys_ss) {

	uint32_t value;
	int enabled = 1;
	//uint32_t denom = 0x190;
	//uint32_t sys_ss = 0xfa0001;

	/* Disable spread spectrum mode */
	//__raw_writel((__raw_readl(anatop_base + PLL2_528_OFFSET + PLL_528_SS_OFFSET) & ~ANADIG_PLL_528_SYS_SS_ENABLE), anatop_base + PLL2_528_OFFSET +  PLL_528_SS_OFFSET);
	value = in32(pwm_ptr->ccm_analog_base + PLL2_528_OFFSET + PLL_528_SS_OFFSET);
	//printf("weilun@adv %s(), L:%d, value = 0x%x\n", __FUNCTION__, __LINE__, value); 
	value &= ~ANADIG_PLL_528_SYS_SS_ENABLE;
	out32(pwm_ptr->ccm_analog_base + PLL2_528_OFFSET + PLL_528_SS_OFFSET, value);

	/* debug */
	//value = in32(pwm_ptr->ccm_analog_base + PLL2_528_OFFSET + PLL_528_SS_OFFSET);
	//printf("weilun@adv %s(), L:%d, value = 0x%x\n", __FUNCTION__, __LINE__, value); 
	/* debug */
   
	/* Write new values */
	//__raw_writel(sys_ss, anatop_base + PLL2_528_OFFSET + PLL_528_SS_OFFSET);
	//__raw_writel(denom, anatop_base + PLL2_528_OFFSET + PLL_528_DENOM_DIV_OFFSET);
	out32(pwm_ptr->ccm_analog_base + PLL2_528_OFFSET + PLL_528_SS_OFFSET, sys_ss);
	out32(pwm_ptr->ccm_analog_base + PLL2_528_OFFSET + PLL_528_DENOM_DIV_OFFSET, denom);

	/* debug */
	//value = in32(pwm_ptr->ccm_analog_base + PLL2_528_OFFSET + PLL_528_SS_OFFSET);
	//printf("weilun@adv %s(), L:%d, value = 0x%x\n", __FUNCTION__, __LINE__, value); 
	value = in32(pwm_ptr->ccm_analog_base + PLL2_528_OFFSET + PLL_528_DENOM_DIV_OFFSET);
	//printf("%s(), denom = 0x%x\n", __FUNCTION__, value); 
	/* debug */

   
	/* Enable spread spectrum mode */
	if (enabled) {
       		 //__raw_writel((__raw_readl(anatop_base + PLL2_528_OFFSET + PLL_528_SS_OFFSET) | ANADIG_PLL_528_SYS_SS_ENABLE), anatop_base + PLL2_528_OFFSET +  PLL_528_SS_OFFSET);
       		uint32_t value2 = in32(pwm_ptr->ccm_analog_base + PLL2_528_OFFSET + PLL_528_SS_OFFSET);
		//printf("weilun@adv %s(), L:%d, value = 0x%x\n", __FUNCTION__, __LINE__, value2); 
		value2 |= ANADIG_PLL_528_SYS_SS_ENABLE;
		out32(pwm_ptr->ccm_analog_base + PLL2_528_OFFSET +  PLL_528_SS_OFFSET, value2);

		/* debug */
		value2 = in32(pwm_ptr->ccm_analog_base + PLL2_528_OFFSET + PLL_528_SS_OFFSET);
		printf("%s(0x%x, 0x%x) denom = 0x%x, sys_ss = 0x%x\n", __FUNCTION__, denom, sys_ss, value, value2); 
		/* debug */
	}
}
 
/* PLL1 - System */
static uint32_t mx6x_pll1_freq()
{
	uint32_t div = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL1_SYS) & 0x7F;
	return MX6X_HCLK_FREQ * (div >> 1);
}

/* PLL2 - System Bus */
static uint32_t mx6x_pll2_freq()
{
	uint32_t div = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL2_SYS_BUS) & 0x1;
	return MX6X_HCLK_FREQ * (20 + (div << 1));
}

/* PLL3 - USB OTG, PHY0 */
static uint32_t mx6x_pll3_freq()
{
	uint32_t div = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL3_USB1_OTG) & 0x3;
	return MX6X_HCLK_FREQ * (20 + (div << 1));
}

/* PLL4 - Audio */
static uint32_t mx6x_pll4_freq()
{
	uint32_t div = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL4_AUDIO) & 0x7f;
	uint32_t num = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL4_AUDIO_NUM);
	uint32_t denom = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL4_AUDIO_DENOM);

	return MX6X_HCLK_FREQ * (div + (num / denom));
}

/* PLL5 - Video */
static uint32_t mx6x_pll5_freq()
{
	uint32_t div = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL5_VIDEO) & 0x7f;
	uint32_t num = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL5_VIDEO_NUM);
	uint32_t denom = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL5_VIDEO_DENOM);

	return MX6X_HCLK_FREQ * (div + (num / denom));
}

/* PLL7 - USB PHY1 */
static uint32_t mx6x_pll7_freq()
{
	uint32_t div = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL7_USB2_HOST) & 0x3;
	return MX6X_HCLK_FREQ * (20 + (div << 1));
}

/* PLL8 - ENET */
static uint32_t mx6x_pll8_freq()
{
	uint32_t  freq = 0;
	uint32_t div = in32(pwm_ptr->ccm_analog_base + MX6X_ANATOP_PLL8_ENET) & 0x3;
	switch (div) {
		case 0: 
			freq = MX6X_ENET_FREQ_0;
			break;
		case 1: 
			freq = MX6X_ENET_FREQ_1;
			break;
		case 2: 
			freq = MX6X_ENET_FREQ_2;
			break;
		case 3: freq = MX6X_ENET_FREQ_3;
		break;
	}
	return freq;
}

uint32_t mx6x_get_cpu_clk()
{
	uint32_t div = in32(pwm_ptr->ccm_base + MX6X_CCM_CACCR) & 0x7;
	return  mx6x_pll1_freq() / (div + 1);
}

static uint32_t mx6x_get_periph_clk(void)
{
    uint32_t reg;
    reg = in32(pwm_ptr->ccm_base + MX6X_CCM_CBCDR);
    if (reg & MX6X_CCM_CBCDR_PERIPH_CLK_SEL) {
        reg = in32(pwm_ptr->ccm_base + MX6X_CCM_CBCMR);
        switch ((reg & MX6X_CCM_CBCMR_PERIPH_CLK2_SEL_MASK) >>
            MX6X_CCM_CBCMR_PERIPH_CLK2_SEL_OFFSET) {
        case 0:
            return mx6x_pll7_freq();
        case 1:
        case 2:
            return MX6X_HCLK_FREQ;
        default:
            return 0;
        }
    } else {
        reg = in32(pwm_ptr->ccm_base + MX6X_CCM_CBCMR);
        switch ((reg & MX6X_CCM_CBCMR_PRE_PERIPH_CLK_SEL_MASK) >>
            MX6X_CCM_CBCMR_PRE_PERIPH_CLK_SEL_OFFSET) {
        default:
        case 0:
            return mx6x_pll2_freq();
        case 1:
            return MX6X_PLL2_PFD2_FREQ;
        case 2:
            return MX6X_PLL2_PFD0_FREQ;
        case 3:
            return MX6X_PLL2_PFD2_DIV_FREQ;
        }
    }
}

static uint32_t mx6x_get_ipg_clk(void)
{
    uint32_t ahb_podf, ipg_podf;

    ahb_podf = in32(pwm_ptr->ccm_base + MX6X_CCM_CBCDR);
    ipg_podf = (ahb_podf & MX6X_CCM_CBCDR_IPG_PODF_MASK) >>
            MX6X_CCM_CBCDR_IPG_PODF_OFFSET;
    ahb_podf = (ahb_podf & MX6X_CCM_CBCDR_AHB_PODF_MASK) >>
            MX6X_CCM_CBCDR_AHB_PODF_OFFSET;
    return mx6x_get_periph_clk() / ((ahb_podf + 1) * (ipg_podf + 1));
}

static uint32_t mx6x_get_ipg_per_clk(void)
{
    uint32_t podf;
    uint32_t clk_root = mx6x_get_ipg_clk();

    podf = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCMR1) & MX6X_CCM_CSCMR1_PERCLK_PODF_MASK;
    return clk_root / (podf + 1);
}

static uint32_t mx6x_get_uart_clk(void)
{
    uint32_t freq = MX6X_PLL3_80M, reg, podf;

    reg = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCDR1);
    podf = (reg & MX6X_CCM_CSCDR1_UART_CLK_PODF_MASK) >>
        MX6X_CCM_CSCDR1_UART_CLK_PODF_OFFSET;
    freq /= (podf + 1);

    return freq;
}


static uint32_t mx6x_get_cspi_clk(void)
{
    uint32_t freq = MX6X_PLL3_60M, reg, podf;

    reg = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCDR2);
    podf = (reg & MX6X_CCM_CSCDR2_ECSPI_CLK_PODF_MASK) >>
        MX6X_CCM_CSCDR2_ECSPI_CLK_PODF_OFFSET;
    freq /= (podf + 1);

    return freq;
}

static uint32_t mx6x_get_ahb_clk(void)
{
    uint32_t cbcdr =  in32(pwm_ptr->ccm_base + MX6X_CCM_CBCDR);
    uint32_t podf = (cbcdr & MX6X_CCM_CBCDR_AHB_PODF_MASK) \
            >> MX6X_CCM_CBCDR_AHB_PODF_OFFSET;

    return  mx6x_get_periph_clk() / (podf + 1);
}

static uint32_t mx6x_get_axi_clk(void)
{
    uint32_t clkroot;
    uint32_t cbcdr =  in32(pwm_ptr->ccm_base + MX6X_CCM_CBCDR);
    uint32_t podf = (cbcdr & MX6X_CCM_CBCDR_AXI_PODF_MASK) >>
        MX6X_CCM_CBCDR_AXI_PODF_OFFSET;

    if (cbcdr & MX6X_CCM_CBCDR_AXI_SEL) {
        if (cbcdr & MX6X_CCM_CBCDR_AXI_ALT_SEL)
            clkroot = MX6X_PLL3_PFD1_FREQ;
        else
            clkroot = MX6X_PLL2_PFD2_FREQ;
    } else
        clkroot = mx6x_get_periph_clk();

    return  clkroot / (podf + 1);
}

static uint32_t mx6x_get_emi_slow_clk(void)
{
    uint32_t cscmr1 =  in32(pwm_ptr->ccm_base + MX6X_CCM_CSCMR1);
    uint32_t emi_clk_sel = (cscmr1 & MX6X_CCM_CSCMR1_ACLK_EMI_SLOW_MASK) >>
        MX6X_CCM_CSCMR1_ACLK_EMI_SLOW_OFFSET;
    uint32_t podf = (cscmr1 & MX6X_CCM_CSCMR1_ACLK_EMI_SLOW_PODF_MASK) >>
            MX6X_CCM_CSCMR1_ACLK_EMI_PODF_OFFSET;

    switch (emi_clk_sel) {
    default:
    case 0:
        return mx6x_get_axi_clk() / (podf + 1);
    case 1:
        return mx6x_pll7_freq() / (podf + 1);
    case 2:
        return MX6X_PLL2_PFD2_FREQ / (podf + 1);
    case 3:
        return MX6X_PLL2_PFD0_FREQ / (podf + 1);
    }
}

static uint32_t mx6x_get_ddr_clk(void)
{
    uint32_t cbcdr = in32(pwm_ptr->ccm_base + MX6X_CCM_CBCDR);
    uint32_t podf = (cbcdr & MX6X_CCM_CBCDR_MMDC_CH0_PODF_MASK) >>
        MX6X_CCM_CBCDR_MMDC_CH0_PODF_OFFSET;

    return mx6x_get_periph_clk() / (podf + 1);
}

static uint32_t mx6x_get_usdhc1_clk()
{
    uint32_t clkroot;
    uint32_t cscmr1 = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCMR1);
    uint32_t cscdr1 = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCDR1);
    uint32_t podf = (cscdr1 & MX6X_CCM_CSCDR1_USDHC1_PODF_MASK) >>
        MX6X_CCM_CSCDR1_USDHC1_PODF_OFFSET;

    if (cscmr1 & MX6X_CCM_CSCMR1_USDHC1_CLK_SEL)
        clkroot = MX6X_PLL2_PFD0_FREQ;
    else
        clkroot = MX6X_PLL2_PFD2_FREQ;

    return clkroot / (podf + 1);
}

static uint32_t mx6x_get_usdhc2_clk(void)
{
    uint32_t clkroot;
    uint32_t cscmr1 = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCMR1);
    uint32_t cscdr1 = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCDR1);
    uint32_t podf = (cscdr1 & MX6X_CCM_CSCDR1_USDHC2_PODF_MASK) >>
        MX6X_CCM_CSCDR1_USDHC2_PODF_OFFSET;

    if (cscmr1 & MX6X_CCM_CSCMR1_USDHC2_CLK_SEL)
        clkroot = MX6X_PLL2_PFD0_FREQ;
    else
        clkroot = MX6X_PLL2_PFD2_FREQ;

    return clkroot / (podf + 1);
}
static uint32_t mx6x_get_usdhc3_clk(void)
{
    uint32_t clkroot;
    uint32_t cscmr1 = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCMR1);
    uint32_t cscdr1 = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCDR1);
    uint32_t podf = (cscdr1 & MX6X_CCM_CSCDR1_USDHC3_PODF_MASK) >>
        MX6X_CCM_CSCDR1_USDHC3_PODF_OFFSET;

    if (cscmr1 & MX6X_CCM_CSCMR1_USDHC3_CLK_SEL)
        clkroot = MX6X_PLL2_PFD0_FREQ;
    else
        clkroot = MX6X_PLL2_PFD2_FREQ;

    return clkroot / (podf + 1);
}

static uint32_t mx6x_get_usdhc4_clk(void)
{
    uint32_t clkroot;
    uint32_t cscmr1 = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCMR1);
    uint32_t cscdr1 = in32(pwm_ptr->ccm_base + MX6X_CCM_CSCDR1);
    uint32_t podf = (cscdr1 & MX6X_CCM_CSCDR1_USDHC4_PODF_MASK) >>
        MX6X_CCM_CSCDR1_USDHC4_PODF_OFFSET;

    if (cscmr1 & MX6X_CCM_CSCMR1_USDHC4_CLK_SEL)
        clkroot = MX6X_PLL2_PFD0_FREQ;
    else
        clkroot = MX6X_PLL2_PFD2_FREQ;

    return clkroot / (podf + 1);
}

static uint32_t mx6x_get_enfc_clk(void)
{
    uint32_t clkroot;
    uint32_t cs2cdr =  in32(pwm_ptr->ccm_base + MX6X_CCM_CS2CDR);
    uint32_t podf = (cs2cdr & MX6X_CCM_CS2CDR_ENFC_CLK_PODF_MASK) >> MX6X_CCM_CS2CDR_ENFC_CLK_PODF_OFFSET;
    uint32_t pred = (cs2cdr & MX6X_CCM_CS2CDR_ENFC_CLK_PRED_MASK) >> MX6X_CCM_CS2CDR_ENFC_CLK_PRED_OFFSET;

    switch ((cs2cdr & MX6X_CCM_CS2CDR_ENFC_CLK_SEL_MASK) >>
        MX6X_CCM_CS2CDR_ENFC_CLK_SEL_OFFSET) {
        default:
        case 0:
            clkroot = MX6X_PLL2_PFD0_FREQ;
            break;
        case 1:
            clkroot = mx6x_pll2_freq();
            break;
        case 2:
            clkroot = mx6x_pll7_freq();
            break;
        case 3:
            clkroot = MX6X_PLL2_PFD2_FREQ;
            break;
    }

    return  clkroot / (pred+1) / (podf+1);
}

int debug_flag = 1;

void mx6x_dump_clocks(void)
{
	if (debug_flag) {
		printf("PLL1 - System     : %dMHz\n", mx6x_pll1_freq() / ONE_MHZ);
		printf("PLL2 - System Bus : %dMHz\n", mx6x_pll2_freq() / ONE_MHZ);
		printf("PLL3 - OTG USB    : %dMHz\n", mx6x_pll3_freq() / ONE_MHZ);
		printf("PLL4 - Audio      : %dMHz\n", mx6x_pll4_freq() / ONE_MHZ);
		printf("PLL5 - Video      : %dMHz\n", mx6x_pll5_freq() / ONE_MHZ);
		printf("PLL7 - Host USB   : %dMHz\n", mx6x_pll7_freq() / ONE_MHZ);
		printf("PLL8 - Enet       : %dMHz\n", mx6x_pll8_freq() / ONE_MHZ);
		printf("IPG clock     : %dHz\n", mx6x_get_ipg_clk());
		printf("IPG per clock : %dHz\n", mx6x_get_ipg_per_clk());
		printf("UART clock    : %dHz\n", mx6x_get_uart_clk());
		printf("CSPI clock    : %dHz\n", mx6x_get_cspi_clk());
		printf("AHB clock     : %dHz\n", mx6x_get_ahb_clk());
		printf("AXI clock     : %dHz\n", mx6x_get_axi_clk());
		printf("EMI_SLOW clock: %dHz\n", mx6x_get_emi_slow_clk());
		printf("DDR clock     : %dHz\n", mx6x_get_ddr_clk());
		printf("USDHC1 clock  : %dHz\n", mx6x_get_usdhc1_clk());
		printf("USDHC2 clock  : %dHz\n", mx6x_get_usdhc2_clk());
		printf("USDHC3 clock  : %dHz\n", mx6x_get_usdhc3_clk());
		printf("USDHC4 clock  : %dHz\n", mx6x_get_usdhc4_clk());
		printf("ENFC clock    : %dHz\n", mx6x_get_enfc_clk());
	}
}

uint32_t mxc_get_clock(void)
{
	return mx6x_get_ipg_per_clk();
}



/*
 * Porting to u-boot:
 * Linux IMX PWM driver
 *
 * Copyright (C) 2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <gulliver.h>
#include <sys/neutrino.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <malloc.h>
#include <hw/inout.h>
#include <arm/mx6x.h>	// Global header for iMX6x SoC

#include "div64.h"
#include "mx6.h"
#include "imx_pwm.h"

#define IMX_PWM1_BASE    PWM1_BASE_ADDR
#define IMX_PWM2_BASE    PWM2_BASE_ADDR

#define CONFIG_MX6_HCLK_FREQ	24000000

#define MX1_PWMC    0x00   /* PWM Control Register */
#define MX1_PWMS    0x04   /* PWM Sample Register */
#define MX1_PWMP    0x08   /* PWM Period Register */

#define MX1_PWMC_EN		(1 << 4)

/* i.MX27, i.MX31, i.MX35 share the same PWM function block: */

#define MX3_PWMCR       	        0x00    /* PWM Control Register */
#define MX3_PWMSR			0x04    /* PWM Status Register */
#define MX3_PWMSAR             		0x0C    /* PWM Sample Register */
#define MX3_PWMPR                 	0x10    /* PWM Period Register */
#define MX3_PWMCR_PRESCALER(x)    	(((x - 1) & 0xFFF) << 4)
#define MX3_PWMCR_DOZEEN                (1 << 24)
#define MX3_PWMCR_WAITEN                (1 << 23)
#define MX3_PWMCR_DBGEN			(1 << 22)
#define MX3_PWMCR_CLKSRC_IPG_HIGH 	(2 << 16)
#define MX3_PWMCR_CLKSRC_IPG      	(1 << 16)
#define MX3_PWMCR_SWR			(1 << 3)
#define MX3_PWMCR_EN              	(1 << 0)
#define MX3_PWMSR_FIFOAV_4WORDS		0x4
#define MX3_PWMSR_FIFOAV_MASK		0x7

#define MX3_PWM_SWR_LOOP		5

#define MX6X_PWM_SIZE			0x4000

static struct pwm_device pwm0 = {
#if defined(CONFIG_MACH_MX6Q_RSB_4410)
	.pwm_id = 1,
#else
	.pwm_id = 0,
#endif
	.lth_brightness = 0,
	//.max_brightness = 7,
	//.dft_brightness = 7,
	.max_brightness = 16,
	.dft_brightness = 16,
	.pwm_period_ns = 5000000,
};

//static unsigned int brightness_levels[] = {0, 4, 8, 16, 32, 64, 128, 255};
static unsigned int brightness_levels[] = {0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255};

struct pwm_device *pwm_ptr = &pwm0;

extern void mx6x_enable_spread_spectrum(uint32_t denom, uint32_t sys_ss);

static int imx_pwm_config_v2(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	unsigned long long c;
	unsigned long period_cycles, duty_cycles, prescale;
	//unsigned int period_ms;
	//int enable = test_bit(PWMF_ENABLED, &pwm->flags);
	int enable = 1;
	int wait_count = 0, fifoav;
	unsigned int cr, sr;

	/*
	 * i.MX PWMv2 has a 4-word sample FIFO.
	 * In order to avoid FIFO overflow issue, we do software reset
	 * to clear all sample FIFO if the controller is disabled or
	 * wait for a full PWM cycle to get a relinquished FIFO slot
	 * when the controller is enabled and the FIFO is fully loaded.
	 */
	if (enable) {
		sr = in32(pwm_ptr->mmio_base + MX3_PWMSR);
		fifoav = sr & MX3_PWMSR_FIFOAV_MASK;
		if (fifoav == MX3_PWMSR_FIFOAV_4WORDS) {
			//period_ms = DIV_ROUND_UP(pwm->period, NSEC_PER_MSEC);
			//msleep(period_ms);
			usleep(1000);

			sr = in32(pwm_ptr->mmio_base + MX3_PWMSR);
			if (fifoav == (sr & MX3_PWMSR_FIFOAV_MASK))
				printf("there is no free FIFO slot\n");
		}
	} else {
		out32(MX3_PWMCR_SWR, pwm_ptr->mmio_base + MX3_PWMCR);
		do {
			//usleep_range(200, 1000);
			usleep(1000);
			cr = in32(pwm_ptr->mmio_base + MX3_PWMCR);
		} while ((cr & MX3_PWMCR_SWR) &&
			 (wait_count++ < MX3_PWM_SWR_LOOP));

		if (cr & MX3_PWMCR_SWR)
			printf("software reset timeout\n");
	}

	c = mxc_get_clock();
	c = c * period_ns;
	do_div(c, 1000000000);
	period_cycles = c;

	prescale = period_cycles / 0x10000 + 1;

	period_cycles /= prescale;
	c = (unsigned long long)period_cycles * duty_ns;
	do_div(c, period_ns);
	duty_cycles = c;

	/*
	 * according to imx pwm RM, the real period value should be
	 * PERIOD value in PWMPR plus 2.
	 */
	if (period_cycles > 2)
		period_cycles -= 2;
	else
		period_cycles = 0;

	out32(pwm_ptr->mmio_base + MX3_PWMSAR, duty_cycles);
	out32(pwm_ptr->mmio_base + MX3_PWMPR, period_cycles);

	cr = MX3_PWMCR_PRESCALER(prescale) |
		MX3_PWMCR_DOZEEN | MX3_PWMCR_WAITEN |
		MX3_PWMCR_DBGEN | MX3_PWMCR_CLKSRC_IPG_HIGH;

	if (enable)
		cr |= MX3_PWMCR_EN;

	out32(pwm_ptr->mmio_base + MX3_PWMCR, cr);

	return 0;
}

int imx_pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	unsigned long long c;
	unsigned long period_cycles, duty_cycles, prescale;
	unsigned int cr;

	c = mxc_get_clock();

	c = c * period_ns;

	do_div(c, 1000000000);
	period_cycles = c;

	prescale = period_cycles / 0x10000 + 1;

	period_cycles /= prescale;

	c = (unsigned long long)period_cycles * duty_ns;

	do_div(c, period_ns);

	duty_cycles = c;

	/*
	 * according to imx pwm RM, the real period value should be
	 * PERIOD value in PWMPR plus 2.
	 */
	if (period_cycles > 2)
		period_cycles -= 2;
	else
		period_cycles = 0;

	out32(pwm->mmio_base + MX3_PWMSAR, duty_cycles);
	out32(pwm->mmio_base + MX3_PWMPR, period_cycles);

	cr = MX3_PWMCR_PRESCALER(prescale) |
		MX3_PWMCR_DOZEEN | MX3_PWMCR_WAITEN |
		MX3_PWMCR_DBGEN | MX3_PWMCR_CLKSRC_IPG_HIGH;

	//if (test_bit(PWMF_ENABLED, &pwm->flags))
		cr |= MX3_PWMCR_EN;

	printf("%s(%d,%d) duty_cycles = %ld, period_cycles = %ld, cr = 0x%x\n", __FUNCTION__, duty_ns, period_ns, duty_cycles, period_cycles, cr);
	out32(pwm->mmio_base + MX3_PWMCR, cr);

	return 0;
}

void imx_pwm_enable(struct pwm_device *pwm, int enable)
{
	unsigned long val;

	val = in32(pwm->mmio_base + MX3_PWMCR);

	if (enable)
		val |= MX3_PWMCR_EN;
	else
		val &= ~MX3_PWMCR_EN;

	out32(pwm->mmio_base + MX3_PWMCR, val);

	//printf("%s(%d), val = 0x%lx \n", __FUNCTION__, enable, val);

}

int pwm_backlight_update_status(int brightness)
{
	int max = pwm_ptr->max_brightness;

	if (brightness == 0) {
		imx_pwm_config(pwm_ptr, 0, pwm_ptr->pwm_period_ns);
		imx_pwm_enable(pwm_ptr, 0);
	} 
	else {

		int duty_cycle;

		if (pwm_ptr->levels) {
			duty_cycle = pwm_ptr->levels[brightness];
			max = pwm_ptr->levels[max];
		} else {
			duty_cycle = brightness;
		}

		duty_cycle = pwm_ptr->lth_brightness +
			(duty_cycle * (pwm_ptr->pwm_period_ns - pwm_ptr->lth_brightness) / max);
		imx_pwm_config(pwm_ptr, duty_cycle, pwm_ptr->pwm_period_ns);
		imx_pwm_enable(pwm_ptr, 1);
	}

	return 0;
}

int imx_enable_spread_spectrum() {
	uint32_t denom = 0x190;
	uint32_t sys_ss = 0xfa0001;
	mx6x_enable_spread_spectrum(denom, sys_ss);
	return 0;
}

int imx_pwm_init()
{
	unsigned long mmio_base;

	if (-1 == ThreadCtl(_NTO_TCTL_IO, 0)) {
        	perror("ThreadCtl");
        	return -1;
    	}

	mmio_base = pwm_ptr->pwm_id ? (unsigned long)IMX_PWM2_BASE:
				(unsigned long)IMX_PWM1_BASE;

	// PWM
	pwm_ptr->mmio_base = mmap_device_io(MX6X_PWM_SIZE, mmio_base);
    	if (pwm_ptr->mmio_base == (uintptr_t)MAP_FAILED) {
        	perror("mmap_device_io");
        	return -1;
    	}

	// CCM
	pwm_ptr->ccm_base = mmap_device_io(MX6X_CCM_SIZE, MX6X_CCM_BASE);
	if ( pwm_ptr->ccm_base == (uintptr_t) MAP_FAILED )
	{
		perror ("Unable to mmap CCM: ");
		return (ENODEV);
	}

	// CCM Analogue + PMU
	pwm_ptr->ccm_analog_base = mmap_device_io(MX6X_ANATOP_SIZE, MX6X_ANATOP_BASE);
	if ( pwm_ptr->ccm_analog_base == (uintptr_t) MAP_FAILED )
	{
		perror ("Unable to mmap CCM analog: ");
		return (ENODEV);
	}


   	//mx6x_dump_clocks();

	if (pwm_ptr->max_brightness > 0) {
		pwm_ptr->levels = brightness_levels;
	}

	pwm_ptr->lth_brightness = pwm_ptr->lth_brightness *
		(pwm_ptr->pwm_period_ns / pwm_ptr->max_brightness);

	return 0;
}



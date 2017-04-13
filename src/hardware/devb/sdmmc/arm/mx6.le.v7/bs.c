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

// Module Description: imx6 generic board specific interface

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <arm/mx6x.h>
#include <imx6.h>
#include <bs.h>

/* Update write protect status */
static void imx6_update_wp_status(sdio_hc_t *hc, int *cstate)
{
	imx6_sdhcx_hc_t		*sdhc;
	imx6_ext_t		*ext;
	uint32_t		status;
	uintptr_t		base;

	sdhc = (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base = sdhc->base;
	ext = (imx6_ext_t *)hc->bs_hdl;

	/* Write protect using GPIO */
	if (ext->wp_pbase) {
		if (imx6_sdhcx_in32(ext->wp_base + MX6X_GPIO_PSR) & (1 << ext->wp_pin)) {
			*cstate |= CD_WP;
		}
		else {
			*cstate &= ~CD_WP;
		}
	}

	/* Check write protect using USDHC PSTATE register */
	else {
		status = imx6_sdhcx_in32(base + IMX6_SDHCX_PSTATE);

		/* When WPSPL bit is 0 write protect is enabled */
		if (!(status & IMX6_SDHCX_PSTATE_WP)) {
			*cstate |= CD_WP;
		} else {
			*cstate &= ~CD_WP;
		}
	}
}

/* Handle CD/WP if no card detection, or through uSDHC interrupt */
static int imx6_cd( sdio_hc_t *hc )
{
	imx6_sdhcx_hc_t		*sdhc;
	imx6_ext_t		*ext;
	uint32_t		status;
	uintptr_t		base;
	int			cstate = CD_RMV;

	sdhc = (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base = sdhc->base;
	ext = (imx6_ext_t *)hc->bs_hdl;

	/* Assuming card is always inserted */
	if (ext->nocd)
		return CD_INS;

	/* SDx_CD and SDx_WP pins are connected so that uSDHCx_PRES_STATE register can tell CD/WP status */
	status = imx6_sdhcx_in32(base + IMX6_SDHCX_PSTATE);

	if (status & IMX6_SDHCX_PSTATE_CD) {
		cstate |= CD_INS;
		imx6_update_wp_status(hc, &cstate);
	}
	return (cstate);
}

/* CD/WP through GPIO pins, interrupt may or may not be used */
static int imx6_gpio_cd(sdio_hc_t *hc)
{
	imx6_ext_t	*ext = (imx6_ext_t *)hc->bs_hdl;
	int cstate = CD_RMV;
	unsigned val;

	/*
	 * CD pin low to indicate card inserted
	 * This may not be true for all targets, so another
	 * "cdpolarity" option may be needed
	 */
	if (!(imx6_sdhcx_in32(ext->cd_base + MX6X_GPIO_PSR) & (1 << ext->cd_pin))) {
		cstate |= CD_INS;

		if (ext->cd_irq) {
			/* clear GPIO interrupt */
			imx6_sdhcx_out32(ext->cd_base + MX6X_GPIO_ISR, (1 << ext->cd_pin));

			/* change gpio interrupt to rising edge sensitive to catch card removal event */
			if (ext->cd_pin < 16) {
				val = imx6_sdhcx_in32(ext->cd_base + MX6X_GPIO_ICR1) & ~(0x3 << (2 * ext->cd_pin));
				val |= (0x2 << (2 * ext->cd_pin));
				imx6_sdhcx_out32(ext->cd_base + MX6X_GPIO_ICR1, val);
			} else {
				val = imx6_sdhcx_in32(ext->cd_base + MX6X_GPIO_ICR2) & ~(0x3 << (2 * (ext->cd_pin - 16)));
				val |= (0x2 << (2 * (ext->cd_pin - 16)));
				imx6_sdhcx_out32(ext->cd_base + MX6X_GPIO_ICR2, val);
			}

			InterruptUnmask( ext->cd_irq, ext->cd_iid );
		}
		imx6_update_wp_status(hc, &cstate);
	} else {
		if (ext->cd_irq) {
			/* card removed, reconfigure gpio interrupt to falling edge sensitive */
			if (ext->cd_pin < 16) {
				val = imx6_sdhcx_in32(ext->cd_base + MX6X_GPIO_ICR1) | (0x3 << (2 * ext->cd_pin));
				imx6_sdhcx_out32(ext->cd_base + MX6X_GPIO_ICR1, val);
			} else {
				val = imx6_sdhcx_in32(ext->cd_base + MX6X_GPIO_ICR2) | (0x3 << (2 * (ext->cd_pin - 16)));
				imx6_sdhcx_out32(ext->cd_base + MX6X_GPIO_ICR2, val);
			}
			InterruptUnmask( ext->cd_irq, ext->cd_iid );
		}
	}

	return cstate;
}

/* Use ":" as separator, other than the regular "," */
int my_getsubopt(char **optionp, char * const *tokens, char **valuep) {
	char		*p, *opt;
	int			len, index;
	const char	*token;

	*valuep = 0;

	for(opt = p = *optionp, len = 0; *p && *p != ':'; p++, len++) {
		if(*p == '=') {
			for(*valuep = ++p; *p && *p != ':'; p++) {
				/* nothing to do */
			}
			break;
		}
	}
	if(*p) {
		*p++ = '\0';
	}
	*optionp = p;

	for(index = 0; (token = *tokens++); index++) {
		if(*token == *opt && !strncmp(token, opt, len) && token[len] == '\0') {
			return index;
		}
	}
	*valuep = opt;
	return -1;
}

/*
 * Board Specific options can be passed through command line arguments or syspage optstr attribute,
 * but the syspage way is recommended since it can pass different options to different uSDHC contollers
 * Example of the BS options: bs=cd_irq=165:cd_base=0x0209C000:cd_pin=5:vdd1_8
 *		-- CD pin is GPIO1[5] (GPIO1 Base: 0x0209C000)
 *		-- IRQ for GPIO1[5] is 165
 *		-- 1.8v is supported
 * Also please note that the optstr passed from syspage can be overwritten by the SDMMC command line arguments
 *
 * Notes:
 *		CD_BASE=base, CD_PIN=pin, CD_IRQ=irq can be replaced by one single option: CD=base^pin^irq
 *		WP_BASE=base, WP_PIN=pin can be replaced by one single option: WP=base^pin
 * For example:
 *		cd=0x020a0000^0^192 is equavalent to cd_base=0x020a0000:cd_pin=0:cd_irq=192
 *		wp=0x020a0000^1 is equavalent to wp_base=0x020a0000:wp_pin=1
 */
static int imx6_bs_args(sdio_hc_t *hc, char *options)
{
	char		*value;
	int			opt;
	imx6_ext_t	*ext = (imx6_ext_t *)hc->bs_hdl;

	/* Default values */
	ext->nocd = 0;
	ext->cd_irq = 0;
	ext->cd_pbase = 0;
	ext->cd_pin = 0;
	ext->wp_pbase = 0;
	ext->wp_pin = 0;
	ext->bw = 4;
	ext->vdd1_8 = 0;

	static char	 *opts[] = {
#define	EMMC		0
			"emmc",		/* implies "nocd" option is set as well */
#define	NOCD		1
			"nocd",		/* no card detection capability */
#define	CD_IRQ		2
			"cd_irq",	/* Interrupt vector for GPIO CD pin */
#define	CD_BASE		3
			"cd_base",	/* GPIO base address for the CD pin */
#define	CD_PIN		4
			"cd_pin",
#define CD			5	/* This option covers CD_IRQ, CD_BASE, CD_PIN */
			"cd",
#define	WP_BASE		6
			"wp_base",	/* GPIO base address for the WP pin */
#define	WP_PIN		7
			"wp_pin",
#define WP			8	/* This option covers WP_BASE, WP_PIN */
			"wp",
#define	BW			9
			"bw",		/* data bus width */
#define	VDD1_8		10
			"vdd1_8",	/* 1.8v support capability */
			NULL };

	while (options && *options != '\0') {
		if ((opt = my_getsubopt( &options, opts, &value)) == -1) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 0, "%s: invalid BS options %s", __func__, options);
			return EINVAL;
		}

		switch (opt) {
			case EMMC:
				hc->caps |= HC_CAP_SLOT_TYPE_EMBEDDED;
				hc->flags |= HC_FLAG_DEV_MMC;
				ext->vdd1_8 = 1;
			case NOCD:
				ext->nocd = 1;
				break;
			case CD_IRQ:
				ext->cd_irq = strtol(value, 0, 0);
				break;
			case CD_BASE:
				ext->cd_pbase = strtol(value, 0, 0);
				break;
			case CD_PIN:
				ext->cd_pin = strtol(value, 0, 0);
				break;
			case CD:
				ext->cd_pbase = strtol(value, &value, 0);
				if (*value == '^') {
					ext->cd_pin = strtol(value + 1, &value, 0);
				}
				if (*value == '^') {
					ext->cd_irq = strtol(value + 1, &value, 0);
				}
				break;
			case WP_BASE:
				ext->wp_pbase = strtol(value, 0, 0);
				break;
			case WP_PIN:
				ext->wp_pin = strtol(value, 0, 0);
				break;
			case WP:
				ext->wp_pbase = strtol(value, &value, 0);
				if (*value == '^') {
					ext->wp_pin = strtol(value + 1, &value, 0);
				}
				break;
			case BW:
				ext->bw = strtol(value, 0, 0);
				break;
			case VDD1_8:
				ext->vdd1_8 = 1;
				break;
			default:
				break;
		}
	}

	return EOK;
}

static int imx6_dinit( sdio_hc_t *hc )
{
	imx6_ext_t *ext = (imx6_ext_t *)hc->bs_hdl;

	if (ext) {
		if (ext->cd_iid != -1) {
			InterruptDetach (ext->cd_iid);
		}

		if (ext->cd_base != MAP_DEVICE_FAILED)
			munmap_device_io (ext->cd_base, MX6X_GPIO_SIZE);

		if (ext->wp_base != MAP_DEVICE_FAILED)
			munmap_device_io (ext->wp_base, MX6X_GPIO_SIZE);

		free( ext );
	}

	return (imx6_sdhcx_dinit(hc));
}

static int imx6_init(sdio_hc_t *hc) {

	imx6_ext_t	*ext;
	sdio_hc_cfg_t	*cfg = &hc->cfg;
	int	status;

	if (!(ext = calloc(1, sizeof(imx6_ext_t))))
		return ENOMEM;

	/* By default 1.8V is not enabled */
	hc->bs_hdl	= ext;
	memset(ext, 0, sizeof(imx6_ext_t));
	ext->cd_base = MAP_DEVICE_FAILED;
	ext->wp_base = MAP_DEVICE_FAILED;
	ext->cd_iid = -1;

	if (imx6_bs_args(hc, cfg->options))
		return EINVAL;

	if (ext->cd_pbase) {
		if (MAP_DEVICE_FAILED == (ext->cd_base = mmap_device_io(MX6X_GPIO_SIZE, ext->cd_pbase))) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 0, "%s: GPIO mmap_device_io failed.", __func__);
			return ENOMEM;
		}
	}

	if (ext->wp_pbase) {
		if (MAP_DEVICE_FAILED == (ext->wp_base = mmap_device_io(MX6X_GPIO_SIZE, ext->wp_pbase))) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 0, "%s: GPIO mmap_device_io failed.", __func__);
			imx6_dinit(hc);
			return ENOMEM;
		}
	}

	if (EOK != (status = imx6_sdhcx_init(hc))) {
		imx6_dinit(hc);
		return status;
	}

	if (ext->cd_irq) {
		struct sigevent	event;

		SIGEV_PULSE_INIT(&event, hc->hc_coid, SDIO_PRIORITY, HC_EV_CD, NULL);

		/* Disable and clear interrupt status */
		imx6_sdhcx_out32(ext->cd_base + MX6X_GPIO_IMR, imx6_sdhcx_in32(ext->cd_base + MX6X_GPIO_IMR) & ~(1 << ext->cd_pin));
		imx6_sdhcx_out32(ext->cd_base + MX6X_GPIO_ISR, (1 << ext->cd_pin));

		if (-1 == (ext->cd_iid = InterruptAttachEvent(ext->cd_irq, &event, _NTO_INTR_FLAGS_TRK_MSK))) {
			sdio_slogf(_SLOGC_SDIODI, _SLOG_ERROR, 1, 0, "%s: InterruptAttachEvent(%d) failure - %s", __FUNCTION__, ext->cd_irq, strerror(errno));
			imx6_dinit(hc);
			return errno;
		}

		hc->caps |= HC_CAP_CD_INTR;
	}

	/*
	 * overwrite cd/dinit functions
	 * by default, CD is detected through uSDHCx_PRES_STATE register and interrupt (HC_CAP_CD_INTR is set)
	 */
	hc->entry.dinit = imx6_dinit;
	if (ext->cd_pbase)
		hc->entry.cd = imx6_gpio_cd;
	else
		hc->entry.cd = imx6_cd;

	/* Overwrite some of the capabilities that are set by imx6_sdhcx_init() */
	if (ext->nocd)
		hc->caps &= ~HC_CAP_CD_INTR;

	/* "bs=vdd1_8" must be set in order to enable 1.8v operations */
	if (!ext->vdd1_8) {
		hc->ocr		&= ~OCR_VDD_17_195;
		hc->caps	&= ~HC_CAP_SV_1_8V;
		hc->caps	&= ~( HC_CAP_SDR12 | HC_CAP_SDR25 | HC_CAP_SDR50 | HC_CAP_SDR104 | HC_CAP_DDR50 | HC_CAP_HS200 );
	} else {
		hc->ocr		|= OCR_VDD_17_195;
	}

	if (ext->bw == 8) {
		hc->caps |= (HC_CAP_BW4 | HC_CAP_BW8);
	} else if (ext->bw == 4) {
		hc->caps |= HC_CAP_BW4;
		hc->caps &= ~HC_CAP_BW8;
	} else if (ext->bw == 1) {
		hc->caps &= ~(HC_CAP_BW4 | HC_CAP_BW8);
	}

	/* Enable GPIO interrupt */
	if (ext->cd_irq)
		imx6_sdhcx_out32(ext->cd_base + MX6X_GPIO_IMR, imx6_sdhcx_in32(ext->cd_base + MX6X_GPIO_IMR) | (1 << ext->cd_pin));

	return EOK;
}

static int mx6_cd_event(sdio_hc_t *hc) {
	sdio_hc_event(hc, HC_EV_CD);
	return EOK;
}

int bs_event(sdio_hc_t *hc, sdio_event_t *ev) {
	int	status;

	switch(ev->code) {
		case HC_EV_CD:
			status = mx6_cd_event(hc);
			break;

		default:
			status = ENOTSUP;
			break;
	}

	return status;
}

sdio_product_t	sdio_fs_products[] = {
	{ SDIO_DEVICE_ID_WILDCARD, 0, 0, "imx6", imx6_init },
};

sdio_vendor_t	sdio_vendors[] = {
	{ SDIO_VENDOR_ID_WILDCARD, "Freescale", sdio_fs_products },
	{ 0, NULL, NULL }
};


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/arm/mx6.le.v7/bs.c $ $Rev: 819326 $")
#endif

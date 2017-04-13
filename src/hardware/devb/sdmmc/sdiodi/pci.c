/*
 * $QNXLicenseC:
 * Copyright 2007, 2008, QNX Software Systems.
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

// Module Description:   PCI interface

#include <errno.h>
#include <string.h>

#include <internal.h>

#ifdef SDIO_PCI_SUPPORT

#include <pci/pci.h>
#include <pci/pci_ccode.h>

int sdio_pci_init( )
{
	return( EOK );
}

int sdio_pci_dinit( )
{
	return( EOK );
}

int sdio_pci_detach_device( sdio_hc_t *hc )
{
	if (hc->pci.dev_hdl != NULL)
	{
		pci_err_t pci_err = pci_device_detach(hc->pci.dev_hdl);
		if (pci_err != PCI_ERR_OK)
		{
			sdio_slogf(_SLOGC_SDIODI, _SLOG_ERROR,
				   hc->cfg.verbosity, 2,
				   "%s: Failure to detach device: %s",
				   __FUNCTION__, pci_strerror(pci_err));
			return pci_err;
		}
	}
	return EOK ;
}

static int sdio_pci_config( sdio_hc_t *hc, sdio_product_t *prod )
{
	sdio_hc_cfg_t	*cfg;
	int				idx;
	pci_ba_t		ba[NELEMENTS(cfg->base_addr)];
	int_t			nba = NELEMENTS(ba);
	pci_irq_t		irq[NELEMENTS(cfg->irq)];
	int_t			nirq = NELEMENTS(irq);
	pci_err_t		pci_err;

	cfg = &hc->cfg;
#if 0
	/*
	 * These translations are not currently being used. Leaving for
	 * reference as they will need to be filled in if that changes in
	 * the future
	 */
	cfg->io_xlat = ;
	cfg->mem_xlat = ;
	cfg->bmstr_xlat = ;
#endif

	pci_err = pci_device_read_irq (hc->pci.dev_hdl, &nirq, irq);
	if( pci_err != PCI_ERR_OK ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 2,
				"%s: pci_device_read_irq() failed: %s.",
				__FUNCTION__, pci_strerror( pci_err ) );
		return( pci_err );
	}
	else if (nirq < 0) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 2,
				"%s: Device configured for %d more IRQ's than the %u the driver can handle",
				__FUNCTION__, -nirq, NELEMENTS(irq) );
		return( ENOMEM );
	}

	pci_err = pci_device_read_ba (hc->pci.dev_hdl, &nba, ba, pci_reqType_e_UNSPECIFIED);
	if( pci_err != PCI_ERR_OK ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 2,
				"%s: pci_device_read_ba() failed: %s",
				__FUNCTION__, pci_strerror( pci_err ) );
		return( pci_err );
	}

	if( cfg->irqs == 0 ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
				"%s: Number of irqs: %u",
				__FUNCTION__, nirq);
		cfg->irqs = nirq;
		for (idx = 0; idx < nirq; idx++) {
			cfg->irq[idx] = irq[idx];
		}
	}

	/* assign base addresses */
	for (idx = 0; idx < nba; idx++ ) {
		switch( ba[idx].type) {
		case pci_asType_e_NONE:
			break;
		case pci_asType_e_MEM:
			if ((ba[idx].attr & pci_asAttr_e_EXPANSION_ROM) != 0) {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_WARNING, hc->cfg.verbosity, 4,
						"%s: Unxpected ROM detected on BA %d.",
						__FUNCTION__, idx);
				break;
			}
			else {
				cfg->base_addr[cfg->base_addrs] = ba[idx].addr;
				cfg->base_addr_size[cfg->base_addrs] = ba[idx].size;
				cfg->base_addrs++;
			}
			break;
		case pci_asType_e_IO:
			cfg->base_addr[cfg->base_addrs] = ba[idx].addr;
			cfg->base_addr_size[cfg->base_addrs] = ba[idx].size;
			cfg->base_addrs++;
			break;
		default:
			sdio_slogf( _SLOGC_SDIODI, _SLOG_WARNING, hc->cfg.verbosity, 3,
					"%s: Unknown BA type %d.",
					__FUNCTION__, idx);
			break;
		}
	}

	/* need to ensure bus master is enabled */
	sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
			"%s: B%u:D%u:F%u, Check bus mastering is enabled", __FUNCTION__,
			PCI_BUS(hc->pci.bdf), PCI_DEV(hc->pci.bdf), PCI_FUNC(hc->pci.bdf) );

	pci_cmd_t cmd;
	pci_err = pci_device_read_cmd(hc->pci.bdf, &cmd);

	if (pci_err != PCI_ERR_OK)
	{
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 2,
				"%s: pci_device_read_cmd() failed: %s",
				__FUNCTION__, pci_strerror( pci_err ) );
		return( pci_err );
	}
	else if ((cmd & (1u << 2)) == 0)
	{
		pci_err = pci_device_write_cmd(hc->pci.dev_hdl, cmd | (1u << 2), &cmd);

		if (pci_err != PCI_ERR_OK)
		{
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 2,
					"%s: pci_device_write_cmd() failed: %s",
					__FUNCTION__, pci_strerror( pci_err ) );
			return( pci_err );
		}
		else if ((cmd & (1u << 2)) == 0)
		{
			sdio_slogf( _SLOGC_SDIODI, _SLOG_WARNING, hc->cfg.verbosity, 2,
					"%s: pci_device_write_cmd() ok but cmd reg is 0x%x",
					__FUNCTION__, cmd);
		}
		else
		{
			sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
					"%s: bus mastering is now enabled", __FUNCTION__);
		}
	}
	else
	{
		sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
				"%s: bus mastering already enabled", __FUNCTION__);
	}

	return( EOK );
}

int sdio_pci_device( sdio_hc_t *hc )
{
	sdio_hc_cfg_t		*cfg;
	sdio_product_t		*prod;

	cfg		= &hc->cfg;

	if( ( prod = sdio_hc_lookup( cfg->vid, cfg->did, cfg->class, "" ) ) == NULL ) {
		return( ENODEV );
	}


	hc->pci.bdf = pci_device_find(0, cfg->vid, cfg->did, PCI_CCODE_ANY);

	if (hc->pci.bdf == PCI_BDF_NONE) hc->pci.dev_hdl = NULL;
	else hc->pci.dev_hdl = pci_device_attach(hc->pci.bdf, pci_attachFlags_DEFAULT, NULL);

	if (hc->pci.dev_hdl == NULL)
	{
		hc->pci.bdf = PCI_BDF_NONE;
		return ENODEV;
	}
	else
	{
		sdio_pci_config( hc, prod );
		return( EOK );
	}
}

int sdio_pci_scan( )
{
	sdio_hc_t			*hc;
	sdio_hc_cfg_t		*cfg;
	sdio_product_t		*prod;
	int				status;

	hc = TAILQ_FIRST( &sdio_ctrl.hlist );

	pci_bdf_t bdf;
	int idx = 0;

	while ((bdf = pci_device_find(idx, PCI_VID_ANY, PCI_DID_ANY, PCI_CCODE_PERIPHERAL_SD_HOST_CTRL)) != PCI_BDF_NONE)
	{
		pci_err_t r;

		pci_vid_t vid;
		r = pci_device_read_vid(bdf, &vid);
		if (r != PCI_ERR_OK) continue;

		pci_did_t did;
		r = pci_device_read_did(bdf, &did);
		if (r != PCI_ERR_OK) continue;

		pci_ccode_t ccode;
		r = pci_device_read_ccode(bdf, &ccode);
		if (r != PCI_ERR_OK) continue;

		prod = sdio_hc_lookup( vid, did, ccode, "" );
		if (prod != NULL)
		{
			if( hc == NULL && ( hc = sdio_hc_alloc( ) ) == NULL ) {
				return( ENOMEM );
			}
			cfg				= &hc->cfg;
			cfg->vid		= vid;
			cfg->did		= did;
			cfg->class		= ccode;

			if( ( status = sdio_pci_device( hc ) ) != EOK ) {
				return( status );
			}
			hc = NULL;
		}
		++idx;
	}
	return( EOK );
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/sdiodi/pci.c $ $Rev: 820524 $")
#endif

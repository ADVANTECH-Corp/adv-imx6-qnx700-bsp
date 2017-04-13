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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>

#include <hw/inout.h>

#include <sys/mman.h>

#include <internal.h>
#include <sdhci.h>

#ifdef SDIO_HC_SDHCI_PCI

#include <pci/pci.h>
#include <pci/pci_id.h>
#include "pci_devices.h"

int sdhci_pci_init( sdio_hc_t *hc )
{
	sdio_hc_cfg_t		*cfg;
	int ret = 0;

	cfg							= &hc->cfg;

	ret = sdhci_init( hc );

	if (ret == EOK){
		switch( cfg->vid ) {
			case PCI_VID_O2Micro__Inc:
				if( cfg->did == PCI_DEVICE_ID_O2_8321 ) {
//					hc->version = SDHCI_SPEC_VER_3;
				}
				break;

			case PCI_VID_Intel_Corp:
				if(cfg->did == PCI_DEVICE_ID_INTEL_BROXTON_EMMC){
					hc->caps |= HC_CAP_HS200 | HC_CAP_HS400;
					hc->caps |= HC_CAP_SLOT_TYPE_EMBEDDED;
					hc->caps &= ~HC_CAP_CD_INTR;
					hc->flags |= HC_FLAG_DEV_MMC;
				}
				break;

			default:
				break;
		}
	}
	return ret;
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/sdiodi/hc/sdhci_pci.c $ $Rev: 820524 $")
#endif

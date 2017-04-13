/*
 * $QNXLicenseC:
 * Copyright 2016, QNX Software Systems.
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

/*
 * Configuration for Cortex-A57 MPCore:
 * - cache/page callouts use inner-cacheable/broadcast operations
 * - pte encodings use Shared/Write-back/Write-allocate encodings
 *
 * NOTE: we need to set bit 10 (ARM_MMU_CR_F) to enable swp instructions.
 *       These were deprecated in ARMv6 and Cortex-A57 disables them by
 *       default causing them to generate illegal instruction exceptions.
 *
 *       We also need to explicitly specify ARM_MMU_CR_XP even though that
 *       bit is always set to 1 by hardware so that arm_pte.c can detect
 *       that we are using the ARMv6 VMSA page table format.
 */
const struct armv_chip armv_chip_a57 = {
	.cpuid		= 0xd070,
	.name		= "Cortex A57",
	.cycles		= 2,
	.power		= &power_v7_wfi,
	.setup		= armv_setup_a57,
};

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/armv_chip_a57.c $ $Rev: 805283 $")
#endif
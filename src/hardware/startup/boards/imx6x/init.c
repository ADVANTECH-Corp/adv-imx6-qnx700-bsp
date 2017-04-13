/*
 * $QNXLicenseC: 
 * Copyright 2013 QNX Software Systems.  
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
#include "mx6x_startup.h"
#include <arm/mx6x.h>

#define CCM_CGPR_WAIT_MODE_FIX			(1 << 17)
#define MASKING_PARITY_IRQ125_WAIT_MODE_FIX	(1 << 29)

extern void l2_flush(void);

/*
 * Override the startup/lib's empty board_init function to include any code
 * applicable to ALL i.MX6x startups
 */
void board_init(void)
{

	/* 
	 * Some bootloaders (including recent versions of u-boot) enable the L2 cache before jumping
	 * to startup. In the initial start up code (cStart.S) we flush the L1 cache but not the L2 cache.
	 * Flush the L2 cache now to ensure that we don't loose any data in the L2 cache. Only do the flush
	 * if the L2 cache is currently enabled.
	 */
	if (in32(MX6X_PL310_BASE + L2_CTRL) & 0x1) {
		l2_flush();
	}

	/* Errata ERR006223 - CCM: Failure to resume from Wait/Stop mode with power gating
	 *
	 * When the Wait For Interrupt (WFI) instruction is executed, the Clock Control Module (CCM)
	 * will begin the WAIT mode preparation sequence. During the WAIT mode preparation sequence,
	 * there is a small window of time where an interrupt could potentially be detected after
	 * the L1 cache clock is gated but before the ARM clock is gated, causing a failure to resume
	 * from WFI.
	 *
	 * Freescale added a bit (offset 17) to CCM_CGPR which should be set at all times except before entering
	 * the STOP low power mode. Unless a BSP is using the STOP low power mode, CCM_CGPR[17] should
	 * always be set.
	 */

	unsigned wait_mode_workaround = FALSE;

	if (get_mx6_chip_type() == MX6_CHIP_TYPE_QUAD_OR_DUAL)
	{
		if (get_mx6_chip_rev() >=  MX6_CHIP_REV_1_2)
			wait_mode_workaround = TRUE;
	}
	else if (get_mx6_chip_type() == MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO)
	{
		if (get_mx6_chip_rev() >=  MX6_CHIP_REV_1_1)
			wait_mode_workaround = TRUE;
	}

	if (wait_mode_workaround == TRUE)
		out32(MX6X_CCM_BASE + MX6X_CCM_CGPR, in32(MX6X_CCM_BASE + MX6X_CCM_CGPR) | CCM_CGPR_WAIT_MODE_FIX);

	/* Parity Check interrupt is always pending on most of i.MX6 silicons which prevents the arm clock 
	* from being gated off (= WAIT MODE) when wfi is called.
	*
	* According to Freescale, no specific errata. But Freescale confirmed that i.MX6 does not support the   
	* parity feature due to the following ARM errata and other design integration issues. For this reason 
	* Freescale has masked the Parity interrupt 125 in their i.MX6 Linux BSP and recommended QNX/customers
	* to do the same workaround.
	*
	* - ERR005187 ARM/MP: 771223 - Parity errors on BTAC and GHB are reported on PARITYFAIL[7:6], regardless
	* of the Parity Enable bit value
	*
	* - ERR003738 ARM: 751475 - Parity error may not be reported on full cache line access (eviction / coherent
	* data transfer / cp15 clean  operations)
	*/

	out32(MX6X_GPC_BASE + MX6X_GPC_IMR3, in32(MX6X_GPC_BASE + MX6X_GPC_IMR3) | MASKING_PARITY_IRQ125_WAIT_MODE_FIX);
 
}



#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/imx6x/init.c $ $Rev: 794469 $")
#endif

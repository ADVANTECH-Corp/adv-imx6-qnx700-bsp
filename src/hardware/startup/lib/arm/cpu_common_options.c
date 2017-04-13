/*
 * $QNXLicenseC:
 * Copyright 2015, QNX Software Systems. 
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

int
cpu_handle_common_option(int opt)
{
	switch (opt) {
	case 'x':
		/*
		 * Set full implemented physical address size
		 */
		if ((arm_mmfr0_get() & 0xf) == 5) {
			switch ((arm_mmfr3_get() >> 24) & 0xf) {
			case 0:
				paddr_bits = 32;
				break;

			case 1:
				paddr_bits = 36;
				break;

			case 2:
				paddr_bits = 40;
				break;

			default:
				/*
				 * Undefined ID_MMFR3 encoding
				 */
				paddr_bits = 32;
				break;
			}
		}
		return 1;
	}
	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/cpu_common_options.c $ $Rev: 782220 $")
#endif

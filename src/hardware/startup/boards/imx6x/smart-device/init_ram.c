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



/*
 * init_raminfo.c
 *	Tell syspage about our RAM configuration
 */
#include "startup.h"
#include "board.h"
#include <arm/mx6x.h>

int enable_TZAC = 0;

void init_ram(unsigned gpu_start, unsigned ram_size) 
{
	add_ram(MX6X_SDRAM_64_BIT_BASE, MEG(ram_size));

	if (enable_TZAC) {
		paddr_t    paddr;

		/* Remove reserved memory from further allocations and mark as 'graphics' in asinfo */
		paddr = alloc_ram(gpu_start, MEG(GPU_SDRAM_SIZE), 0);
		if(paddr != (paddr_t)gpu_start)
			crash("%s: Cannot reserve %dk at 0x%x", __FUNCTION__, (unsigned)ram_size/1024, (unsigned)gpu_start);

		as_add_containing(gpu_start, MX6X_SDRAM_64_BIT_BASE + MEG(ram_size) - 1, AS_ATTR_RAM, "graphics", "ram");
	}
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/imx6x/smart-device/init_ram.c $ $Rev: 819455 $")
#endif


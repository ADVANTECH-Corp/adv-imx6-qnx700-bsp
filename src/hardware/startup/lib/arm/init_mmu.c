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

/*
 * The L1/L2 paddr values are directly used by startup as pointers
 */
uintptr_t	L1_paddr;
uintptr_t	L1_vaddr;
uintptr_t	L2_paddr;
uintptr_t	startup_base;
unsigned	startup_size;

uintptr_t	(*mmu_map)(uintptr_t va, paddr_t pa, size_t sz, unsigned prot);
void		(*mmu_map_cpu)(int cpu, uintptr_t va, paddr_t pa, unsigned flags);
void 		(*mmu_elf_map)(uintptr_t va, paddr32_t pa, size_t sz, unsigned prot);
paddr_t		(*mmu_vaddr_to_paddr)(uintptr_t va);

/*
 * Initialize page tables. This prepares the system to leap into virtual mode.
 * This code is hardware independant and should not have to be
 * changed by end users.
 */
void
init_mmu(void)
{
	if (paddr_bits > 32) {
		init_mmu_lpae();
	} else {
		init_mmu_v7();
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/init_mmu.c $ $Rev: 781531 $")
#endif

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
#include <arm/mpcore.h>

/*
 * Additional Cortex A9 MPCore specific CPU initialisation
 */

#define	CPUID_REVISION(x)	((x) & 0x00f0000f)
#define	CPUID_R2P0			0x00200000
#define CPUID_R2P2			0x00200002
#define CPUID_R3P0			0x00300000
#define CPUID_R4P0			0x00400000

static inline unsigned
cp15_diag_get(void)
{
	unsigned	diag;
	__asm__ __volatile__("mrc	p15, 0, %0, c15, c0, 1" : "=r"(diag));
	return diag;
}

static inline void
cp15_diag_set(unsigned diag)
{
	__asm__ __volatile__("mcr	p15, 0, %0, c15, c0, 1" : : "r"(diag));
}

/*
 * Board startups should set either of these variable if that register
 * cannot be written in non-secure state.
 *
 * a9_actlr_allowed - set to 0 if actlr writes are denied
 * a9_diag_allowed  - set to 0 if CP15 diagnostic register writes are denied
 */
int	a9_actlr_allowed = 1;
int	a9_diag_allowed = 1;

void
armv_setup_a9(struct cpuinfo_entry *cpu, unsigned cpunum, unsigned cpuid)
{
	if (cpunum == 0 && mpcore_scu_base != NULL_PADDR) {
		unsigned	scu_diag;

		/*
		 * SCU invalidate all and enable SCU
		 */
		out32(mpcore_scu_base + MPCORE_SCU_INVALIDATE, -1);
		out32(mpcore_scu_base + MPCORE_SCU_CTRL, MPCORE_SCU_CTRL_EN);
		scu_diag = in32(mpcore_scu_base + MPCORE_SCU_DIAGNOSTIC);        
		scu_diag |= 0x1;              //disable migratory
		out32(mpcore_scu_base + MPCORE_SCU_DIAGNOSTIC, scu_diag);
	}

	/*
	 * Configure CP15 actlr
	 * - clear EXCL (bit 7) to set inclusive caches
	 * - set SMP (bit 6)
	 * - set FW  (bit 0) enable cache/TLB maintainence broadcast
	 */
	unsigned	actlr = arm_actlr_get();
	unsigned	new_actlr;

	new_actlr = actlr & ~(1 << 7);
	if (mpcore_scu_base != NULL_PADDR) {
		new_actlr |= (1 << 6);
	}
	new_actlr |= (1 << 0);
	if (CPUID_REVISION(cpuid) < CPUID_R3P0) {
		new_actlr &= ~((1 << 2) | (1 << 1));	// Errata #719332 (r0, r1) and #751473 (r2) - disable prefetch
	}

	if (a9_actlr_allowed) {
		arm_actlr_set(new_actlr);
	}
	actlr = arm_actlr_get();
	if (new_actlr != actlr) {
		kprintf("cpu%d: WARNING: actlr=%x (wanted %x)\n", cpunum, actlr, new_actlr);
	}

	if (CPUID_REVISION(cpuid) < CPUID_R2P0) {
		/*
		 * Inner-shareable TLB ops don't work correctly so make sure
		 * procnto-smp applies the necessary workaround using TLBIALLIS
		 */
		cpu->flags &= ~ARM_CPU_FLAG_V7_MP;
		cpu->flags |= ARM_CPU_FLAG_V7_MP_ERRATA;	// Errata #720789

		sctlr_set |= ARM_SCTLR_RR;		// Errata #716044
	}

	if (a9_diag_allowed) {

		/* Erratum fixes applicable to Cortex-A9 MPCore but not Cortex-A9 uniprocessor */
		if (mpcore_scu_base != NULL_PADDR) {
			if (CPUID_REVISION(cpuid) < CPUID_R4P0) {
				/* 
				 * Fix for ARM Errata 761320 - "Full cache line writes to the same memory region from at
				 * least two processors might deadlock the processor"
				 * Only applicable for 2 or more processors if ACP present, or 3 or more processors if ACP not present.
				 * Assume ACP present.
				 */
				if (lsp.syspage.p->num_cpu >= 2)
					cp15_diag_set(cp15_diag_get() | (1u << 21));	// #0x200000
			}
			/* 
			 * Fix for ARM Errata 845369 - "Under very rare timing circumstances, transitioning 
			 * into streaming mode might create a data corruption".
			 * Only applicable for 1 or more processors if ACP present, or 2 or more processors if ACP not present.
			 * Assume ACP present. Present in all revisions.
			 */
			cp15_diag_set(cp15_diag_get() | (1u << 22));	// #0x00400000
		}

		/*
		 * Fix for ARM Errata 742230 (applies to versions <= R2P2) applicable for 1 processor
		 * with the ACP, or 2 or more processors with or without ACP.
		 * And ARM Errata 794072 (applicable to all versions of the Cortex-A9 MPCore with two or more processors)
		 */
		if (CPUID_REVISION(cpuid) <= CPUID_R2P2 || (lsp.syspage.p->num_cpu >= 2))
			cp15_diag_set(cp15_diag_get() | (1u << 4)); // set bit #4

		if (CPUID_REVISION(cpuid) < CPUID_R3P0) {
			/* 
			 * Apply fix for ARM_ERRATA_751472 for all r0, r1, r2 revisions (applicable for 2 or more CPUs working in SMP mode):
			 * "An interrupted ICIALLUIS operation may prevent the completion
			 *  of a following broadcasted operation" 
			 */
			if(lsp.syspage.p->num_cpu >= 2)
				cp15_diag_set(cp15_diag_get() | (1u << 11)); // set bit #11

			if (CPUID_REVISION(cpuid) >= CPUID_R2P0) {
				/*
				 * Apply fix for ARM_ERRATA_743622 for all r2 revisions:
				 * "Faulty logic in the Store Buffer may lead to data corruption"
				 */
				cp15_diag_set(cp15_diag_get() | (1u << 6)); // set bit #6
			}
		}
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/armv_setup_a9.c $ $Rev: 790639 $")
#endif

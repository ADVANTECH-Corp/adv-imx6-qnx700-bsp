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
 * Page table encoding values
 */
struct v7_pte {
	unsigned short		upte_ro;	// user RO mappings
	unsigned short		upte_rw;	// user RW mappings
	unsigned short		kpte_ro;	// kern RO mappings
	unsigned short		kpte_rw;	// kern RW mappings
	unsigned			kscn_ro;	// kern RO section mapping
	unsigned			kscn_rw;	// kern RW section mapping
	unsigned			kscn_cb;	// cacheable section mapping
	unsigned short		pte_attr;	// page table mapping attributes
	unsigned short		ttb_attr;	// ttbr attributes
};

static struct v7_pte	v7_pte = {
	.upte_ro = ARM_L2_SP | ARM_SP_WA | ARM_L2_URO | ARM_L2_nG,
	.upte_rw = ARM_L2_SP | ARM_SP_WA | ARM_L2_URW | ARM_L2_nG,
	.kpte_ro = ARM_L2_SP | ARM_SP_WA | ARM_L2_KRO,
	.kpte_rw = ARM_L2_SP | ARM_SP_WA | ARM_L2_KRW,

	.kscn_ro = ARM_L1_SC | ARM_SCN_KRO,
	.kscn_rw = ARM_L1_SC | ARM_SCN_KRW,
	.kscn_cb = ARM_SCN_ATTR_MASK,

	.pte_attr = ARM_L2_SP | ARM_L2_KRW | ARM_SP_XN,
	.ttb_attr = 0
};

unsigned	v7_ttbr;

#define	CPU_L1_TABLE(cpu)	(L1_paddr + ((cpu) * ARM_L1_SIZE))
#define	CPU_L2_TABLE(cpu)	(L2_paddr + ((cpu) * __PAGESIZE))

static pte32_t
set_pte(paddr_t pa, unsigned prot)
{
	pte32_t	pte = pa & ~PGMASK;

	if (prot & PROT_PGTBL) {
		/*
		 * Page table mapping attributes must match TTBR attributes
		 */
		return pte | v7_pte.pte_attr;
	}

	/*
	 * Create global mappings with required access permissions
	 */
	if (prot & PROT_USER) {
		pte |= (prot & PROT_WRITE) ? v7_pte.upte_rw : v7_pte.upte_ro;
	} else {
		pte |= (prot & PROT_WRITE) ? v7_pte.kpte_rw : v7_pte.kpte_ro;
	}
	if ((prot & PROT_EXEC) == 0) {
		pte |= ARM_SP_XN;
	}
	pte &= ~ARM_L2_nG;

	/*
	 * Need to change attribute for device/uncached mappings
	 */
	if (prot & (PROT_NOCACHE|PROT_DEVICE)) {
		pte &= ~ARM_SP_ATTR_MASK;

		switch (prot & (PROT_NOCACHE|PROT_DEVICE)) {
		case PROT_DEVICE:
			pte |= ARM_SP_DEV;
			break;

		case PROT_DEVICE|PROT_NOCACHE:
			pte |= ARM_SP_SO;
			break;

		case PROT_NOCACHE:
			pte |= ARM_SP_NC;
			break;
		}
	}
	return pte;
}

/*
 * Map a 4K page table into "page directory" and L1 table for specified cpu
 */
static void
map_ptbl_cpu(int cpu, pte32_t ptbl, uintptr_t vaddr)
{
	pte32_t	*pte = (pte32_t *)CPU_L2_TABLE(cpu) + (vaddr >> 22);
	pte32_t	*ptp = (pte32_t *)CPU_L1_TABLE(cpu) + ((vaddr >> 20) & ~3);
	int		i;

	/*
	 * Map the page table into the "page directory" page table
	 */
	*pte = ptbl | v7_pte.pte_attr;

	/*
	 * Map the page table into the L1 table
	 */
	for (i = 0; i < 4; i++, ptbl += ARM_L2_SIZE) {
		*ptp++ = ptbl | ARM_L1_PT;
	}
}

/*
 * Map a 4K page table into "page directory" and L1 table on all cpus
 */
static void
map_ptbl(pte32_t ptbl, uintptr_t vaddr)
{
	int		cpu;

	for (cpu = 0; cpu < lsp.syspage.p->num_cpu; cpu++) {
		map_ptbl_cpu(cpu, ptbl, vaddr);
	}
}

/*
 * Map a 1MB section into the L1 table.
 */
static void
v7_scmap(uintptr_t vaddr, paddr_t paddr, unsigned prot)
{
	pte32_t		pte = (pte32_t)paddr;
	unsigned	cpu;

	if ((paddr_t)pte != paddr) {
		crash("v7_scmap: paddr out of range\n");
	}

	if (prot & PROT_WRITE) {
		pte |= v7_pte.kscn_rw;
	} else {
		pte |= v7_pte.kscn_ro;
	}
	if ((prot & PROT_NOCACHE) == 0) {
		pte |= ARM_SCN_WA;
	}
	if ((prot & PROT_EXEC) == 0) {
		pte |= ARM_SCN_XN;
	}
	for (cpu = 0; cpu < lsp.syspage.p->num_cpu; cpu++) {
		pte32_t	*ptp = (pte32_t *)CPU_L1_TABLE(cpu) + (vaddr >> 20);
		*ptp = pte;
	}
}

/*
 * Unmap section entry
 */
static void
scunmap(uintptr_t vaddr)
{
	unsigned	cpu;

	for (cpu = 0; cpu < lsp.syspage.p->num_cpu; cpu++) {
		pte32_t	*ptp = (pte32_t *)CPU_L1_TABLE(cpu) + (vaddr >> 20);
		*ptp = 0;
	}
}

static uintptr_t
v7_map(uintptr_t vaddr, paddr_t paddr, size_t size, unsigned prot)
{
	static uintptr_t	next_addr = ARM_STARTUP_BASE;
	pte32_t				pa = (pte32_t)paddr;
	uintptr_t			off = pa & PGMASK;
	int					cpu;

	if ((paddr_t)pa != paddr) {
		crash("v7_map: paddr out of range\n");
	}

	if (!(shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL)) {
		return (uintptr_t)paddr;
	}

	size = ROUNDPG(size + off);
	if (vaddr == ~(uintptr_t)0) {
		vaddr = next_addr;
		next_addr += size;
	}

	pa = set_pte(pa, prot);

	for (cpu = 0; cpu < lsp.syspage.p->num_cpu; cpu++) {
		size_t		sz = size;
		uintptr_t	va = vaddr;
		pte32_t		pt = pa;
		pte32_t		*ptp = (pte32_t *)CPU_L1_TABLE(cpu) + ((va >> 20) & ~3);

		while (sz) {
			int		i;
			pte32_t	*pte = (pte32_t *)(*ptp & ~PGMASK);

			if (!ARM_L1_VALID(*ptp)) {
				/*
				 * Need to allocate a page table
				 */
				pte = (pte32_t *)calloc_ram(__PAGESIZE, __PAGESIZE);
				map_ptbl((pte32_t)pte, va);
			}
			pte += (va >> 12) & (ARM_L2_SIZE-1);
			for (i = (va >> 12) & (ARM_L2_SIZE-1); sz && i < ARM_L2_SIZE; i++) {
				*pte++ = pt;
				pt += __PAGESIZE;
				va += __PAGESIZE;
				sz -= __PAGESIZE;
			}
			ptp += 4;	// move to next 4MB
		}
	}
	return vaddr + off;
}

static void
v7_map_cpu(int cpu, uintptr_t vaddr, paddr_t paddr, unsigned prot)
{
	pte32_t	*ptp = (pte32_t *)CPU_L1_TABLE(cpu) + ((vaddr >> 20) & ~3);
	pte32_t	*pte = (pte32_t *)(*ptp & ~PGMASK);
	pte32_t	pa = (pte32_t)paddr;

	if ((paddr_t)pa != paddr) {
		crash("v7_map_cpu: paddr out of range\n");
	}

	if (!ARM_L1_VALID(*ptp)) {
		/*
		 * Need to allocate a page table
		 */
		pte = (pte32_t *)calloc_ram(__PAGESIZE, __PAGESIZE);
		map_ptbl_cpu(cpu, (pte32_t)pte, vaddr);
	}
	pte += (vaddr >> 12) & (ARM_L2_SIZE-1);
	*pte = set_pte(pa, prot);
}

static paddr_t
v7_vaddr_to_paddr(uintptr_t vaddr) {
	pte32_t	*ptp;
	pte32_t	*pte;

	ptp = (pte32_t *)CPU_L1_TABLE(0) + (vaddr >> 20);
	if (ARM_L1_SCN(*ptp)) {
		return (*ptp & ~ARM_SCMASK) + (vaddr & ARM_SCMASK);
	}
	else {
		pte = (pte32_t *)(*ptp & ~PGMASK);
		pte += (vaddr >> 12) & (ARM_L2_SIZE-1);
		return (*pte & ~PGMASK) + (vaddr & PGMASK);
	}
};

static void
v7_elf_map(uintptr_t vaddr, paddr32_t paddr, size_t size, unsigned prot)
{
#define	MAX_PHDR	2	/* We assume we only have 2 load segments */
	static uintptr_t	va[MAX_PHDR];
	static paddr32_t	pa[MAX_PHDR];
	static size_t		sz[MAX_PHDR];
	static int			fl[MAX_PHDR];
	static int			phdr;

	/*
	 * We assume all the elf sections are physically contiguous
	 * If they are not, we must remap using 4K pages
	 */
	if ((vaddr & ARM_SCMASK) != (paddr & ARM_SCMASK) ||
	    (phdr && (phdr == -1 || pa[phdr-1] + sz[phdr-1] != paddr))) {
		int	i;

		for (i = 0; i < phdr; i++) {
			/*
			 * assumes segment was <= 1MB...
			 */
			scunmap(va[i]);
		}
		for (i = 0; i < phdr; i++) {
			if (debug_flag > 1) {
				kprintf("elf_map: 4K va=%v pa=%x sz=%x fl=%x\n", va[i], pa[i], sz[i], fl[i]);
			}
			mmu_map(va[i], pa[i], sz[i], fl[i]);
		}
		if (debug_flag > 1) {
			kprintf("elf_map: 4K va=%v pa=%x sz=%x fl=%x\n", vaddr, paddr, size, prot);
		}
		mmu_map(vaddr, paddr, size, prot);
		phdr = -1;
		return;
	}

	if (phdr == MAX_PHDR) {
		crash("bootstrap has too many load segments");
	}
	va[phdr] = vaddr;
	pa[phdr] = paddr;
	sz[phdr] = size;
	fl[phdr] = prot;
	phdr++;

	size = (size + (vaddr & ARM_SCMASK) + ARM_SCMASK) & ~ARM_SCMASK;
	vaddr &= ~ARM_SCMASK;
	paddr &= ~ARM_SCMASK;
	if (debug_flag > 1) {
		kprintf("elf_map: 1M va=%v pa=%x sz=%x\n", vaddr, paddr, size);
	}
	while (size) {
		v7_scmap(vaddr, paddr, PROT_READ|PROT_WRITE|PROT_EXEC);
		vaddr += ARM_SCSIZE;
		paddr += ARM_SCSIZE;
		size  -= ARM_SCSIZE;
	}
}

/*
 * ARMv7 MMU initialisation:
 * - set mapping functions for ARMv7 MMU
 * - set up page table descriptor information passed in ARM syspage
 * - allocate initial page tables to map syspage/callouts and procnto
 * - set up 1-1 mapping for startup code for vstart_v7
 */
void
init_mmu_v7(void)
{
	unsigned	base;
	unsigned	ncpu = lsp.syspage.p->num_cpu;
	unsigned	L1size = ncpu * ARM_L1_SIZE;
	unsigned	L2size = ncpu * __PAGESIZE;
	int			i;

	/*
	 * Set up MMU mapping functions
	 */
	vstart = vstart_v7;
	mmu_map = v7_map;
	mmu_map_cpu = v7_map_cpu;
	mmu_elf_map = v7_elf_map;
	mmu_vaddr_to_paddr = v7_vaddr_to_paddr;

	/*
	 * Set up the pte and ttb encodings
	 */
	if ((arm_mpidr_get() & (1u << 31)) != 0) {
		/*
		 * Multiprocessing extensions are implemented so page table walks
		 * are required to look in the L1 cache.
		 * This means we can map page tables as write-back/write-allocate.
		 */
		v7_pte.pte_attr |= ARM_SP_WA;
		v7_pte.ttb_attr |= ARM_TTBR_RGN_WA | ARM_TTBR_IRGN_WA;
	} else {
		/*
		 * Multiprocessing extensions are not implemented so page table walks
		 * are not required to look in the L1 cache, so page table writes must
		 * be cleaned to PoU in order to be visible to page table walks.
		 * We map page tables as write-through so that we can cache page table
		 * accesses without requiring explicit cache maintenance operations.
		 */
		v7_pte.pte_attr |= ARM_SP_WT;
		v7_pte.ttb_attr |= ARM_TTBR_RGN_WT;
	}
	if (ncpu > 1) {
		/*
		 * Add shareable attribute to pte and ttb attributes
		 */
		v7_pte.upte_ro |= ARM_L2_S;
		v7_pte.upte_rw |= ARM_L2_S;
		v7_pte.kpte_ro |= ARM_L2_S;
		v7_pte.kpte_rw |= ARM_L2_S;
		v7_pte.kscn_ro |= ARM_SCN_S;
		v7_pte.kscn_rw |= ARM_SCN_S;
		v7_pte.pte_attr |= ARM_L2_S;
		v7_pte.ttb_attr |= ARM_TTBR_S;
	}

	/*
	 * Set the syspage arm_cpu fields
	 */
	struct arm_cpu_entry	*cpu = lsp.cpu.arm_cpu.p;

	cpu->upte_ro = v7_pte.upte_ro;
	cpu->upte_rw = v7_pte.upte_rw;
	cpu->kpte_ro = v7_pte.kpte_ro;
	cpu->kpte_rw = v7_pte.kpte_rw;
	cpu->mask_nc = ARM_SP_ATTR_MASK;
	cpu->ttb_attr = v7_pte.ttb_attr;
	cpu->pte_attr = v7_pte.pte_attr;

	/*
	 * Allocate the L1 table and the "page directory" used to map L2 tables
	 */
	L1_paddr = calloc_ram(L1size, ARM_L1_SIZE);
	L2_paddr = calloc_ram(L2size, __PAGESIZE);
	v7_ttbr = L1_paddr | v7_pte.ttb_attr;

	/*
	 * Map the "page directory" within itself for each processor
	 */
	for (i = 0; i < ncpu; i++) {
		map_ptbl_cpu(i, CPU_L2_TABLE(i), ARM_KPTP_BASE);
	}

	/*
	 * Map the real L1 table
	 */
	L1_vaddr = v7_map(~0L, L1_paddr, L1size, PROT_PGTBL);

	/*
	 * Section map startup code to allow transition to virtual addresses.
	 * This 1-1 mapping is also used by kdebug to access the imagefs.
	 * procnto uses syspage->un.arm.startup_base/startup_size to unmap it.
	 */
	startup_base = shdr->ram_paddr & ~ARM_SCMASK;
	startup_size = shdr->ram_size;
	for (base = startup_base; base < startup_base + startup_size; base += ARM_SCSIZE) {
		v7_scmap(base, base, PROT_READ|PROT_EXEC);
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/mmu_v7.c $ $Rev: 784178 $")
#endif

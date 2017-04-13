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
 * Default ttbcr configuration:
 * - EAE   specifies long descriptor format
 * - A1    specifies TTBR0 sets ASID (A1 bit is 0)
 * - SH1   specifies non-shared attribute for TTBR1 walks
 * - ORGN1 specifies outer WBWA attribute for TTBR1 walks
 * - IRGN1 specifies inner WBWA attribute for TTBR1 walks
 * - T1SZ  specifies 30-bit TTBR1 address width to map [3G, 4G)
 * - SH0   specifies non-shared attribute for TTBR0 walks
 * - ORGN0 specifies outer WBWA attribute for TTBR0 walks
 * - IRGN0 specifies inner WBWA attribute for TTBR0 walks
 * - T0SZ  specifies 32-bit TTBR0 address width to map [0, 3G)
 *
 * init_mmu_lpae() will change SH1/SH0 to inner-shared for SMP.
 */
unsigned	lpae_ttbcr =	ARM_LPAE_TTBCR_EAE |
							ARM_LPAE_TTBCR_SH1_NSH |
							ARM_LPAE_TTBCR_ORGN1_WBWA |
							ARM_LPAE_TTBCR_IRGN1_WBWA |
							ARM_LPAE_TTBCR_T1SZ(30) |
							ARM_LPAE_TTBCR_SH0_NSH |
							ARM_LPAE_TTBCR_ORGN0_WBWA |
							ARM_LPAE_TTBCR_IRGN0_WBWA |
							ARM_LPAE_TTBCR_T0SZ(32);

uint64_t	lpae_ttbr0[PROCESSORS_MAX];
uint64_t	lpae_ttbr1[PROCESSORS_MAX];

#define	CPU_L1_TABLE(cpu)	((pte64_t *)(L1_paddr + ((cpu) * ARM_LPAE_L1_SIZE)))
#define	CPU_L2_TABLE(cpu)	((pte64_t *)(L2_paddr + ((cpu) * ARM_LPAE_L2_SIZE)))

#define	PTE_PADDR(p)	((p) & 0x0000fffffffff000ull)
#define	PTE_PTR(p)		((pte64_t *)(uintptr_t)PTE_PADDR(p))

static pte64_t
set_pte(paddr_t pa, unsigned prot)
{
	pte64_t	pte = PTE_PADDR(pa) | ARM_LPAE_PTE_L3 | ARM_LPAE_PTE_AF;

	if (prot == PROT_PGTBL) {
		/*
		 * Page tables are mapped using regular WBWA cacheable attribute
		 */
		prot = PROT_READ|PROT_WRITE;
	}

	/*
	 * Set access permissions
	 */
	if (prot & PROT_WRITE) {
		pte |= (prot & PROT_USER) ? ARM_LPAE_PTE_URW : ARM_LPAE_PTE_KRW;
	} else {
		pte |= (prot & PROT_USER) ? ARM_LPAE_PTE_URO : ARM_LPAE_PTE_KRO;
	}
	if ((prot & PROT_EXEC) == 0) {
		pte |= ARM_LPAE_PTE_UXN | ARM_LPAE_PTE_PXN;
	}

	/*
	 * Set mapping attributes.
	 */
	if (prot & PROT_DEVICE) {
		pte |= (prot & PROT_NOCACHE) ? ARM_LPAE_PTE_SO : ARM_LPAE_PTE_DEV;
	} else {
		pte |= (prot & PROT_NOCACHE) ? ARM_LPAE_PTE_NC : ARM_LPAE_PTE_WBWA;
		if (lsp.syspage.p->num_cpu > 1) {
			pte |= ARM_LPAE_PTE_ISH;
		}
	}
	return pte;
}

static void
map_l2_cpu(int cpu, pte64_t pa, uintptr_t va)
{
	pte64_t	*L1 = CPU_L1_TABLE(cpu);

	L1[ARM_LPAE_L1_IDX(va)] = pa;
}

static void
map_l2(pte64_t pa, uintptr_t va)
{
	int		cpu;

	for (cpu = 0; cpu < lsp.syspage.p->num_cpu; cpu++) {
		map_l2_cpu(cpu, pa, va);
	}
}

static void
map_l3_cpu(int cpu, pte64_t pa, uintptr_t va)
{
	pte64_t	*L2 = CPU_L2_TABLE(cpu);

	if (va < ARM_LPAE_USER_END) {
		crash("map_l3_cpu %x\n", va);
	}
	L2[ARM_LPAE_L2_IDX(va)] = pa;
}

static void
map_l3(pte64_t pa, uintptr_t va)
{
	int		cpu;

	for (cpu = 0; cpu < lsp.syspage.p->num_cpu; cpu++) {
		map_l3_cpu(cpu, pa, va);
	}
}

static void
block_map(uintptr_t vaddr, paddr_t paddr, unsigned prot)
{
	int		cpu;

	if (paddr & ARM_LPAE_BLOCK_MASK) {
		crash("block_map: %P is not 2MB aligned", paddr);
	}

	/*
	 * Create block descriptor for paddr
	 */
	paddr = (set_pte(paddr, prot) & ~ARM_LPAE_PTP_TYPE) | ARM_LPAE_PTP_BLK;

	for (cpu = 0; cpu < lsp.syspage.p->num_cpu; cpu++) {
		pte64_t	pt;
		pte64_t	*pte = CPU_L1_TABLE(cpu) + ARM_LPAE_L1_IDX(vaddr);

		if (!ARM_LPAE_PTP_VALID(*pte)) {
			pt = calloc_ram(ARM_LPAE_L2_SIZE, ARM_LPAE_L2_SIZE);
			pt = set_pte(pt, PROT_PGTBL);
			map_l2(pt, vaddr);
		}
		
		/*
		 * Create block descriptor
		 */
		pte = PTE_PTR(*pte) + ARM_LPAE_L2_IDX(vaddr);
		*pte = paddr;
	}
}

static void
block_unmap(uintptr_t vaddr)
{
	int		cpu;

	for (cpu = 0; cpu < lsp.syspage.p->num_cpu; cpu++) {
		pte64_t	*pte = CPU_L1_TABLE(cpu) + ARM_LPAE_L1_IDX(vaddr);

		if (ARM_LPAE_PTP_VALID(*pte)) {
			pte = PTE_PTR(*pte) + ARM_LPAE_L2_IDX(vaddr);
			*pte = 0;
		}
	}
}

static uintptr_t
lpae_map(uintptr_t vaddr, paddr_t paddr, size_t size, unsigned prot)
{
	static uintptr_t	next_addr = ARM_STARTUP_BASE;
	uintptr_t			off = paddr & PGMASK;
	int					cpu;

	if ((shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL) == 0) {
		return paddr;
	}

	size = ROUNDPG(size + off);
	if (vaddr == ~(uintptr_t)0) {
		vaddr = next_addr;
		next_addr += size;
	}

	/*
	 * Create page descriptor for paddr
	 */
	paddr = set_pte(paddr, prot);

	for (cpu = 0; cpu < lsp.syspage.p->num_cpu; cpu++) {
		paddr_t		pa = paddr;
		uintptr_t	va = vaddr;
		size_t		sz = size;

		while (sz) {
			int			i;
			pte64_t	pt;
			pte64_t	*pte;

			/*
			 * Check L1 entry for vaddr and allocate L2 table if required
			 */
			pte = CPU_L1_TABLE(cpu) + ARM_LPAE_L1_IDX(va);
			if (!ARM_LPAE_PTP_VALID(*pte)) {
				pt = calloc_ram(ARM_LPAE_L2_SIZE, ARM_LPAE_L2_SIZE);
				pt = set_pte(pt, PROT_PGTBL);
				map_l2(pt, va);
			}

			/*
			 * Check L2 entry for vaddr and allocate L3 table if required
			 */
			pte = PTE_PTR(*pte) + ARM_LPAE_L2_IDX(va);
			if (!ARM_LPAE_PTP_VALID(*pte)) {
				pt = calloc_ram(ARM_LPAE_L3_SIZE, ARM_LPAE_L3_SIZE);
				pt = set_pte(pt, PROT_PGTBL);
				map_l3(pt, va);
			}

			/*
			 * Set L3 entries
			 */
			pte = PTE_PTR(*pte);
			for (i = ARM_LPAE_L3_IDX(va); sz && i < ARM_LPAE_NPTE_PTBL; i++) {
				pte[i] = pa;
				pa += __PAGESIZE;
				va += __PAGESIZE;
				sz -= __PAGESIZE;
			}
		}
	}
	return vaddr + off;
}

static void
lpae_map_cpu(int cpu, uintptr_t vaddr, paddr_t paddr, unsigned prot)
{
	pte64_t	*pte;
	pte64_t	pt;

	/*
	 * Create page descriptor for paddr
	 */
	paddr = set_pte(paddr, prot);

	/*
	 * Check L1 entry for vaddr and allocate L2 table if required
	 */
	pte = CPU_L1_TABLE(cpu) + ARM_LPAE_L1_IDX(vaddr);
	if (!ARM_LPAE_PTP_VALID(*pte)) {
		pt = calloc_ram(ARM_LPAE_L2_SIZE, ARM_LPAE_L2_SIZE);
		pt = set_pte(pt, PROT_PGTBL);
		map_l2_cpu(cpu, pt, vaddr);
	}

	/*
	 * Check L2 entry for vaddr and allocate L3 table if required
	 */
	pte = PTE_PTR(*pte) + ARM_LPAE_L2_IDX(vaddr);
	if (!ARM_LPAE_PTP_VALID(*pte)) {
		pt = calloc_ram(ARM_LPAE_L3_SIZE, ARM_LPAE_L3_SIZE);
		pt = set_pte(pt, PROT_PGTBL);
		map_l3_cpu(cpu, pt, vaddr);
	}

	/*
	 * Set L3 entry
	 */
	pte = PTE_PTR(*pte);
	pte[ARM_LPAE_L3_IDX(vaddr)] = paddr;
}

static paddr_t
lpae_vaddr_to_paddr(uintptr_t vaddr)
{
	paddr_t		paddr;

	if (vaddr < ARM_LPAE_USER_END) {
		crash("vtop %x\n", vaddr);
	}

	pte64_t	*pte = CPU_L2_TABLE(0) + ARM_LPAE_L2_IDX(vaddr);

	switch (*pte & ARM_LPAE_PTP_TYPE) {
	case ARM_LPAE_PTP_BLK:
		paddr = PTE_PADDR(*pte) + (vaddr & ARM_LPAE_BLOCK_MASK);
		break;

	case ARM_LPAE_PTP_TBL:
		pte = PTE_PTR(*pte);
		paddr = PTE_PADDR(pte[ARM_LPAE_L3_IDX(vaddr)]) | (vaddr & PGMASK);
		break;

	default:
		paddr = NULL_PADDR;
		break;
	}
	return paddr;
}

static void
lpae_elf_map(uintptr_t vaddr, paddr32_t paddr, size_t size, unsigned prot)
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
	if ((vaddr & ARM_LPAE_BLOCK_MASK) != (paddr & ARM_LPAE_BLOCK_MASK) ||
	    (phdr && (phdr == -1 || pa[phdr-1] + sz[phdr-1] != paddr))) {
		int	i;

		for (i = 0; i < phdr; i++) {
			/*
			 * assumes segment was <= 2MB...
			 */
			block_unmap(va[i]);
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

	size = (size + (vaddr & ARM_LPAE_BLOCK_MASK) + ARM_LPAE_BLOCK_MASK) & ~ARM_LPAE_BLOCK_MASK;
	vaddr &= ~ARM_LPAE_BLOCK_MASK;
	paddr &= ~ARM_LPAE_BLOCK_MASK;
	if (debug_flag > 1) {
		kprintf("elf_map: 2M va=%v pa=%x sz=%x\n", vaddr, paddr, size);
	}
	while (size) {
		block_map(vaddr, paddr, PROT_READ|PROT_WRITE|PROT_EXEC);
		vaddr += ARM_LPAE_BLOCK_SIZE;
		paddr += ARM_LPAE_BLOCK_SIZE;
		size  -= ARM_LPAE_BLOCK_SIZE;
	}
}

void
init_mmu_lpae(void)
{
	unsigned	ncpu = lsp.syspage.p->num_cpu;
	int			i;

	/*
	 * Set up MMU mapping functions
	 */
	vstart = vstart_lpae;
	mmu_map = lpae_map;
	mmu_map_cpu = lpae_map_cpu;
	mmu_elf_map = lpae_elf_map;
	mmu_vaddr_to_paddr = lpae_vaddr_to_paddr;

	/*
	 * Set inner-shareable attributes for page table walks if SMP
	 */
	if (ncpu > 1) {
		lpae_ttbcr |= ARM_LPAE_TTBCR_SH1_ISH | ARM_LPAE_TTBCR_SH0_ISH;
	}

	/*
	 * Allocate L1 and L2 tables for each processor:
	 *
	 * L1 tables are each 32 bytes (4 entries)
	 * L2 tables are each 4K.
	 */
	L1_paddr = calloc_ram(__PAGESIZE, __PAGESIZE);
	L2_paddr = calloc_ram(ncpu * ARM_LPAE_L2_SIZE, ARM_LPAE_L2_SIZE);

	/*
	 * Set up the TTBR0 and TTBR0 values for each processor:
	 * - TTBR0 uses a per-cpu L1 table covering [0, 3G)
	 * - TTBR1 uses a per-cpu L2 table covering [3G, 4G)
	 */
	pte64_t	L2_pte = set_pte(L2_paddr, PROT_PGTBL);
	for (i = 0; i < ncpu; i++) {
		lpae_ttbr0[i] = L1_paddr + (i * ARM_LPAE_L1_SIZE);
		lpae_ttbr1[i] = L2_paddr + (i * ARM_LPAE_L2_SIZE);

		/*
		 * Map the L2 table into the L1 table and create recursive L2 mapping.
		 */
		map_l2_cpu(i, L2_pte, ARM_LPAE_USER_END);
		CPU_L2_TABLE(i)[ARM_LPAE_KPTE_MAP] = L2_pte;
		L2_pte += ARM_LPAE_L2_SIZE;
	}

	/*
	 * Map the L1 tables
	 */
	L1_vaddr = lpae_map(~0L, L1_paddr, __PAGESIZE, PROT_PGTBL);

	/*
	 * Block map startup code so vstart_lpae can safely enable mmu.
	 * procnto uses syspage->un.arm.startup_base/startup_size to unmap it.
	 */
	startup_base = shdr->ram_paddr & ~ ARM_LPAE_BLOCK_MASK;
	startup_size = shdr->ram_size;

	unsigned	base = startup_base;
	unsigned	end  = startup_base + startup_size;

	while (base < end) {
		block_map(base, base, PROT_READ|PROT_EXEC);
		base += ARM_LPAE_BLOCK_SIZE;
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/mmu_lpae.c $ $Rev: 786270 $")
#endif

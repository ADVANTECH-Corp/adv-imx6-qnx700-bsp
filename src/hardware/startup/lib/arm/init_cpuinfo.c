/*
 * $QNXLicenseC:
 * Copyright 2015 QNX Software Systems. 
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
 * Initialize cpuinfo structures in the system page.
 * This code is hardware independant and should not have to be changed
 * changed by end users.
 */

static struct callout_rtn	*icache_rtn;
static struct callout_rtn	*dcache_rtn;

#define	ARM_CPACR_CP0	(3 << 0)
#define	ARM_CPACR_CP1	(3 << 2)
#define	ARM_CPACR_CP2	(3 << 4)
#define	ARM_CPACR_CP3	(3 << 6)
#define	ARM_CPACR_CP4	(3 << 8)
#define	ARM_CPACR_CP5	(3 << 10)
#define	ARM_CPACR_CP6	(3 << 12)
#define	ARM_CPACR_CP7	(3 << 14)
#define	ARM_CPACR_CP8	(3 << 16)
#define	ARM_CPACR_CP9	(3 << 18)
#define	ARM_CPACR_CP10	(3 << 20)
#define	ARM_CPACR_CP11	(3 << 22)
#define	ARM_CPACR_CP12	(3 << 24)
#define	ARM_CPACR_CP13	(3 << 26)
#define	ARM_CPACR_CP14	(3 << 28)
#define ARM_CPACR_VFP	(ARM_CPACR_CP10 | ARM_CPACR_CP11)

#define	MVFR0_A_SIMD_MASK	0xF
#define	MVFR0_A_SIMD_D16	1
#define	MVFR0_A_SIMD_D32	2
#define	ID_ISAR0_DIV_BIT	(24)
#define	ID_ISAR0_DIV_MASK	(0xF  << ID_ISAR0_DIV_BIT)

static void
fpsimd_check(struct cpuinfo_entry *cpu, unsigned cpunum)
{
	unsigned	tmp;

	/*
	 * Check for presence of VFP (cp10/11) and enable access if present.
	 * Access bits will not be written if coprocessor is not present.
	 */
	arm_cpacr_set(arm_cpacr_get() | ARM_CPACR_VFP);
	isb();
	tmp = arm_cpacr_get();
	if ((tmp & ARM_CPACR_VFP) == ARM_CPACR_VFP) {
		unsigned	mvfr0;
		unsigned	mvfr1;

		/*
		 * Indicate we have an FPU unit
		 */
		cpu->flags |= CPU_FLAG_FPU;

		/*
		 * Check if we have Neon or VFP only
		 */
		mvfr0 = arm_mvfr0_get();
		mvfr1 = arm_mvfr1_get();
		if ((mvfr1 & 0x00ffff00) != 0) {
			cpu->flags |= ARM_CPU_FLAG_NEON;
		}
		if ((mvfr0 & MVFR0_A_SIMD_MASK) == MVFR0_A_SIMD_D32) {
			cpu->flags |= ARM_CPU_FLAG_VFP_D32;
		}

		if (debug_flag) {
			kprintf("cpu%d: VFP-d%d FPSID=%x\n",
					cpunum, (mvfr0 & 0xf) * 16, arm_fpsid_get());
			if (cpu->flags & ARM_CPU_FLAG_NEON) {
				kprintf("cpu%d: NEON MVFR0=%x MVFR1=%x\n", cpunum, mvfr0, mvfr1);
			}
		}
	}
}

static inline unsigned
ccsidr_get(int level, int icache)
{
	unsigned	ccsidr;

	/*
	 * Set CSSELR for this level and cache type and read its CSSIDR value
	 */
	__asm__ __volatile__("mcr	p15, 2, %0, c0, c0, 0" : : "r"((level << 1) | icache));
	__asm__ __volatile__("mrc	p15, 1, %0, c0, c0, 0" : "=r"(ccsidr));
	return ccsidr;
}

static inline void
cache_size(unsigned cssidr, unsigned *nline, unsigned *lsize)
{
	unsigned	nsets = ((cssidr >> 13) & 0x7fff) + 1;
	unsigned	assoc = ((cssidr >> 3) & 0x3ff) + 1;

	*nline = nsets * assoc;
	*lsize = 1 << ((cssidr & 7) + 4);
}

static void
cache_check(struct cpuinfo_entry *cpu, unsigned cpunum)
{
	unsigned	clidr;
	int			i;

	/*
	 * Read CLIDR to figure out the cache configuration
	 */
	clidr = arm_clidr_get();

	if (debug_flag) {
		static const char *l1ip[] = {
			"unknown", "AIVIVT", "VIPT", "PIPT"
		};
		unsigned	ctr;

		/*
		 * Read CTR
		 */
		ctr = arm_ctr_get();

		kprintf("cpu%d: CTR=%x CWG=%d ERG=%d Dminline=%d Iminline=%d %s\n",
			cpunum,
			ctr,
			4 << ((ctr >> 24) & 0xf),
			4 << ((ctr >> 20) & 0xf),
			4 << ((ctr >> 16) & 0xf),
			4 << (ctr & 0xf),
			l1ip[(ctr >> 14) & 3]);

		kprintf("cpu%d: CLIDR=%w LoUU=%d LoC=%d LoUIS=%d\n",
			cpunum,
			clidr,
			(clidr >> 27) & 7,
			(clidr >> 24) & 7,
			(clidr >> 21) & 7);
	}

	for (i = 0; i < 8; i++, clidr >>= 3) {
		unsigned	type = clidr & 7;
		unsigned	ccsidr;
		unsigned	nline = 0;
		unsigned	lsize = 0;
		unsigned	flags;

		if (type == 0) {
			break;
		}
		if (type > 4) {
			crash("CLIDR reports reserved value");
		}

		flags = 0;
		if (type & 1) {
			ccsidr = ccsidr_get(i, 1);
			cache_size(ccsidr, &nline, &lsize);

			if (debug_flag) {
				kprintf("cpu%d: L%d Icache: %dx%d\n", cpunum, i+1, nline, lsize);
			}

			flags = CACHE_FLAG_INSTR;

			/*
			 * CP15 cache operations work on all levels so only call add_cache()
			 * for L1 caches.
			 */
			if (i == 0) {
				cpu->ins_cache = add_cache(cpu->ins_cache,
											flags,
											lsize,
											nline,
											icache_rtn);
			}
		}
		if ((type & 2) || type == 4) {
			ccsidr = ccsidr_get(i, 0);
			cache_size(ccsidr, &nline, &lsize);

			flags = (type == 4) ? CACHE_FLAG_UNIFIED : CACHE_FLAG_DATA;
			if (ccsidr & (1 << 30)) {
				flags |= CACHE_FLAG_WRITEBACK;
			}
			if (debug_flag) {
				kprintf("cpu%d: L%d Dcache: %dx%d %s\n", cpunum, i+1, nline, lsize, (flags & CACHE_FLAG_WRITEBACK) ? "WB" : "WT");
			}
			/*
			 * CP15 cache operations work on all levels so only call add_cache()
			 * for L1 caches.
			 */
			if (i == 0) {
				cpu->data_cache = add_cache(cpu->data_cache,
											flags,
											lsize,
											nline,
											dcache_rtn);
			}
		}
	}
}

void
init_one_cpuinfo(unsigned cpunum)
{
	struct cpuinfo_entry	*cpu;
	unsigned				cpuid;
	const struct armv_chip	*chip;

	cpu = &lsp.cpuinfo.p[cpunum];

	cpuid = arm_midr_get();
	cpu->cpu = cpuid;
	cpu->smp_hwcoreid = arm_mpidr_get();

	/*
	 * Work out what core we are running on
	 */
	chip = armv_chip_detect();
	if (chip == 0) {
		crash("Unsupported CPUID");
	}

	cpu->name = add_string(chip->name);
	cycles_per_loop = chip->cycles;

	/*
	 * Set the CPU speed
	 */
	if (cpu_freq != 0) {
		cpu->speed = cpu_freq / 1000000;
	} else {
		cpu->speed = (cycles_per_loop != 0) ? arm_cpuspeed() : 0;
	}

	if (debug_flag) {
		if ((cpuid >> 24) == 'A') {
			kprintf("cpu%d: %x: %s r%dp%d %dMHz\n",
					cpunum, cpuid,
					chip->name, (cpuid >> 20) & 15, cpuid & 15,
					cpu->speed);
		} else {
			kprintf("cpu%d: %x: %s %dMHz\n",
					cpunum, cpuid, chip->name, cpu->speed);
		}
		kprintf("cpu%d: MIPDIR=%x\n", cpunum, cpu->smp_hwcoreid);
	}

	/*
	 * Set up cpuinfo flags
	 */
	cpu->flags = ARM_CPU_FLAG_V7 | ARM_CPU_FLAG_V6 | ARM_CPU_FLAG_V6_ASID;
	if (shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL) {
		cpu->flags |= CPU_FLAG_MMU;
		if (paddr_bits > 32) {
			cpu->flags |= ARM_CPU_FLAG_LPAE;
		}
	}
	if ((cpu->smp_hwcoreid & (1u << 31)) != 0) {
		cpu->flags |= ARM_CPU_FLAG_V7_MP;
	}
	if (lsp.syspage.p->num_cpu > 1) {
		cpu->flags |= ARM_CPU_FLAG_SMP;
	}
	if (__clockcycles_incr_bit == 32) {
		cpu->flags |= ARM_CPU_FLAG_CC_INCR_BIT_32;
	}

	/*
	 * Check if the cpu implements sdiv/udiv instructions
	 */
	if ((arm_isar0_get() & ID_ISAR0_DIV_MASK) >> ID_ISAR0_DIV_BIT) {
		cpu->flags |= ARM_CPU_FLAG_IDIV;
	}

	/*
	 * Detect the processor caches and set up cache callouts
	 */
	cache_check(cpu, cpunum);

	/*
	 * Check FP/SIMD
	 */
	fpsimd_check(cpu, cpunum);

	if (cpunum == 0) {
		/*
		 * WARNING: we assume the power callouts are the same for ALL cpus
		 */
		if (chip->power) {
			add_callout(offsetof(struct callout_entry, power), chip->power);
		}
	}

	/*
	 * Set up GIC cpu interface if present.
	 * For CPU0, this is performed by arm_gic_init().
	 */
	if (cpunum != 0 && gic_cpu_base != NULL_PADDR) {
		arm_gic_cpu_init();
	}

	/*
	 * Do any extra CPU specific initialisation
	 */
	if (chip->setup) {
		chip->setup(cpu, cpunum, cpuid);
	}
}

void
init_cpuinfo()
{
	struct cpuinfo_entry	*cpu;
	unsigned				num;
	unsigned				i;

	num = lsp.syspage.p->num_cpu;
	if (num > 1) {
		extern unsigned	__cpu_flags;

		/*
		 * __cpu_flags is never explictly initialised in libstartup so set it
		 * to ensure mem_barrier/__cpu_membarrier executes the DMB operation
		 */
		__cpu_flags |= ARM_CPU_FLAG_SMP;
		icache_rtn = &cache_v7mp_i;
		dcache_rtn = &cache_v7mp_d;
	} else {
		icache_rtn = &cache_v7up_i;
		dcache_rtn = &cache_v7up_d;
	}

	cpu = set_syspage_section(&lsp.cpuinfo, sizeof(*lsp.cpuinfo.p) * num);
	for (i = 0; i < num; i++) {
		cpu[i].ins_cache  = system_icache_idx;
		cpu[i].data_cache = system_dcache_idx;
	}

	if (shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL) {
		int	i;

		/*
		 * Allocate and map the trap vector table
		 */
		paddr32_t	pa = calloc_ram(__PAGESIZE, __PAGESIZE);

		for (i = 0; i < num; i++) {
			mmu_map_cpu(i, TRAP_VECTORS, pa, PROT_READ|PROT_WRITE|PROT_EXEC);
		}

		/*
		 * Set up the trap entry point to be "ldr pc, [pc, #0x18]"
		 * These jump slot addresses are all zero, so until these have
		 * been properly set up, any exception will result in a loop.
		 */
		for (i = 0; i < 8; i++) {
			*((unsigned *)pa + i) = 0xe59ff018;
		}
	}

	init_one_cpuinfo(0);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/init_cpuinfo.c $ $Rev: 784257 $")
#endif

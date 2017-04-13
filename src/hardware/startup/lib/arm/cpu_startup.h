/*
 * $QNXLicenseC:
 * Copyright 2015-2016 QNX Software Systems.
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

#include <arm/inout.h>
#include <arm/inline.h>
#include <arm/mmu.h>

/********************************************************************
*********************************************************************

	These definitions are required for the system independent code.

*********************************************************************
********************************************************************/

#define KERCALL_SEQUENCE(name)	uint32_t name[] = {			\
			0xef000000,		/* swi (syscall no. in ip)	*/	\
			0xe7ffffef		/* undefined instruction	*/	\
}

#define CPU_SYSPAGE_TYPE	SYSPAGE_ARM

struct cpu_local_syspage {
	SYSPAGE_SECTION(arm_boxinfo);
	SYSPAGE_SECTION(arm_cpu);
};

#define BOOTSTRAPS_RUN_ONE_TO_ONE	0

#define CPU_COMMON_OPTIONS_STRING	"x"

/********************************************************************
*********************************************************************

	Everything below is specific to the ARM.

*********************************************************************
********************************************************************/

/*
 * ------------------------------------------------------------------
 * Generic ARM support
 * ------------------------------------------------------------------
 */

extern struct callout_rtn	cache_v7up_i;
extern struct callout_rtn	cache_v7up_d;
extern struct callout_rtn	cache_v7mp_i;
extern struct callout_rtn	cache_v7mp_d;
extern struct callout_rtn	power_v7_wfi;

extern uintptr_t		callout_io_map_armv7_dev(unsigned size, paddr_t *phys);

extern void				board_cpu_startup(void);
extern void				board_cpu_startnext(void);

/*
 * MMU mapping support.
 *
 * Implementation is different for ARMv7 and LPAE.
 */
extern void			(*vstart)(uintptr_t sysp, unsigned pc, unsigned cpu);
extern uintptr_t	(*mmu_map)(uintptr_t va, paddr_t pa, size_t sz, unsigned flags);
extern void			(*mmu_map_cpu)(int cpu, uintptr_t va, paddr_t pa, unsigned flags);
extern void			(*mmu_elf_map)(uintptr_t va, paddr32_t pa, size_t sz, unsigned prot);
extern paddr_t		(*mmu_vaddr_to_paddr)(uintptr_t va);

extern unsigned long	arm_cpuspeed();
extern void				arm_add_clock_cycles(struct callout_rtn *callout, int incr_bit);
extern void				arm_enable_mmu(unsigned long arm_tlb);
extern void				arm_v7_dcache_flush();
extern void             arm_v7_icache_invalidate();
extern void				arm_v7_enable_cache();
extern void				arm_v7_disable_cache_mmu();
extern int				arm_mp_cpuid();

extern int				cycles_per_loop;
extern uintptr_t		L1_paddr;
extern uintptr_t		L1_vaddr;
extern uintptr_t		L2_paddr;
extern uintptr_t		startup_base;
extern unsigned			startup_size;

extern unsigned			sctlr_clr;
extern unsigned			sctlr_set;

#define	PROT_DEVICE		0x80000000
#define	PROT_PGTBL		0x40000000
#define	TRAP_VECTORS	0xffff0000

extern uint32_t         boot_regs[4];   /* r0-r3 at time of entry to _start */

/*
 * ------------------------------------------------------------------
 * ARMv7 MMU specific support
 * ------------------------------------------------------------------
 */

extern void			init_mmu_v7(void);
extern void			vstart_v7(uintptr_t sysp, unsigned pc, unsigned cpu);

extern unsigned		v7_prrr;
extern unsigned		v7_nmrr;
extern unsigned		v7_ttbr;

/*
 * ------------------------------------------------------------------
 * LPAE MMU specific support
 * ------------------------------------------------------------------
 */

extern void			init_mmu_lpae(void);
extern void			vstart_lpae(uintptr_t sysp, unsigned pc, unsigned cpu);

extern unsigned		lpae_mair0;
extern unsigned		lpae_mair1;

/*
 * ------------------------------------------------------------------
 * CPU specific configuration information
 * ------------------------------------------------------------------
 */
struct armv_chip {
	unsigned					cpuid;		// bits 4-15 of CP15 C1
	const char					*name;
	int							cycles;		// cycles per arm_cpuspeed() loop
	const struct callout_rtn	*power;		// idle callout
	void						(*setup)(struct cpuinfo_entry *cpu, unsigned cpunum, unsigned cpuid);
	const struct armv_chip		*(*detect)(void);
};

extern const struct armv_chip	*armv_list[];
extern const struct armv_chip	*armv_chip_detect();
extern const struct armv_chip	*armv_chip;

extern const struct armv_chip	armv_chip_a7;
extern const struct armv_chip	armv_chip_a8;
extern const struct armv_chip	armv_chip_a9;
extern const struct armv_chip	armv_chip_a15;
extern const struct armv_chip	armv_chip_a57;

/*
 * ------------------------------------------------------------------
 * OMAP UART support
 * ------------------------------------------------------------------
 */
extern void init_omap(unsigned channel, const char *init, const char *defaults);
extern void put_omap(int);

/*
 * ------------------------------------------------------------------
 * PrimeCell PL011 UART support
 * ------------------------------------------------------------------
 */
extern void init_pl011(unsigned, const char *, const char *);
extern void put_pl011(int);

extern struct callout_rtn	display_char_pl011;
extern struct callout_rtn	poll_key_pl011;
extern struct callout_rtn	break_detect_pl011;

/*
 * ------------------------------------------------------------------
 * Freescale MX1 UART support
 * ------------------------------------------------------------------
 */
extern void init_mx1(unsigned channel, const char *init, const char *defaults);
extern void put_mx1(int);

extern struct callout_rtn	display_char_mx1;
extern struct callout_rtn	poll_key_mx1;
extern struct callout_rtn	break_detect_mx1;

/*
 * ------------------------------------------------------------------
 * Freescale LPUART support
 * ------------------------------------------------------------------
 */
extern void init_lpuart_le(unsigned channel, const char *init, const char *defaults);
extern void init_lpuart_be(unsigned channel, const char *init, const char *defaults);
extern void put_lpuart_le(int);
extern void put_lpuart_be(int);

extern struct callout_rtn	display_char_lpuart_le;
extern struct callout_rtn	display_char_lpuart_be;
extern struct callout_rtn	poll_key_lpuart_le;
extern struct callout_rtn	poll_key_lpuart_be;
extern struct callout_rtn	break_detect_lpuart_le;
extern struct callout_rtn	break_detect_lpuart_be;

/*
 * ------------------------------------------------------------------
 * Cortex A8 support
 * ------------------------------------------------------------------
 */
extern void		armv_setup_a8(struct cpuinfo_entry *, unsigned, unsigned);
extern void		arm_a8_jacinto_enable_cache();
extern void		arm_a8_jacinto_setup_auxcr();

/*
 * ------------------------------------------------------------------
 * Cortex A9 support
 * ------------------------------------------------------------------
 */
extern const struct armv_chip	*armv_detect_a9(void);

extern paddr_t	mpcore_scu_base;

extern void		armv_setup_a9(struct cpuinfo_entry *, unsigned, unsigned);
extern int		a9_actlr_allowed;
extern int		a9_diag_allowed;

extern paddr_t				pl310_base;
extern struct callout_rtn	cache_pl310;
extern struct callout_rtn	smp_spin_pl310;

extern void					init_qtime_a9gt(unsigned timer_freq);
extern struct callout_rtn	timer_load_a9gt;
extern struct callout_rtn	timer_value_a9gt;
extern struct callout_rtn	timer_reload_a9gt;
extern struct callout_rtn	clock_cycles_a9gt;

/*
 * ------------------------------------------------------------------
 * Cortex A15 support
 * ------------------------------------------------------------------
 */
extern const struct armv_chip	*armv_detect_a15(void);

extern void		armv_setup_a15(struct cpuinfo_entry *, unsigned, unsigned);

/*
 * ------------------------------------------------------------------
 * Cortex A57 support
 * ------------------------------------------------------------------
 */
extern void		armv_setup_a57(struct cpuinfo_entry *, unsigned, unsigned);

/*
 * ------------------------------------------------------------------
 * ARM Generic Interrupt Controller Support
 * ------------------------------------------------------------------
 */
extern struct callout_rtn	interrupt_id_gic;
extern struct callout_rtn	interrupt_eoi_gic;
extern struct callout_rtn	interrupt_mask_gic;
extern struct callout_rtn	interrupt_unmask_gic;
extern struct callout_rtn	interrupt_config_gic;

extern struct callout_rtn	sendipi_gic;

extern paddr_t				gic_dist_base;
extern paddr_t				gic_cpu_base;
extern void					arm_gic_init(paddr_t, paddr_t);
extern void					arm_gic_cpu_init(void);
extern int                  arm_gic_num_spis(void);

/*
 * ------------------------------------------------------------------
 * HVC debug callout support
 * ------------------------------------------------------------------
 */
extern void                 init_hvc(unsigned channel, const char *init, const char *defaults);
extern void                 put_hvc(int c);
extern struct callout_rtn	display_char_hvc;
extern struct callout_rtn	poll_key_hvc;


/*
 * ------------------------------------------------------------------
 * Dummy debug callout support
 * ------------------------------------------------------------------
 */
void 						init_null(unsigned, const char *, const char *);
void 						put_null(int);
extern struct callout_rtn	display_char_null;
extern struct callout_rtn	poll_key_null;
extern struct callout_rtn	break_detect_null;

/*
 * ------------------------------------------------------------------
 * Primecell SP804 timer support
 * ------------------------------------------------------------------
 */
extern struct callout_rtn	timer_load_sp804;
extern struct callout_rtn	timer_value_sp804;
extern struct callout_rtn	timer_reload_sp804;

/*
 * ------------------------------------------------------------------
 * ARMv7 Generic Timer support
 * ------------------------------------------------------------------
 */
extern struct callout_rtn	timer_load_v7gt;
extern struct callout_rtn	timer_value_v7gt;
extern struct callout_rtn	timer_reload_v7gt;
extern struct callout_rtn	clock_cycles_v7gt;

extern void					init_qtime_v7gt(unsigned timer_freq, unsigned intr);

/*
 * ------------------------------------------------------------------
 * OMAP4 support
 * ------------------------------------------------------------------
 */
extern void					init_qtime_omap4430(void);

extern struct callout_rtn	timer_load_omap4430;
extern struct callout_rtn	timer_value_omap4430;
extern struct callout_rtn	timer_reload_omap4430;

extern struct callout_rtn	clock_cycles_omap4xx;

extern struct callout_rtn	reboot_4430;

extern struct callout_rtn	interrupt_id_omap4_sdma;
extern struct callout_rtn	interrupt_eoi_omap4_sdma;
extern struct callout_rtn	interrupt_mask_omap4_sdma;
extern struct callout_rtn	interrupt_unmask_omap4_sdma;

extern struct callout_rtn	interrupt_id_omap4_gpio;
extern struct callout_rtn	interrupt_eoi_omap4_gpio;
extern struct callout_rtn	interrupt_mask_omap4_gpio;
extern struct callout_rtn	interrupt_unmask_omap4_gpio;

extern struct callout_rtn	interrupt_id_omap4_wugen;
extern struct callout_rtn	interrupt_eoi_omap4_wugen;
extern struct callout_rtn	interrupt_mask_omap4_wugen;
extern struct callout_rtn	interrupt_unmask_omap4_wugen;

/*
 * ------------------------------------------------------------------
 * OMAP5 support
 * ------------------------------------------------------------------
 */
extern struct callout_rtn	interrupt_id_omap5_sdma;
extern struct callout_rtn	interrupt_eoi_omap5_sdma;
extern struct callout_rtn	interrupt_mask_omap5_sdma;
extern struct callout_rtn	interrupt_unmask_omap5_sdma;

/*
 * ------------------------------------------------------------------
 * DRA6xx (J5) PCIe related interrupt callouts
 * ------------------------------------------------------------------
 */
extern struct callout_rtn interrupt_id_dra6xx_pci_msi;
extern struct callout_rtn interrupt_eoi_dra6xx_pci_msi;
extern struct callout_rtn interrupt_mask_dra6xx_pci_msi;
extern struct callout_rtn interrupt_unmask_dra6xx_pci_msi;
extern struct callout_rtn interrupt_id_dra6xx_pci_intpin;
extern struct callout_rtn interrupt_eoi_dra6xx_pci_intpin;
extern struct callout_rtn interrupt_mask_dra6xx_pci_intpin;
extern struct callout_rtn interrupt_unmask_dra6xx_pci_intpin;
extern struct callout_rtn interrupt_id_dra6xx_pci_err;
extern struct callout_rtn interrupt_eoi_dra6xx_pci_err;
extern struct callout_rtn interrupt_mask_dra6xx_pci_err;
extern struct callout_rtn interrupt_unmask_dra6xx_pci_err;
extern struct callout_rtn interrupt_id_dra6xx_pci_pmre;
extern struct callout_rtn interrupt_eoi_dra6xx_pci_pmre;
extern struct callout_rtn interrupt_mask_dra6xx_pci_pmre;
extern struct callout_rtn interrupt_unmask_dra6xx_pci_pmre;

/*
 * ------------------------------------------------------------------
 * Renesas support
 * ------------------------------------------------------------------
 */
extern void init_scif(unsigned, const char *, const char *);
extern void put_scif(int);
extern struct callout_rtn	display_char_scif;
extern struct callout_rtn	poll_key_scif;
extern struct callout_rtn	break_detect_scif;

extern struct callout_rtn	interrupt_id_rcar_gpio;
extern struct callout_rtn	interrupt_eoi_rcar_gpio;
extern struct callout_rtn	interrupt_mask_rcar_gpio;
extern struct callout_rtn	interrupt_unmask_rcar_gpio;

/*
 * ------------------------------------------------------------------
 * TI DM814x/816x Support
 * ------------------------------------------------------------------
 */
extern void					init_qtime_dm816x(unsigned timer_freq);

extern struct callout_rtn	timer_load_dm816x;
extern struct callout_rtn	timer_value_dm816x;
extern struct callout_rtn	timer_reload_dm816x;

extern struct callout_rtn	interrupt_id_dm816x;
extern struct callout_rtn	interrupt_eoi_dm816x;
extern struct callout_rtn	interrupt_mask_dm816x;
extern struct callout_rtn	interrupt_unmask_dm816x;

extern struct callout_rtn	interrupt_id_dm814x_gpio;
extern struct callout_rtn	interrupt_eoi_dm814x_gpio;
extern struct callout_rtn	interrupt_mask_dm814x_gpio;
extern struct callout_rtn	interrupt_unmask_dm814x_gpio;

extern struct callout_rtn	interrupt_id_dm814x_msi;
extern struct callout_rtn	interrupt_eoi_dm814x_msi;
extern struct callout_rtn	interrupt_mask_dm814x_msi;
extern struct callout_rtn	interrupt_unmask_dm814x_msi;

extern struct callout_rtn	interrupt_id_dm814x_edma;
extern struct callout_rtn	interrupt_eoi_dm814x_edma;
extern struct callout_rtn	interrupt_mask_dm814x_edma;
extern struct callout_rtn	interrupt_unmask_dm814x_edma;

extern struct callout_rtn	reboot_dm816x;

/*
 * ------------------------------------------------------------------
 * nVidia Tegra2/3 Support
 * ------------------------------------------------------------------
 */
extern struct callout_rtn	interrupt_id_tegra2_msi;
extern struct callout_rtn	interrupt_eoi_tegra2_msi;
extern struct callout_rtn	interrupt_mask_tegra2_msi;
extern struct callout_rtn	interrupt_unmask_tegra2_msi;


#include "common_arm/armboth_startup.h"

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/cpu_startup.h $ $Rev: 805440 $")
#endif

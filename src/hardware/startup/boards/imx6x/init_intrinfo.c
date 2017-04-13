/*
 * $QNXLicenseC: 
 * Copyright 2012 QNX Software Systems.  
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
 * i.MX6x General Interrupt Controller support.
 */

#include "startup.h"
#include <arm/mx6x.h>
#include <arm/mpcore.h>

extern struct callout_rtn	interrupt_id_mx6x_gpio_low;
extern struct callout_rtn	interrupt_eoi_mx6x_gpio_low;
extern struct callout_rtn	interrupt_mask_mx6x_gpio_low;
extern struct callout_rtn	interrupt_unmask_mx6x_gpio_low;

extern struct callout_rtn	interrupt_id_mx6x_gpio_high;
extern struct callout_rtn	interrupt_eoi_mx6x_gpio_high;
extern struct callout_rtn	interrupt_mask_mx6x_gpio_high;
extern struct callout_rtn	interrupt_unmask_mx6x_gpio_high;

extern struct callout_rtn	interrupt_id_mx6x_msi;
extern struct callout_rtn	interrupt_eoi_mx6x_msi;
extern struct callout_rtn	interrupt_mask_mx6x_msi;
extern struct callout_rtn	interrupt_unmask_mx6x_msi;

static paddr_t mx6x_gpio1_base = MX6X_GPIO1_BASE;
static paddr_t mx6x_gpio2_base = MX6X_GPIO2_BASE;
static paddr_t mx6x_gpio3_base = MX6X_GPIO3_BASE;
static paddr_t mx6x_gpio4_base = MX6X_GPIO4_BASE;
static paddr_t mx6x_gpio5_base = MX6X_GPIO5_BASE;
static paddr_t mx6x_gpio6_base = MX6X_GPIO6_BASE;
static paddr_t mx6x_gpio7_base = MX6X_GPIO7_BASE;

/*
 * There are 8 sets of 3 32 bit MSI related registers (12 bytes) arranged as
 * follows
 *
 * 		PCIE_PL_MSICIn_ENB
 * 		PCIE_PL_MSICIn_MASK
 * 		PCIE_PL_MSICIn_STATUS
 *
 * set 0 - 0x1FFC828 thru 0x1FFC830
 * set 1 - 0x1FFC834 thru 0x1FFC83C
 * set 2 - 0x1FFC840 thru 0x1FFC848
 * set 3 - 0x1FFC84C thru 0x1FFC854
 * set 4 - 0x1FFC858 thru 0x1FFC860
 * set 5 - 0x1FFC864 thru 0x1FFC86C
 * set 6 - 0x1FFC870 thru 0x1FFC878
 * set 7 - 0x1FFC87C thru 0x1FFC884
 *
 * Prioritizing them within the callouts will be done as follows
 *
 * highest priority - bit 0 in set 0 (return 0)
 *     :
 *               - bit 31 in set 0 (return 31)
 *               - bit 0 in set 1 (return 32)
 *     :
 *               - bit 31 in set 6 (return 223)
 *               - bit 0 in set 7 (return 224)
 *     :
 * 2nd lowest priority - bit 31 in set 7 (return 255)
 * lowest priority - (if no bits set in any of the 8 status registers) return 256 corresponding to PCI INTD
 *
 * This prioritization scheme ensures that MSI/MSI-X vectors are assigned in highest
 * priority order on a FIFO basis by the PCI server
 *
 */
static paddr_t mx6x_msi_base = MX6X_PCIE_REG_BASE + 0x828;

const static struct startup_intrinfo	intrs[] = {

	/* ARM General Interrupt Controller */
	{	.vector_base     = _NTO_INTR_CLASS_EXTERNAL,
		.num_vectors     = 160,
		.cascade_vector  = _NTO_INTR_SPARE,			
		.cpu_intr_base   = 0,						
		.cpu_intr_stride = 0,						
		.flags           = 0,						
        .id = { INTR_GENFLAG_LOAD_SYSPAGE,	0, &interrupt_id_gic },
        .eoi = { INTR_GENFLAG_LOAD_SYSPAGE | INTR_GENFLAG_LOAD_INTRMASK, 0, &interrupt_eoi_gic },
		.mask            = &interrupt_mask_gic,
		.unmask          = &interrupt_unmask_gic,
		.config          = &interrupt_config_gic,
		.patch_data      = &mpcore_scu_base,
	},

	/* MSI/MSI-X interrupts (256) + PCI intpin D (1) starting at 0x1000) */
	{   .vector_base = 0x1000,		// vector base (MSI/MSI-X 0x1000 - 0x10FF, INTD=0x1100)
		.num_vectors = 256 + 1,		// 256 MSI's plus INTD
		.cascade_vector = 152,
		.cpu_intr_base = 0,			// CPU vector base (MSI/MSI-X from 0 - 255, INTD=256)
		.cpu_intr_stride = 0,
		.flags = INTR_FLAG_MSI,
		.id = {INTR_GENFLAG_NOGLITCH, 0, &interrupt_id_mx6x_msi},
		.eoi = {INTR_GENFLAG_LOAD_INTRMASK, 0, &interrupt_eoi_mx6x_msi},
		.mask = &interrupt_mask_mx6x_msi,
		.unmask = &interrupt_unmask_mx6x_msi,
		.config = 0,
		.patch_data = &mx6x_msi_base,
	},

	/* GPIO 1 low interrupts (160 -175) */
	{	160,					// vector base
		16,					// number of vectors
		98,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio1_base,
	},
	/* GPIO 1 high interrupts (176 -191) */
	{	176,					// vector base
		16,					// number of vectors
		99,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio1_base,
	},
	// GPIO 2 low interrupts (192 -207)
	{	192,					// vector base
		16,					// number of vectors
		100,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio2_base,
	},
	// GPIO 2 high interrupts (208 -223)
	{	208,					// vector base
		16,					// number of vectors
		101,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio2_base,
	},
	// GPIO 3 low interrupts (224 -239)
	{	224,					// vector base
		16,					// number of vectors
		102,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio3_base,
	},
	// GPIO 3 high interrupts (240 -255)
	{	240,					// vector base
		16,					// number of vectors
		103,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio3_base,
	},
	// GPIO 4 low interrupts (256 -271)
	{	256,					// vector base
		16,					// number of vectors
		104,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio4_base,
	},
	// GPIO 4 high interrupts (272 -287)
	{	272,					// vector base
		16,					// number of vectors
		105,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio4_base,
	},
	// GPIO 5 low interrupts (288 -303)
	{	288,					// vector base
		16,					// number of vectors
		106,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio5_base,
	},
	// GPIO 5 high interrupts (304 -319)
	{	304,					// vector base
		16,					// number of vectors
		107,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio5_base,
	},
	// GPIO 6 low interrupts (320 -335)
	{	320,					// vector base
		16,					// number of vectors
		108,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio6_base,
	},
	// GPIO 6 high interrupts (336 -351)
	{	336,					// vector base
		16,					// number of vectors
		109,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio6_base,
	},
	// GPIO 7 low interrupts (352 -367)
	{	352,					// vector base
		16,					// number of vectors
		110,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_low },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_low },
		&interrupt_mask_mx6x_gpio_low,		// mask   callout
		&interrupt_unmask_mx6x_gpio_low,	// unmask callout
		0,					// config callout
		&mx6x_gpio7_base,
	},
	// GPIO 7 high interrupt (368 - 383)
	{	368,					// vector base
		16,					// number of vectors
		111,					// cascade vector
		0,					// CPU vector base
		0,					// CPU vector stride
		0,					// flags

		{ 0, 0, &interrupt_id_mx6x_gpio_high },
		{ INTR_GENFLAG_LOAD_INTRMASK,	0, &interrupt_eoi_mx6x_gpio_high },
		&interrupt_mask_mx6x_gpio_high,		// mask   callout
		&interrupt_unmask_mx6x_gpio_high,	// unmask callout
		0,					// config callout
		&mx6x_gpio7_base,
	},
};

void init_intrinfo()
{
	/* mpcore_scu_base is set in function armv_detect_a9 */
	unsigned	gic_dist = mpcore_scu_base + MPCORE_GIC_DIST_BASE;
	unsigned	gic_cpu  = mpcore_scu_base + MPCORE_GIC_CPU_BASE;

	/*
	 * Initialise GIC distributor and our cpu interface
	 */
	arm_gic_init(gic_dist, gic_cpu);

	add_interrupt_array(intrs, sizeof(intrs));
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/imx6x/init_intrinfo.c $ $Rev: 811218 $")
#endif

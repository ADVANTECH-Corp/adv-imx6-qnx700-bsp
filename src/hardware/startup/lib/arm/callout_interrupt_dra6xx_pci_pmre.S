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


/*
 * Freescale DRA6xx (J5/J5 eco) PCI Power Management and Reset event interrupt callouts.
 *
 * interrupt_id_* and interrupt_eoi_* are copied and intermixed with other
 * kernel code during initialisation.
 *
 * They do not follow normal calling conventions, and must fall through
 * to the end, rather than attempting to perform a return instruction.
 *
 * The INTR_GENFLAG_* bits in the intrinfo_entry defines which of the
 * following values can be loaded on entry to these code fragments:
 *
 *	r5 - holds the syspageptr				(INTR_GENFLAG_SYSPAGE  set)
 *	r6 - holds the intrinfo_entry pointer	(INTR_GENFLAG_INTRINFO set)
 *	r7 - holds the interrupt mask count		(INTR_GENFLAG_INTRMASK set)
 *
 * The interrupt_id_* routine returns the (controller-relative) level in r4
 *
 * NOTE: The mask register is really a mask register and not an enable. That is
 *       a value of 1 masks and 0 unmasks
 */

#include "callout.ah"

#define DRA6XX_EOI			0x50
#define DRA6XX_PMRST_ISR	0x1D4		/* offset to status register */
#define DRA6XX_PMRST_IER	0x1D8		/* offset to enable register */
#define DRA6XX_PMRST_IMR	0x1DC		/* offset to mask (enable clear) register */

/* the first register in map base is offset 0 */
#define REG_MAP_SIZE	(DRA6XX_PMRST_IMR + 4)

/*
 * -----------------------------------------------------------------------
 * Patch callout code (for PCI Power Management and Reset event IRQ's)
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 * -----------------------------------------------------------------------
 */
interrupt_patch_pci_pmre:
	stmdb   sp!,{r4,lr}
	ldr     r1, [sp, #8]
	ldr     r1, [r1]
	add     r4, r0, r2			// address of callout routine

	ldr     r0, Lpaddr
	cmp     r0, r1
	ldreq   r0, Lvaddr
	beq     1f

	str     r1, Lpaddr

	/*
	 * Map interrupt controller registers
	 */
	mov     r0, #REG_MAP_SIZE	// size of interrupt registers
	ldr     r1, [sp, #8]		// pass the address of the paddr
	bl      callout_io_map_indirect
	str     r0, Lvaddr

1:
	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH  r4, r0, r1, r2, ip
	ldmia   sp!,{r4,pc}

Lpaddr:	.word	-1
Lvaddr:	.word	-1

/*
 * -----------------------------------------------------------------------
 * Identify PCI Power Management and Reset event interrupt source.
 *
 * Returns interrupt number in r4
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_id_dra6xx_pci_pmre, 0, interrupt_patch_pci_pmre)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	mov     r1, #0
	mov     r4, #1
	/*
	 * Read Interrupt Mask and Status
	 */
	ldr     r3, [ip, #DRA6XX_PMRST_IER]
	ldr     r2, [ip, #DRA6XX_PMRST_ISR]
	/* clear any masked and reserved bits from the status register */
	and     r2, r2, #0xF
	ands    r2, r2, r3
	beq     done

	/* prioritization is MSb to LSb */
	clz     r2, r2
	rsb     r4, r2, #31

	/*
	 * Mask the interrupt source and clear the status
	 */
	mov     r2, #1
	mov     r2, r2, lsl r4
	str     r2, [ip, #DRA6XX_PMRST_IMR]
	str     r2, [ip, #DRA6XX_PMRST_ISR]

done:

CALLOUT_END(interrupt_id_dra6xx_pci_pmre)

/*
 * -----------------------------------------------------------------------
 * Acknowledge specified PCI Power Management and Reset event interrupt
 *
 * On entry:
 *	r4 contains the interrupt number
 *	r7 contains the interrupt mask count
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_eoi_dra6xx_pci_pmre, 0, interrupt_patch_pci_pmre)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov     ip,     #0x000000ff
	orr     ip, ip, #0x0000ff00
	orr     ip, ip, #0x00ff0000
	orr     ip, ip, #0xff000000

	/* only need to do something if a PCI Power Management and Reset event (0 - 3) */
	cmp     r4, #4
	bge     0f

	/* PCI Power Management and Reset event's are on PCIINT0 */
	mov		r3, #0
	str		r3, [ip, #DRA6XX_EOI]

	/* Only unmask interrupt if mask count is zero */
	teq     r7, #0
	bne     0f

	ldr     r3, [ip, #DRA6XX_PMRST_IER]
	mov     r2, #1
	orr     r3, r3, r2, lsl r4
	str     r3, [ip, #DRA6XX_PMRST_IER]
0:

CALLOUT_END(interrupt_eoi_dra6xx_pci_pmre)

/*
 * -----------------------------------------------------------------------
 * Mask specified PCI Power Management and Reset event interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_mask_dra6xx_pci_pmre, 0, interrupt_patch_pci_pmre)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov     ip,     #0x000000ff
	orr     ip, ip, #0x0000ff00
	orr     ip, ip, #0x00ff0000
	orr     ip, ip, #0xff000000

	/* only need to do something if an PCI Power Management and Reset event (0 - 3) */
	cmp     r1, #4
	movge	r0, #-1
	bge     0f

	ldr     r3, [ip, #DRA6XX_PMRST_IMR]
	mov     r2, #1
	orr     r3, r3, r2, lsl r1
	str     r3, [ip, #DRA6XX_PMRST_IMR]
	mov     r0, #0
0:
	mov     pc, lr

CALLOUT_END(interrupt_mask_dra6xx_pci_pmre)

/*
 * -----------------------------------------------------------------------
 * Unmask specified PCI Power Management and Reset event interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_unmask_dra6xx_pci_pmre, 0, interrupt_patch_pci_pmre)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov     ip,     #0x000000ff
	orr     ip, ip, #0x0000ff00
	orr     ip, ip, #0x00ff0000
	orr     ip, ip, #0xff000000

	/* only need to do something if an PCI Power Management and Reset event (0 - 3) */
	cmp     r1, #4
	movge	r0, #-1
	bge     0f

	ldr     r3, [ip, #DRA6XX_PMRST_IER]
	mov     r2, #1
	orr     r3, r3, r2, lsl r1
	str     r3, [ip, #DRA6XX_PMRST_IER]
	mov     r0, #0
0:
	mov     pc, lr

CALLOUT_END(interrupt_unmask_dra6xx_pci_pmre)



#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/callout_interrupt_dra6xx_pci_pmre.S $ $Rev: 786976 $";
.previous
#endif
#endif

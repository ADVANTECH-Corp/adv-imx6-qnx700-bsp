/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, 2010, 2011, 2012, 2013, 2014 QNX Software Systems.
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
 * Freescale MCIMX6X (i.MX6) specific MSI interrupt callouts.
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
#include <arm/mx6x.h>

#define MX6X_MSI_IER	0		/* offset to enable register */
#define MX6X_MSI_IMR	4		/* offset to mask register */
#define MX6X_MSI_ISR	8		/* offset to status register */
/*
 * -----------------------------------------------------------------------
 * Patch callout code (for PCIe MSI's)
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 * -----------------------------------------------------------------------
 */
interrupt_patch_msi:
	stmdb   sp!,{r4,lr}
	add     r4, r0, r2			// address of callout routine

	/*
	 * Check if we have already mapped this paddr
	 */
	ldr 	r0, [sp, #8]	 	// patch_data
	ldrd 	r0, [r0] 			// r0,r1 = paddr
	adr 	r2, Lpaddr
	ldrd 	r2, [r2] 			// r2,r3 = Lpaddr
	cmp 	r0, r2
	cmpeq 	r1, r3
	ldreq 	r0, Lvaddr 			// get previous mapped vaddr
	beq 	1f
	adr 	r2, Lpaddr
	strd 	r0, [r2]

	/*
	 * Map interrupt controller registers
	 */
	mov     r0, #0x20			// size of interrupt registers
	adr		r1, Lpaddr
	bl      callout_io_map_indirect
	str     r0, Lvaddr

1:
	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH  r4, r0, r1, r2, ip
	ldmia   sp!,{r4,pc}

		.align	3
Lpaddr:	.quad	0xffffffff
Lvaddr:	.word	-1

/*
 * -----------------------------------------------------------------------
 * Identify MSI interrupt source.
 *
 * Returns interrupt number in r4
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_id_mx6x_msi, 0, interrupt_patch_msi)
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
start:
    ldr     r3, [ip, #MX6X_MSI_IMR]
    ldr     r2, [ip, #MX6X_MSI_ISR]
    /* clear any masked bits from the status register */
	bics    r2, r2, r3
    bne     scan
	/*try the next set */
	add     r1, #32
	add     ip, ip, #12		// 12 bytes per set
	cmp     r1, #256
	blt     start
	/* we will assume that it must be PCI INTD if no MSI status registers are set */
	mov     r4, #256
	b       done

	/*
	 * Scan for first set bit
	 * Priority is bit 0 thru 31
	 */
scan:
	tst     r2, r4
	bne     found
	movs    r4, r4, asl #1
	bcc     scan

	/* technically we should never get here, but if we do ... */
	mov     r4, #-1
	b       done

found:
	/*
	 * Mask the interrupt source and clear the status
	 */
	orr     r3, r3, r4
	str     r3, [ip, #MX6X_MSI_IMR]
    str     r4, [ip, #MX6X_MSI_ISR]
    /* prepare return value */
    clz     r3, r4
    add     r2, r1, #31
    sub     r4, r2, r3
done:

CALLOUT_END(interrupt_id_mx6x_msi)

/*
 * -----------------------------------------------------------------------
 * Acknowledge specified MSI interrupt
 *
 * On entry:
 *	r4 contains the interrupt number
 *	r7 contains the interrupt mask count
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_eoi_mx6x_msi, 0, interrupt_patch_msi)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov     ip,     #0x000000ff
	orr     ip, ip, #0x0000ff00
	orr     ip, ip, #0x00ff0000
	orr     ip, ip, #0xff000000

	/* only need to do something if an MSI (0 - 255) */
	cmp     r4, #256
	bge     0f

    /* Only unmask interrupt if mask count is zero */
    teq     r7, #0
	bne     0f

	/* point to the correct register set */
	mov     r2, r4, asr #5
	mov     r3, #12
	smlabb  ip, r3, r2, ip

	/* get the bit to unmask */
	mov     r3, r2, asl #5
	sub     r3, r4, r3

    ldr     r1, [ip, #MX6X_MSI_IMR]
	mov     r2, #1
	bic     r3, r1, r2, lsl r3
    str     r3, [ip, #MX6X_MSI_IMR]
0:

CALLOUT_END(interrupt_eoi_mx6x_msi)

/*
 * -----------------------------------------------------------------------
 * Mask specified MSI interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_mask_mx6x_msi, 0, interrupt_patch_msi)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov     ip,     #0x000000ff
	orr     ip, ip, #0x0000ff00
	orr     ip, ip, #0x00ff0000
	orr     ip, ip, #0xff000000

	/* only need to do something if an MSI (0 - 255) */
	cmp     r1, #256
	bge     0f

	/* point to the correct register set */
	mov     r2, r1, asr #5
	mov     r3, #12
	smlabb  ip, r3, r2, ip

	/* get the bit to mask */
	mov     r3, r2, asl #5
	sub     r3, r1, r3

    ldr     r1, [ip, #MX6X_MSI_IMR]
	mov     r2, #1
	orr     r3, r1, r2, lsl r3
    str     r3, [ip, #MX6X_MSI_IMR]

0:
	mov     r0, #0
    mov     pc, lr

CALLOUT_END(interrupt_mask_mx6x_msi)

/*
 * -----------------------------------------------------------------------
 * Unmask specified MSI interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_unmask_mx6x_msi, 0, interrupt_patch_msi)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov     ip,     #0x000000ff
	orr     ip, ip, #0x0000ff00
	orr     ip, ip, #0x00ff0000
	orr     ip, ip, #0xff000000

	/* only need to do something if an MSI (0 - 255) */
	cmp     r1, #256
	bge     0f

	/* point to the correct register set */
	mov     r2, r1, asr #5
	mov     r3, #12
	smlabb  ip, r3, r2, ip

	/* get the bit to unmask */
	mov     r3, r2, asl #5
	sub     r3, r1, r3

    ldr     r1, [ip, #MX6X_MSI_IMR]
	mov     r2, #1
	bic     r3, r1, r2, lsl r3
    str     r3, [ip, #MX6X_MSI_IMR]

0:
	mov     r0, #0
    mov     pc, lr

CALLOUT_END(interrupt_unmask_mx6x_msi)



#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/imx6x/callout_interrupt_mx6x_msi.S $ $Rev: 781278 $";
.previous
#endif
#endif 

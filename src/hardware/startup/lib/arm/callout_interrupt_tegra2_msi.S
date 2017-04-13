#
# Copyright 2010, QNX Software Systems. 
# 
# Licensed under the Apache License, Version 2.0 (the "License"). You 
# may not reproduce, modify or distribute this software except in 
# compliance with the License. You may obtain a copy of the License 
# at: http://www.apache.org/licenses/LICENSE-2.0 
# 
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" basis, 
# WITHOUT WARRANTIES OF ANY KIND, either express or implied.
#
# This file may contain contributions from others, either as 
# contributors under the License or as licensors under other terms.  
# Please review this entire file for other proprietary rights or license 
# notices, as well as the QNX Development Suite License Guide at 
# http://licensing.qnx.com/license-guide/ for other information.
#




#include "callout.ah"

#define TEGRA2_MSI_STATUS0		0x6C
#define TEGRA2_MSI_STATUS1		0x70
#define TEGRA2_MSI_STATUS2		0x74
#define TEGRA2_MSI_STATUS3		0x78
#define TEGRA2_MSI_STATUS4		0x7C
#define TEGRA2_MSI_STATUS5		0x80
#define TEGRA2_MSI_STATUS6		0x84
#define TEGRA2_MSI_STATUS7		0x88

#define TEGRA2_MSI_ENABLE0		0x8C
#define TEGRA2_MSI_ENABLE1		0x90
#define TEGRA2_MSI_ENABLE2		0x94
#define TEGRA2_MSI_ENABLE3		0x98
#define TEGRA2_MSI_ENABLE4		0x9C
#define TEGRA2_MSI_ENABLE5		0xA0
#define TEGRA2_MSI_ENABLE6		0xA4
#define TEGRA2_MSI_ENABLE7		0xA8


/*
 * -----------------------------------------------------------------------
 * Routine to patch callout code
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 *  [sp] - patch data specified by startup_intrinfo struct
 * -----------------------------------------------------------------------
 */

patch_intr:
	stmdb	sp!,{r4,lr}
	add		r4, r0, r2					

	ldr		r0, Lvaddr
	cmp		r0, #-1
	bne		1f

	/*
	 * Tegra 2 AFI Base address
	 */
	mov		r0, #0x800            /* Size of the AFI */
	ldr		r1, [sp, #8]	
	bl		callout_io_map_indirect
	str		r0, Lvaddr

1:
	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH	r4, r0, r1, r2, ip

	ldmia	sp!,{r4,pc}
    
Lvaddr: .word   -1
    
/*
 * -----------------------------------------------------------------------
 * Identify interrupt source.
 * FIXME!!!
 * For now, we only care about vector 0-31
 * Returns interrupt number in r4
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_id_tegra2_msi, 0, patch_intr)
	/*
	 * Get the AFI base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	mov		r4, #-1

    /*
     * Read status/enable registers
     */
	mov		r3, #0
0:
	ldr		r0, [ip, #TEGRA2_MSI_STATUS0]
	ldr		r1, [ip, #TEGRA2_MSI_ENABLE0]
	ands	r2, r0, r1
	bne		1f
	add		r3, r3, #4
	cmp		r3, #(TEGRA2_MSI_ENABLE7 - TEGRA2_MSI_ENABLE0)
	bgt		2f
	add		ip, ip, #4
	b		0b

1:
	clz		r4, r2
	rsbs	r4, r4, #31

	mov		r2, #1
	mov		r2, r2, lsl r4

	/*
	 * Mask the interrupt
	 */
	bic		r1, r1, r2
	str		r1, [ip, #TEGRA2_MSI_ENABLE0]

	/*
	 * Clear status
	 */
	str		r2, [ip, #TEGRA2_MSI_STATUS0]

	add		r4, r4, r3, lsl #3

2:  
CALLOUT_END(interrupt_id_tegra2_msi)


/*
 * -----------------------------------------------------------------------
 * Acknowledge specified interrupt
 *
 * On entry:
 *	r4 contains the interrupt number
 *	r7 contains the interrupt mask count
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_eoi_tegra2_msi, 0, patch_intr)
	/*
	 * Get the AFI base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	   Only unmask interrupt if mask count is zero
	 */
	teq		r7, #0
	bne		0f

	/*
	 * Unmask the interrupt
	 */
	and		r2, r4, #0x1F
	lsr     r0, r4, #5
	add		ip, ip, r0, lsl #2

	mov		r1, #1
	ldr		r0, [ip, #TEGRA2_MSI_ENABLE0]
	orr		r0, r0, r1, lsl r2
	str		r0, [ip, #TEGRA2_MSI_ENABLE0]
0:
CALLOUT_END(interrupt_eoi_tegra2_msi)


/*
 * -----------------------------------------------------------------------
 * Mask specified interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_mask_tegra2_msi, 0, patch_intr)
	/*
	 * Get the AFI base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000
 
	/*
	 * Mask the interrupt
	 */
	and		r2, r1, #0x1F
	lsr     r0, r1, #5
	add		ip, ip, r0, lsl #2

	mov		r1, #1
	ldr		r0, [ip, #TEGRA2_MSI_ENABLE0]
	bic		r0, r0, r1, lsl r2
	str		r0, [ip, #TEGRA2_MSI_ENABLE0]

	mov		r0, #0
	mov		pc, lr
CALLOUT_END(interrupt_mask_tegra2_msi)



/*
 * -----------------------------------------------------------------------
 * Unmask specified interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_unmask_tegra2_msi, 0, patch_intr)
	/*
	 * Get the AFI base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

	/*
	 * Unmask the interrupt
	 */
	and		r2, r1, #0x1F
	lsr     r0, r1, #5
	add		ip, ip, r0, lsl #2

	mov		r1, #1
	ldr		r0, [ip, #TEGRA2_MSI_ENABLE0]
	orr		r0, r0, r1, lsl r2
	str		r0, [ip, #TEGRA2_MSI_ENABLE0]
 
	mov		r0, #0
	mov		pc, lr
CALLOUT_END(interrupt_unmask_tegra2_msi)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/callout_interrupt_tegra2_msi.S $ $Rev: 781278 $";
.previous
#endif
#endif 

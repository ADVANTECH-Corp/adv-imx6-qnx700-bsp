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
 * Default MMU configuration parameters:
 *
 * sctlr_clr: specifies which bits must be cleared before enabling the MMU
 * sctlr_set: specifies which bits must be set to enable the MMU
 *
 * v7_prrr: default PRRR value that specifies the memory attribute types
 * v7_nmrr: default NMRR value that specifies normal memory attributes
 *
 * lpae_mair0: memory attributes for LPAE attribute index 0-3
 * lpae_mair1: memory attributes for LPAE attribute index 4-7
 */

/*
 * Bits that must be cleared in sctlr before applying sctlr_set.
 * This includes all bits that architecturally RAZ/SBZP.
 * It excludes bits that are specified in sctlr_set.
 */
unsigned	sctlr_clr =	ARM_SCTLR_A
					  | ARM_SCTLR_CP15BEN
					  | ARM_SCTLR_SBZ_7
					  | ARM_SCTLR_SBZ_8
					  | ARM_SCTLR_SBZ_9
					  | ARM_SCTLR_SW
					  | ARM_SCTLR_RR
					  | ARM_SCTLR_SBZ_15
					  | ARM_SCTLR_HA
					  | ARM_SCTLR_SBO_18
					  | ARM_SCTLR_WXN
					  | ARM_SCTLR_UWXN
					  | ARM_SCTLR_FI
					  | ARM_SCTLR_VE
					  | ARM_SCTLR_EE
					  | ARM_SCTLR_SBZ_26
					  | ARM_SCTLR_NMFI
					  | ARM_SCTLR_AFE
					  | ARM_SCTLR_TE
					  | ARM_SCTLR_SBZ_31;

/*
 * Bits that must be set in sctlr to enable the MMU.
 *
 * The following are for binary compatibility with old code:
 * CP15BEN - for code using deprecated CP15 barrier instructions
 * SW - for code using deprecated swp/swpb instructions
 */
unsigned	sctlr_set =	ARM_SCTLR_M
					  |	ARM_SCTLR_C
					  |	ARM_SCTLR_SBO_3
					  |	ARM_SCTLR_SBO_4
					  |	ARM_SCTLR_CP15BEN
					  |	ARM_SCTLR_SBO_6
					  |	ARM_SCTLR_SW
					  |	ARM_SCTLR_Z
					  |	ARM_SCTLR_I
					  |	ARM_SCTLR_V
					  |	ARM_SCTLR_SBO_16
					  |	ARM_SCTLR_SBO_18
					  |	ARM_SCTLR_SBO_22
					  |	ARM_SCTLR_SBO_23
#ifdef VARIANT_be
					  | ARM_SCTLR_EE;
#endif
					  |	ARM_SCTLR_TRE;

/*
 * Default PRRR configuration to replicate the non-TRE TEX[0]/C/B encodings.
 * 
 * PRRR[15:0] define the memory types for each attribute index:
 * 0 = (00) Strongly Ordered
 * 1 = (01) Device
 * 2 = (10) Normal WTnWA
 * 3 = (10) Normal WBnWA
 * 4 = (10) Normal NC
 * 5 = (00) reserved
 * 6 = (00) implementation defined
 * 7 = (10) Normal WBWA
 *
 * PRRR[17:16] define device memory shareability to match page table S bit.
 * PRRR[19:18] define normal memory shareability to match page table S bit.
 * PRRR[31:24] define not-outer shareable for each attribute index
 */
unsigned	v7_prrr = 0xff0a82a4;

/*
 * Default NMRR configuration to replicate the non-TRE TEX[0]/C/B encodings.
 *
 * NMRR[15:0] define the inner cacheability for each attribute index:
 * 0 = (00) non-cacheable
 * 1 = (00) non-cacheable
 * 2 = (10) WTnWA
 * 3 = (11) WBnWA
 * 4 = (00) non-cacheable
 * 5 = (00) reserved
 * 6 = (00) implementation defined
 * 7 = (01) WBWA
 *
 * NMRR[31:16] define the outer cacheability for each attribute index:
 * 0 = (00) non-cacheable
 * 1 = (00) non-cacheable
 * 2 = (10) WTnWA
 * 3 = (11) WBnWA
 * 4 = (00) non-cacheable
 * 5 = (00) reserved
 * 6 = (00) implementation defined
 * 7 = (01) WBWA
 */
unsigned	v7_nmrr = 0x40e040e0;

/*
 * Default MAIR0/1 values.
 * Encodings make AttrIndex[2:0] identical to ARMv7 TEX/CB=00x/xx:
 *
 * 0 = (00) Strongly Ordered
 * 1 = (04) Device
 * 2 = (aa) Normal WTnWA
 * 3 = (ee) Normal WBnWA
 * 4 = (44) Normal NC
 * 5 = (00) reserved
 * 6 = (00) implementation defined
 * 7 = (ff) Normal WBWA
 */
unsigned	lpae_mair0 = 0xeeaa0400;
unsigned	lpae_mair1 = 0xff000044;

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/mmu_config.c $ $Rev: 781531 $")
#endif

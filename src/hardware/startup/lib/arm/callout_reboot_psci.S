.ifdef VARIANT_v7

#
# Copyright 2015, QNX Software Systems. 
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
/*
 * requests PSCI:SYSTEM_RESET (0x84000009) to reset the system
 */

#include "callout.ah"

CALLOUT_START(reboot_psci_hvc, 0, 0)
	movw r0, 0x9
	movt r0, 0x8400
	.arch_extension virt
	hvc	#0
CALLOUT_END(reboot_psci_hvc)

CALLOUT_START(reboot_psci_smc, 0, 0)
	movw r0, 0x9
	movt r0, 0x8400
	.arch_extension sec
	smc	#0
CALLOUT_END(reboot_psci_smc)

#ifdef __QNXNTO__
#ifdef __USESRCVERSION
.section .ident, "SM",%progbits,1;
.asciz "$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/callout_reboot_psci.S $ $Rev: 781585 $";
.previous
#endif
#endif 
.endif

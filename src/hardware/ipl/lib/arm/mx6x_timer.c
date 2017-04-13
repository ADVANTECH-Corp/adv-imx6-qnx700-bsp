/*
 * $QNXLicenseC:
 * Copyright 2016, QNX Software Systems.
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


#include <arm/mx6x.h>
#include <hw/inout.h>
#include "private/mx6x_timer.h"

#define IPG_CLKS_IN_ONE_US          66

/*
 * Enable the EPIT timer if it is not already enabled. This also
 * requires that the parent clock gating register is enabled as well
 * i.e. CCM_CCGR1[15:14] or CCM_CCGR1[13:12] = 0x3.
 * Note that the timer code does not handle rollovers as of now.
 */
void mx6x_timer_init(mx6x_timer_t *timer) {
    out32(timer->base, 0x1000001);
}

void mx6x_usleep(mx6x_timer_t *timer, uint32_t sleep_duration)
{
    uint32_t start, dur;

    start = in32(timer->base + MX6X_EPIT_CNR);
    dur = (sleep_duration * IPG_CLKS_IN_ONE_US);

    while (start-in32(timer->base + MX6X_EPIT_CNR) <= dur) {
        __asm__ __volatile__("nop");
    }
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/arm/mx6x_timer.c $ $Rev: 799761 $")
#endif

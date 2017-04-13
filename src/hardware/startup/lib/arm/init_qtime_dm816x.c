/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems.
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
 * TI DM816X specific timer support.
 * TIMER1 is used.
 */

#include "startup.h"
#include <arm/dm816x.h>

static uintptr_t    timer_base;

static const struct callout_slot    timer_callouts[] = {
    { CALLOUT_SLOT(timer_load, _dm816x) },
    { CALLOUT_SLOT(timer_value, _dm816x) },
    { CALLOUT_SLOT(timer_reload, _dm816x) },
};

static unsigned
timer_start_dm816x()
{
    out32(timer_base + DM816X_TIMER_TCLR, 
        in32(timer_base + DM816X_TIMER_TCLR) | DM816X_TIMER_TCLR_ST);
    return in32(timer_base + DM816X_TIMER_TCRR);
}

static unsigned
timer_diff_dm816x(unsigned start)
{
    unsigned now = in32(timer_base + DM816X_TIMER_TCRR);
    return (now - start);
}

void
init_qtime_dm816x(unsigned timer_freq)
{
    struct qtime_entry  *qtime = alloc_qtime();

    /*
     * Map timer registers, Uboot only enabled timer1, so use timer1 for now
     */
    timer_base = startup_io_map(DM816X_TIMER_SIZE, DM816X_TIMER1_BASE);

    /* Clear timer count and reload count */
    out32(timer_base + DM816X_TIMER_TLDR, 0);
    out32(timer_base + DM816X_TIMER_TCRR, 0);

    /*
     * Setup Timer0
     * Auto-reload enable
     * Prescaler disable
     * Stop timer, timer_load will enable it
     */
    out32(timer_base + DM816X_TIMER_TCLR, (DM816X_TIMER_TCLR_PRE_DISABLE | DM816X_TIMER_TCLR_AR));

    timer_start = timer_start_dm816x;
    timer_diff  = timer_diff_dm816x;

    qtime->intr = 67;   /* Timer1 irq */

	qtime->cycles_per_sec = (uint64_t)timer_freq;
	invert_timer_freq(qtime, timer_freq);

    add_callout_array(timer_callouts, sizeof(timer_callouts));
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/init_qtime_dm816x.c $ $Rev: 782002 $")
#endif

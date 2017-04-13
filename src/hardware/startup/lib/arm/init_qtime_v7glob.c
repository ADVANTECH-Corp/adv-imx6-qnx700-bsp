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

/*
 * ARMv7 Cortex-A9MP timer support.
 * Here we use the per-cpu Global Timer on CPU0.
 */

#include "startup.h"
#include <arm/mpcore.h>

extern void arm_add_clock_cycles(struct callout_rtn *callout, int incr_bit);

/*
 * TIMER_IRQ is the IRQ number for the Global Timer Peripheral Interrupt.
 * It is defined  by the ARMv7 Cortex-A9 architecture extensions.
 */
#define TIMER_IRQ	27 //GLOBAL_TIMER_IRQ

extern struct callout_rtn timer_load_v7glob;
extern struct callout_rtn timer_value_v7glob;
extern struct callout_rtn timer_reload_v7glob;
extern struct callout_rtn clock_cycles_v7glob;

static const struct callout_slot timer_callouts[] = {
	{ CALLOUT_SLOT(timer_load, _v7glob) },
	{ CALLOUT_SLOT(timer_value, _v7glob) },
	{ CALLOUT_SLOT(timer_reload, _v7glob) },
};

static unsigned
timer_start_v7glob()
{
	return in32(mpcore_scu_base + GTIM_COUNTERL);
}

static unsigned
timer_diff_v7glob(unsigned start)
{
	return in32(mpcore_scu_base + GTIM_COUNTERL) - start;
}

static void
timer_enable_v7glob(void)
{
	out32(mpcore_scu_base + GTIM_CONTROL, in32(mpcore_scu_base + GTIM_CONTROL) | 1<<0);
}

void
init_qtime_v7glob()
{
	struct qtime_entry  *qtime = alloc_qtime();
	unsigned cntfrq;

	timer_start = timer_start_v7glob;
	timer_diff = timer_diff_v7glob;

	/*
	 * Enables the counter timer
	 */
	timer_enable_v7glob();

	/*
	 * Disable timer comparator matching
	 */
	out32(mpcore_scu_base + GTIM_CONTROL, in32(mpcore_scu_base + GTIM_CONTROL) & ~(1<<1));

	cntfrq = 50000000;
	qtime->intr = TIMER_IRQ;
	qtime->cycles_per_sec = (uint64_t)cntfrq;
	invert_timer_freq(qtime, cntfrq);

	/*
	 * Global timer registers are banked per-cpu so ensure that the
	 * system clock is only operated on via cpu0
	 */
	qtime->flags |= QTIME_FLAG_TIMER_ON_CPU0;

	add_callout_array(timer_callouts, sizeof(timer_callouts));

	/*
	 * Add clock_cycles callout to directly access 64-bit counter
	 */
	arm_add_clock_cycles(&clock_cycles_v7glob, 0);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/init_qtime_v7glob.c $ $Rev: 811885 $")
#endif

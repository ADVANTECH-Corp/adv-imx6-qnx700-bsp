/*
 * $QNXLicenseC:
 * Copyright 2014, QNX Software Systems.
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

static const struct callout_slot	timer_callouts[] = {
	{ CALLOUT_SLOT(timer_load,   _a9gt) },
	{ CALLOUT_SLOT(timer_value,  _a9gt) },
	{ CALLOUT_SLOT(timer_reload, _a9gt) },
};

extern struct callout_rtn	clock_cycles_arm_global;

static unsigned
timer_start_a9gt()
{
	unsigned	tmp;

	tmp = in32(mpcore_scu_base + 0x608);		// control register
	out32(mpcore_scu_base + 0x608, tmp & ~0xf);	// disable counter
	out32(mpcore_scu_base + 0x600, ~0);			// set maximum count
	out32(mpcore_scu_base + 0x608, tmp | 1);	// enable counter

	return in32(mpcore_scu_base + 0x604);	// counter
}

static unsigned
timer_diff_a9gt(unsigned start)
{
	unsigned now = in32(mpcore_scu_base + 0x604);	// counter

	return start - now;	// count down timer
}

void
init_qtime_a9gt(unsigned timer_freq)
{
	struct qtime_entry	*qtime = alloc_qtime();

	timer_start = timer_start_a9gt;
	timer_diff  = timer_diff_a9gt;

	/*
	 * The global time has enable/comparator bits banked for each cpu
	 * Set TIMER_ON_CPU0 so we ensure interrupts go only to CPU0
	 */
	qtime->flags |= QTIME_FLAG_TIMER_ON_CPU0;
	qtime->intr = 27;
	invert_timer_freq(qtime, timer_freq);
	qtime->cycles_per_sec = (uint64_t)timer_freq;
	add_callout_array(timer_callouts, sizeof(timer_callouts));

	/*
	 * Add clock_cycles callout to directly get 64-bit counter value
	 */
	arm_add_clock_cycles(&clock_cycles_a9gt, 0);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/init_qtime_a9gt.c $ $Rev: 781278 $")
#endif

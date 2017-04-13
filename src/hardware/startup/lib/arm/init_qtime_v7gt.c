/*
 * $QNXLicenseC:
 * Copyright 2014, QNX Software Systems. All Rights Reserved.
 *
 * This source code may contain confidential information of QNX Software
 * Systems (QSS) and its licensors.  Any use, reproduction, modification,
 * disclosure, distribution or transfer of this software, or any software
 * that includes or is based upon any of this code, is prohibited unless
 * expressly authorized by QSS by written agreement.  For more information
 * (including whether this source code file has been published) please
 * email licensing@qnx.com. $
*/


#include "startup.h"

static const struct callout_slot	timer_callouts[] = {
	{ CALLOUT_SLOT(timer_load,		_v7gt) },
	{ CALLOUT_SLOT(timer_value,		_v7gt) },
	{ CALLOUT_SLOT(timer_reload,	_v7gt) },
};

static unsigned
timer_start_v7gt()
{
	unsigned	lo, hi;

	__asm__ __volatile__("mrrc	p15, 1, %0, %1, c14" : "=r"(lo), "=r"(hi));
	return lo;
}

static unsigned
timer_diff_v7gt(unsigned start)
{
	unsigned	lo, hi;

	__asm__ __volatile__("mrrc	p15, 1, %0, %1, c14" : "=r"(lo), "=r"(hi));

	return lo - start;
}

void
init_qtime_v7gt(unsigned timer_freq, unsigned intr)
{
	struct qtime_entry	*qtime = alloc_qtime();

	timer_start = timer_start_v7gt;
	timer_diff  = timer_diff_v7gt;

	/*
	 * Disable timer event matching
	 */
	__asm__ __volatile__("mcr	p15, 0, %0, c14, c3, 1" : : "r"(0));

	if (intr == 0) {
		/*
		 * Default to using Virtual Timer interrupt is PPI4 (irq #27)
		 */
		qtime->intr = 27;
	} else {
		qtime->intr = intr;
	}
	qtime->cycles_per_sec = (uint64_t)timer_freq;
	invert_timer_freq(qtime, timer_freq);

	/*
	 * Generic timer registers are banked per-cpu so ensure that the
	 * system clock is only operated on via cpu0
	 */
	qtime->flags |= QTIME_FLAG_TIMER_ON_CPU0;

	add_callout_array(timer_callouts, sizeof(timer_callouts));

	/*
	 * Add clock_cycles callout to directly access 64-bit counter
	 */
	arm_add_clock_cycles(&clock_cycles_v7gt, 0);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/init_qtime_v7gt.c $ $Rev: 781601 $")
#endif

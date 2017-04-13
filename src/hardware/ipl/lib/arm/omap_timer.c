/*
 * $QNXLicenseC:
 * Copyright 2013, QNX Software Systems.
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
#include "ipl.h"
#include <hw/inout.h>
#include <arm/dm816x.h>
#include <omap_timer.h>

/*
 * Timer utilities dedicated for benchmarking purpose
 */


/* Base address of the timer */
unsigned long timer_base;

/* timer clcok */
unsigned long clock;

#define PTV			0		/* Roughly 0.1 micro seconds per unit, will get rounded at 214 seconds */
#define PRE			0		/* No prescale */
#define CONVERT_MILLI_SEC	1000	/* input clock DEVOSC 20MHz */
#define CONVERT_MICRO_SEC	(1000 * 1000)

/* To simplify the APIs, only keep one free run timer */
unsigned start;

void omap_timer_enable(unsigned long base, unsigned long freq)
{
	timer_base = base;
	clock = freq;

	out32(timer_base + DM816X_TIMER_TCRR, 0);

	/* Timer re-load value */
	out32(timer_base + DM816X_TIMER_TLDR, 0);

	/*
	 * Start timer, autoload enabled
	 * Please note that when the timer gets overflow and reloaded,
	 * The omap_timer_curr() gets "restart",
	 * but the delay() functions remain accurate
	 */
	out32(timer_base + DM816X_TIMER_TCLR, (PRE ? DM816X_TIMER_TCLR_PRE : 0) | (PTV << 2) | DM816X_TIMER_TCLR_AR | DM816X_TIMER_TCLR_ST);

	start = 0;
}

/* It's good to call omap_timer_disable() at the end of startup to save power */
void omap_timer_disable(void)
{
	out32(timer_base + DM816X_TIMER_TCLR, 0);
}

static unsigned omap_timer_get(void)
{
	return in32(timer_base + DM816X_TIMER_TCRR);
}

void omap_timer_start(void)
{
	start = omap_timer_get();
}

static unsigned omap_timer_delta(unsigned current, unsigned start)
{
	/* We assume it can only overflow once*/
	if (start > current) {
		return (0 - start + current);
	}

	return (current - start);
}

void omap_timer_curr(const char * msg, int micro)
{
	unsigned current = omap_timer_get();

	/* Time unit in micro second */
	if (micro) {
		ser_putdec(omap_timer_delta(current, start) * (1 << (PTV + PRE)) / (clock / CONVERT_MICRO_SEC));
		ser_putstr(" us on ");
	} else {
		ser_putdec(omap_timer_delta(current, start) * (1 << (PTV + PRE)) / (clock / CONVERT_MILLI_SEC));
		ser_putstr(" ms on ");
	}

	ser_putstr(msg);
	ser_putstr("\n");
}

void omap_usec_delay(unsigned us)
{
	unsigned long long time_start;
	unsigned long long diff;
	unsigned long dly;

	time_start = omap_timer_get();

	/* How many clicks per milli second */
	dly = ((clock / CONVERT_MILLI_SEC) / (1 << (PTV + PRE)));

	/* How many clicks in the delay period */
	dly = us * dly / CONVERT_MILLI_SEC;

	dly = dly ? dly : 1;

	for (;;) {
		diff = omap_timer_delta(omap_timer_get(), time_start);
		if (diff >= dly)
			break;
	}
}

/*
 * If both PRE and PTV are 0, the unit is 50 nano seconds
 * 'ns' will be rounded to integer multiple of 50 nano seconds.
 */
void omap_nano_delay(unsigned ns)
{
	unsigned long long time_start;
	unsigned long long diff;
	unsigned long dly;

	time_start = omap_timer_get();

	/* How many clicks per milli second */
	dly = ((clock / CONVERT_MILLI_SEC) / (1 << (PTV + PRE)));

	/* How many clicks in the delay period */
	dly = ns * dly / CONVERT_MICRO_SEC;

	dly = dly ? dly : 1;

	for (;;) {
		diff = omap_timer_delta(omap_timer_get(), time_start);
		if (diff >= dly)
			break;
	}
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/arm/omap_timer.c $ $Rev: 798438 $")
#endif

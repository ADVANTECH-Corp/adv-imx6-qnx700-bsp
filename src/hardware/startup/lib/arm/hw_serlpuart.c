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

/*
 * Polled serial operations for Freescale LPUART
 * Note: The driver currently assumes that the LPUART is already initialized.
 */

#include "startup.h"
#include "hw/inout.h"

#define LPUART_STAT (0x4) /* LPUART Status Register */
#define LPUART_DATA (0xC) /* LPUART Data Register */

#define LPUART_STAT_TDRE (1<<23) /* Transmit Data Register Empty Flag */

static void
parse_line(unsigned channel, const char *line, unsigned *baud, unsigned *clk) {
	/*
	 * Get device base address and register stride
	 */
	if (*line != '.' && *line != '\0') {
		dbg_device[channel].base = strtoul(line, (char **)&line, 16);
		if (*line == '^')
			dbg_device[channel].shift = strtoul(line+1, (char **)&line, 0);
	}

	/*
	 * Get baud rate
	 */
	if (*line == '.')
		++line;
	if (*line != '.' && *line != '\0')
		*baud = strtoul(line, (char **)&line, 0);

	/*
	 * Get clock rate
	 */
	if (*line == '.')
		++line;
	if (*line != '.' && *line != '\0')
		*clk = strtoul(line, (char **)&line, 0);
}

/*
 * Initialize one of the serial ports
 */
static void
init_lpuart(unsigned channel, const char *init, const char *defaults) {
	unsigned baud, clk;

	parse_line(channel, defaults, &baud, &clk);
	parse_line(channel, init, &baud, &clk);
}

void
init_lpuart_le(unsigned channel, const char *init, const char *defaults) {
	init_lpuart(channel, init, defaults);
}

void
init_lpuart_be(unsigned channel, const char *init, const char *defaults) {
	init_lpuart(channel, init, defaults);
}

/*
 * Send a character
 */
void
put_lpuart_le(int data)
{
	unsigned base = dbg_device[0].base;

	while (!(inle32(base + LPUART_STAT) & LPUART_STAT_TDRE));

	outle32(base + LPUART_DATA, (unsigned)data);
}

void
put_lpuart_be(int data)
{
	unsigned base = dbg_device[0].base;

	while (!(inbe32(base + LPUART_STAT) & LPUART_STAT_TDRE));

	outbe32(base + LPUART_DATA, (unsigned)data);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/hw_serlpuart.c $ $Rev: 768373 $")
#endif

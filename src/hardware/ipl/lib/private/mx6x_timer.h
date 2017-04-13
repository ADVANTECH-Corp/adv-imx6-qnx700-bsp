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
#ifndef __MX6X_TIMER_H__
#define __MX6X_TIMER_H__

#include <stdint.h>

/* Either MX6X_EPIT1_BASE or MX6X_EPIT2_BASE can be used as the clock/timer source */
typedef struct {
    unsigned int base;
} mx6x_timer_t;

void mx6x_timer_init(mx6x_timer_t *timer);
void mx6x_usleep(mx6x_timer_t *timer, uint32_t sleep_duration);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/private/mx6x_timer.h $ $Rev: 799775 $")
#endif

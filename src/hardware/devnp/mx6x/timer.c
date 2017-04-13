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

#include "mx6q.h"
#include <netdrvr/avb.h>
#include <sys/syslog.h>

pthread_t		tid;
int			intid;
struct sigevent		event;

void mx6q_timer_init (mx6q_dev_t *mx6q)
{
    volatile uint32_t	*tbase = mx6q->treg;

    /* Remove from the old thread */
    if (intid) {
	InterruptDetach(intid);
    }

    /* Ensure timer is stopped */
    *(tbase + GPT_CR) = GPT_CR_SWR | GPT_CR_CLKSRC_PERI | GPT_CR_STOPEN |
      GPT_CR_DOZEEN | GPT_CR_WAITEN | GPT_CR_DBGEN | GPT_CR_ENMOD;

    /* Attach to this thread */
    event.sigev_notify = SIGEV_INTR;
    intid = InterruptAttachEvent(GPT_INT, &event, 0);
    tid = pthread_self();
}

int mx6q_timer_delay (mx6q_dev_t *mx6q, struct ifdrv *ifd)
{
    volatile uint32_t		*tbase = mx6q->treg;
    precise_timer_delay_t	td;
    uint32_t			val;

    if (ISSTACK) {
	log(LOG_ERR, "mx6x precise delay called from stack context");
	return EWOULDBLOCK;
    }

    memcpy(&td, (((uint8_t *)ifd) + sizeof(*ifd)), sizeof(td));

    val = (td.ns / TICK_TIME);
    if (val == 0) {
	return EOK;
    }

    /* Ensure this thread is the one attached */
    if (!pthread_equal(tid, pthread_self())) {
	mx6q_timer_init(mx6q);
    }

    /* Setup IRQ registers */
    *(tbase + GPT_IR) = GPT_IR_OF1IE;

    /* Setup timer value */
    *(tbase + GPT_CR) = GPT_CR_CLKSRC_PERI | GPT_CR_STOPEN |
      GPT_CR_DOZEEN | GPT_CR_WAITEN | GPT_CR_DBGEN | GPT_CR_ENMOD | GPT_CR_EN;
    *(tbase + GPT_OCR1) = val;

    /* Wait for the timer to fire */
    InterruptWait(0, NULL);

    /* Clear interrupt and shutdown timer */
    *(tbase + GPT_CR) = GPT_CR_SWR | GPT_CR_CLKSRC_PERI | GPT_CR_STOPEN |
      GPT_CR_DOZEEN | GPT_CR_WAITEN | GPT_CR_DBGEN | GPT_CR_ENMOD;
    InterruptUnmask(GPT_INT, intid);

    return EOK;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/mx6x/timer.c $ $Rev: 791878 $")
#endif

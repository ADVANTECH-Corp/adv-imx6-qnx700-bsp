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


#include "mx6q.h"

int
mx6q_process_queue(void *arg, struct nw_work_thread *wtp)
{
    mx6q_dev_t      *mx6q = arg;
    struct ifnet    *ifp = &mx6q->ecom.ec_if;
    struct mbuf     *m;

    while(1) {
    pthread_mutex_lock(&mx6q->rx_mutex);
    IF_DEQUEUE(&mx6q->rx_queue, m);
    if (m != NULL) {
        pthread_mutex_unlock(&mx6q->rx_mutex);
        (*ifp->if_input)(ifp, m);
    } else {
        /* Leave mutex locked to prevent any enqueues, unlock in enable */
        break;
    }
    }
    return 1;
}

int
mx6q_enable_queue(void *arg)
{
    mx6q_dev_t		*mx6q = arg;
    struct ifnet	*ifp = &mx6q->ecom.ec_if;
    volatile uint32_t	*base = mx6q->reg;

    InterruptLock(&mx6q->spinlock);
    if (ifp->if_flags & IFF_RUNNING) {
#ifdef MX6XSLX
	if (mx6q->rx_full & (1 << 1)) {
	    *(base + MX6Q_IMASK) |= IMASK_RXF1EN;
	    *(base + MX6Q_R_DES_ACTIVE1) = R_DES_ACTIVE;
	}
	if (mx6q->rx_full & (1 << 2)) {
	    *(base + MX6Q_IMASK) |= IMASK_RXF2EN;
	    *(base + MX6Q_R_DES_ACTIVE2) = R_DES_ACTIVE;
	}
#endif
	if (mx6q->rx_full & (1 << 0)) {
	    *(base + MX6Q_IMASK) |= IMASK_RFIEN;
	    *(base + MX6Q_R_DES_ACTIVE) = R_DES_ACTIVE;
	}
    }
    InterruptUnlock(&mx6q->spinlock);
    mx6q->rx_full = 0;
    mx6q->rx_running = 0;
    pthread_mutex_unlock(&mx6q->rx_mutex);

  return 1;
}

static inline int
mx6q_last_bd_active (mx6q_dev_t *mx6q, int queue)
{
    int			idx;
    volatile mpc_bd_t	*tx_bd;

    idx = mx6q->tx_pidx[queue];
    if (idx != 0) {
	idx--;
    } else {
	idx = mx6q->num_tx_descriptors - 1;
    }
    tx_bd = &mx6q->tx_bd[idx + (queue * mx6q->num_tx_descriptors)];
    if (tx_bd->status & TXBD_R) {
	return 1;
    }
    return 0;
}

//
// this is the interrupt handler which directly masks off
// the hardware interrupt at the nic.
//
const struct sigevent *
mx6q_isr(void *arg, int iid)
{
    mx6q_dev_t		*mx6q;
    struct _iopkt_inter	*ient;
    volatile uint32_t	*base;
    uint32_t		ievent, imask;

    mx6q = arg;
    ient = &mx6q->inter;
    base = mx6q->reg;

    /* Read interrupt cause bits */
    ievent = *(base + MX6Q_IEVENT);

    /* Read current mask */
    InterruptLock(&mx6q->spinlock);
    imask = *(base + MX6Q_IMASK);

    /* Rx interrupts get masked out and a pulse to the Rx thread */
#ifdef MX6XSLX
    if (((imask & IMASK_RXF1EN) != 0) &&
        ((ievent & IEVENT_RXF1) != 0)) {
        *(base + MX6Q_IMASK) = imask & (~IMASK_RXF1EN);
        InterruptUnlock(&mx6q->spinlock);
        return &mx6q->isr_event[1];
    }
    if (((imask & IMASK_RXF2EN) != 0) &&
        ((ievent & IEVENT_RXF2) != 0)) {
        *(base + MX6Q_IMASK) = imask & (~IMASK_RXF2EN);
        InterruptUnlock(&mx6q->spinlock);
        return &mx6q->isr_event[2];
    }
#endif
    if (((imask & IMASK_RFIEN) != 0) &&
        ((ievent & IEVENT_RFINT) != 0)) {
        *(base + MX6Q_IMASK) = imask & (~IMASK_RFIEN);
        InterruptUnlock(&mx6q->spinlock);
        return &mx6q->isr_event[0];
    }

    /* Handle TS_TIMER */
    InterruptLock(&mx6q->rtc_spinner);
    if ((ievent & IEVENT_TS_TIMER) != 0) {
	*(base + MX6Q_IEVENT) = IEVENT_TS_TIMER;
	mx6q->rtc_half = 0;
	mx6q->rtc++;
    }
    InterruptUnlock(&mx6q->rtc_spinner);

    /*
     * Tx interrupts need to deal with ERR006358.
     * Clear the interrupt, if the X_DES is clear and the last BD is active
     * then re-enable X_DES.
     * Do it here in the ISR to avoid the overhead of mx6q_process_interrupt()
     * and mx6q_enable_interrupt(). This also permits all of them to be
     * processed in a single ISR call if they are all set.
     */
#ifdef MX6XSLX
    if ((ievent & IEVENT_TXF1) != 0) {
        *(base + MX6Q_IEVENT) = IEVENT_TXF1;
	if ((*(base + MX6Q_X_DES_ACTIVE1) == 0) &&
	    mx6q_last_bd_active(mx6q, 1)) {
	    *(base + MX6Q_X_DES_ACTIVE1) = X_DES_ACTIVE;
	}
    }

    if ((ievent & IEVENT_TXF2) != 0) {
        *(base + MX6Q_IEVENT) = IEVENT_TXF2;
	if ((*(base + MX6Q_X_DES_ACTIVE2) == 0) &&
	    mx6q_last_bd_active(mx6q, 2)) {
	    *(base + MX6Q_X_DES_ACTIVE2) = X_DES_ACTIVE;
	}
    }
#endif

    if ((ievent & IEVENT_TFINT) != 0) {
	*(base + MX6Q_IEVENT) = IEVENT_TFINT;
	if ((*(base + MX6Q_X_DES_ACTIVE) == 0) &&
	    mx6q_last_bd_active(mx6q, 0)) {
	    *(base + MX6Q_X_DES_ACTIVE) = X_DES_ACTIVE;
	}
	/*
	 * If it was out of descriptors then send it the normal path to
	 * call start to reap and Tx more.
	 */
	if (mx6q->tx_full) {
	    InterruptUnlock(&mx6q->spinlock);
	    return interrupt_queue(mx6q->iopkt, ient);
	}
    }

    InterruptUnlock(&mx6q->spinlock);
    return NULL;
}

const struct sigevent *
mx6q_1588_isr(void *arg, int iid)
{
    mx6q_dev_t		*mx6q;
    volatile uint32_t	*base;

    mx6q = arg;
    base = mx6q->reg;

    InterruptLock(&mx6q->rtc_spinner);
    mx6q->rtc_half = 1;
    InterruptUnlock(&mx6q->rtc_spinner);

    /* Clear the Timer flag and setup the next load of the compare value */
    *(base + MX6Q_TCSR0) = (MX6Q_TCSR_MODE_OUT_SW << MX6Q_TCSR_MODE_SHIFT) |
	MX6Q_TCSR_TIE | MX6Q_TCSR_TF;
    *(base + MX6Q_TCCR0) = MX6Q_TIMER_PER05SEC;

    return NULL;
}

/*
 * The usage of TS_AVAIL and the TXF interrupts is complicated.
 * TS_AVAIL is set on the frame transmission, TXF is set on descriptor update.
 * Without the 802.1Qav credit based shapers enabled then these are the same
 * time. With the shapers enabled, the flow for a single queue is:
 *
 * BD1 Fetch | Data1 Fetch | Tx1 | shaper wait | BD1 Update | BD2 Fetch |
 * Data 2 Fetch | Tx 2 | shaper wait | BD2 Update | BD3 Fetch...
 *
 * Freescale did it this way to improve bus performance. The problem is that
 * it separates out the TS_AVAIL interrupt from the actual value being
 * available in the buffer descriptor. The value is available in ENET_ATSTMP
 * but it would be complex to correlate that with the appropriate PTP frame,
 * especially if PTP was being transmitted on multiple queues. This means that
 * we would need to use the TXF interrupt to pick up the timestamp value
 * from the updated buffer descriptor. Instead we just reap the descriptors
 * when fetching a Tx timestamp.
 */

int
mx6q_enable_interrupt(void *arg)
{
    return 0;
}

int
mx6q_process_interrupt(void *arg, struct nw_work_thread *wtp)
{
    mx6q_dev_t		*mx6q = arg;

    for (;;) {

        if (!(mx6q->tx_full)) {
            break;
        }

        if (mx6q->cfg.verbose > 6) {
            log(LOG_ERR, "%s(): tx_full %d",
                __FUNCTION__, mx6q->tx_full);
        }

        if (mx6q->tx_full) {
            NW_SIGLOCK_P(&mx6q->ecom.ec_if.if_snd_ex,
                 mx6q->iopkt, WTP);
	    mx6q->tx_full = FALSE;
	    __sync_synchronize();
	    /* Call start to reap and Tx more. */
	    mx6q_start(&mx6q->ecom.ec_if);
        }
    }

    return 1;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/mx6x/event.c $ $Rev: 807530 $")
#endif

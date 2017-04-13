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
#include "proto.h"
#include <pthread.h>
#include <sys/neutrino.h>

i2c_status_t imx_i2c_recv(void *hdl, void *buf, unsigned int len, unsigned int stop)
{
    imx_dev_t      *dev = hdl;
    uint64_t        timeout;
    struct _pulse    pulse;

   /* Nothing to do - exit */
    if (len == 0) {
        return I2C_STATUS_DONE;
    }

    /*
     * Start transaction - mutex will block call if another transaction is
     * in progress
     */
    pthread_mutex_lock(&dev->mutex);
    I2C_DEBUG_SLOG( "Receive transaction started, addr fmt %d, addr 0x%x, dat0 0x%2X, dat1 0x%2X, length %d, stop flag %d, Interrupt count %d",
                    dev->slave_addr_fmt,
                    dev->slave_addr,
                    ((char *)buf)[0],
                    ((char *)buf)[1],
                    len,
                    stop,
                    dev->intr_count );
    if (dev->bus_state == STOPPED && IMX_I2C_BUS_BUSY(dev)) {

        /*
         * Bus is busy, possibly because of a multimaster bus or the last access was illegal.
         * Wait for bus idle, reset or timeout.
         */
        if (imx_wait_bus_idle(dev) == -1) {
            I2C_DEBUG_SLOG( "I2C bus busy timeout pre-recv: reset and retry"
                            "(CTRL=0x%x STS=0x%x)",
                            in16(dev->regbase + IMX_I2C_CTRREG_OFF),
                            in16(dev->regbase + IMX_I2C_STSREG_OFF) );
            imx_i2c_reset(dev);
            if (imx_wait_bus_idle(dev) == -1) {
                I2C_ERROR_SLOG( "I2C timeout waiting to start a send transaction, "
                                "(CTRL=0x%x STS=0x%x)",
                                in16(dev->regbase + IMX_I2C_CTRREG_OFF),
                                in16(dev->regbase + IMX_I2C_STSREG_OFF) );
                pthread_mutex_unlock(&dev->mutex);
                return I2C_STATUS_BUSY;
            }
        }
    }

    /*
     * Fill in all pertinent transaction info in the I2C structure, then
     * leave the interrupt to take care of the I2C bus phases ... wake up
     * on completion or failure.
     */
    dev->transaction = MASTER_RECEIVE;
    dev->transaction_status = I2C_TRANSACTION_IN_PROGRESS;
    dev->transaction_error = 0;
    dev->rx_buf = buf;
    dev->rx_buf_idx = 0;
    dev->rx_buf_len = len;
    dev->stop_req = stop;

    /* Make sure interrupts are on */
    out16(dev->regbase + IMX_I2C_CTRREG_OFF, in16(dev->regbase + IMX_I2C_CTRREG_OFF) | CTRREG_IIEN);

    /*
     * Initiate the transaction by sending the address - the handler
     * will take care of the rest.
     */
    if (dev->slave_addr_fmt == I2C_ADDRFMT_7BIT) {
        imx_sendaddr7(dev);
    } else { /* 10-bit format */
        imx_sendaddr10(dev);
    }

    /* Block the caller until the transaction completes or a timeout occurs. */
    timeout = dev->slave_timeout_multiplier * (len+1);
    TimerTimeout(CLOCK_MONOTONIC, _NTO_TIMEOUT_RECEIVE, NULL, &timeout, NULL);
    MsgReceivePulse(dev->chid, &pulse, sizeof(pulse), NULL);
    I2C_DEBUG_SLOG("Receive return prio is %d, pulse code was %d, intr count is %d",
                   getprio(0),
                   pulse.code,
                   dev->intr_count);

    if (dev->transaction_status == I2C_TRANSACTION_IN_PROGRESS) {

        /* Flag timeout condition */
        I2C_ERROR_SLOG("Recv transaction completion timeout (CTRL=0x%x STS=0x%x)",
                       in16(dev->regbase + IMX_I2C_CTRREG_OFF),
                       in16(dev->regbase + IMX_I2C_STSREG_OFF) );
        I2C_ERROR_SLOG("I2C state: bus_state=%d, transac_status=%d, transac_error=%d, rxidx=%d, rxlen=%d",
                       dev->bus_state,
                       dev->transaction_status,
                       dev->transaction_error,
                       dev->rx_buf_idx,
                       dev->rx_buf_len );

        out16(dev->regbase + IMX_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN);
        dev->bus_state = STOPPED;
        pthread_mutex_unlock(&dev->mutex);
        return I2C_STATUS_ERROR;
    } else if ( dev->transaction_status == I2C_TRANSACTION_FAILED ) {

        /* Flag arbitration lost, address NACK, etc (handler sets error) */
        I2C_ERROR_SLOG("Recv transaction error 0x%x (CTRL=0x%x STS=0x%x)",
                       dev->transaction_error,
                       in16(dev->regbase + IMX_I2C_CTRREG_OFF),
                       in16(dev->regbase + IMX_I2C_STSREG_OFF) );

        /* Reset I2C engine */
        imx_i2c_reset(dev);
        pthread_mutex_unlock(&dev->mutex);
        return dev->transaction_error;
    }

    /* Success: return */
    pthread_mutex_unlock(&dev->mutex);
    return I2C_STATUS_DONE;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/imx/recv.c $ $Rev: 810496 $")
#endif

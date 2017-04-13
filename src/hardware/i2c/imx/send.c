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

i2c_status_t
imx_i2c_send(void *hdl, void *buf, unsigned int len, unsigned int stop)
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
    I2C_DEBUG_SLOG( "Transmit transaction started, addr fmt %d, addr 0x%X, dat0 0x%2X, dat1 0x%2X, length %d, stop flag %d, Interrupt count %d",
                    dev->slave_addr_fmt,
                    dev->slave_addr,
                    ((char *)buf)[0],
                    ((char *)buf)[1],
                    len,
                    stop,
                    dev->intr_count );

    if (dev->bus_state == STOPPED && IMX_I2C_BUS_BUSY(dev)) {

        /*
         * Bus is busy, possibly because of a multimaster bus.
         * Wait for bus idle, reset or timeout.
         */
        if (imx_wait_bus_idle(dev) == -1) {
            I2C_DEBUG_SLOG( "I2C bus busy timeout pre-snd: reset and retry"
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
    dev->transaction = MASTER_SEND;
    dev->transaction_status = I2C_TRANSACTION_IN_PROGRESS;
    dev->transaction_error = 0;
    dev->tx_buf = buf;
    dev->tx_buf_len = len;
    dev->tx_buf_idx = 0;
    dev->stop_req = stop;

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
    I2C_DEBUG_SLOG("Transmit return prio is %d, pulse code was %d", getprio(0), pulse.code);

   if (dev->transaction_status == I2C_TRANSACTION_IN_PROGRESS) {

        /* Flag timeout condition */
        I2C_ERROR_SLOG("Transmit transaction completion timeout (CTRL=0x%x STS=0x%x)",
                       in16(dev->regbase + IMX_I2C_CTRREG_OFF),
                       in16(dev->regbase + IMX_I2C_STSREG_OFF) );
        pthread_mutex_unlock(&dev->mutex);
        return I2C_STATUS_BUSY;
    } else if ( dev->transaction_status == I2C_TRANSACTION_FAILED ) {

        /* Flag arbitration lost,etc (handler sets error) */
        I2C_ERROR_SLOG("Transmit transaction error 0x%x (CTRL=0x%x STS=0x%x)",
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

void imx_sendaddr7(imx_dev_t *dev) {

    unsigned char address_byte;

    /*
     * Trigger the I2C start or a restart if applicable.
     */
    out16(dev->regbase + IMX_I2C_CTRREG_OFF, in16(dev->regbase + IMX_I2C_CTRREG_OFF) | CTRREG_MTX);
    if (dev->bus_state == STOPPED) {
        out16(dev->regbase + IMX_I2C_CTRREG_OFF, in16(dev->regbase + IMX_I2C_CTRREG_OFF) | CTRREG_MSTA);
    } else if (dev->bus_state == WAIT_RESTART) {
        out16(dev->regbase + IMX_I2C_CTRREG_OFF, in16(dev->regbase + IMX_I2C_CTRREG_OFF) | CTRREG_RSTA);
    }

    /* Set bit 0 of the address is 0 for a send transaction, 1 for a receive. */
    address_byte = (dev->slave_addr) << 1;
    if (dev->transaction == MASTER_RECEIVE) {
        address_byte |= IMX_I2C_ADDR_RD;
    }

    /* Store the address in the data reg */
    out16(dev->regbase + IMX_I2C_DATREG_OFF, address_byte);

    /* Update bus state for ISR to keep track of what to do next */
    dev->bus_state = TX_ADDR_7BIT;
}

void imx_sendaddr10(imx_dev_t *dev) {

    unsigned char address_byte;

    /*
     * Trigger either an I2C start or a restart.
     */
    out16(dev->regbase + IMX_I2C_CTRREG_OFF, in16(dev->regbase + IMX_I2C_CTRREG_OFF) | CTRREG_MTX);
    if (dev->bus_state == STOPPED) {
        out16(dev->regbase + IMX_I2C_CTRREG_OFF, in16(dev->regbase + IMX_I2C_CTRREG_OFF) | CTRREG_MSTA);
    } else if (dev->bus_state == WAIT_RESTART) {
        out16(dev->regbase + IMX_I2C_CTRREG_OFF, in16(dev->regbase + IMX_I2C_CTRREG_OFF) | CTRREG_RSTA);
    }

    /*
     * Form the first address byte with the 2 msbs of the 10-bit address
     * Bit 0 of the address is 0 for send transaction, 1 for receive.
     */
    address_byte = IMX_I2C_XADDR1(dev->slave_addr);
    if (dev->transaction == MASTER_RECEIVE) {
        address_byte |= IMX_I2C_ADDR_RD;
    }

    /* Send the first address byte - the ISR will send the second byte. */
    out16(dev->regbase + IMX_I2C_DATREG_OFF, address_byte);

    /* Update bus state for ISR to keep track of what to do next */
    dev->bus_state = TX_ADDR_10BIT_MSB;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/imx/send.c $ $Rev: 810496 $")
#endif

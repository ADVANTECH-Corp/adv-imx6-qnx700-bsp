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
#include <unistd.h>

const struct sigevent *imx_i2c_intr(void *context, int id)
{
    imx_dev_t *dev;
    uint16_t   i2sr;

    dev = (imx_dev_t *)context;
    dev->intr_count++;

    /* Get the interrupt status and clear the interrupts */
    i2sr = in16(dev->regbase + IMX_I2C_STSREG_OFF);
    out16(dev->regbase + IMX_I2C_STSREG_OFF, 0);

    /* Check for lost arbitration - valid in multi-master mode */
    if (i2sr & STSREG_IAL) {
        /* Arbitration lost - unblock send() or receive() to fail the transaction */
        dev->transaction_status = I2C_TRANSACTION_FAILED;
        dev->transaction_error = I2C_STATUS_ARBL;
        out16( dev->regbase + IMX_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN);
        return &dev->intrevent;
    }

    /* Check for NACKs after sending address or data byte(s) */
    if ( (dev->bus_state == TX_ADDR_7BIT || dev->bus_state == TX_ADDR_10BIT_MSB ||
          dev->bus_state == TX_ADDR_10BIT_MSB || dev->bus_state == TX_DATA ) &&
         (i2sr & STSREG_RXAK) )
    {
            /* Acknowledge failed - fail transaction, send stop */
            dev->transaction_status = I2C_TRANSACTION_FAILED;
            dev->transaction_error = I2C_STATUS_NACK;
            out16( dev->regbase + IMX_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN);
            return &dev->intrevent;
    }

    /* Error checking done */

    /* Address phase: second part of extended 10-bit address */
    if (dev->bus_state == TX_ADDR_10BIT_MSB) {
        /* Transmit lsb of extended address - don't unblock tx/rx function */
        out16(dev->regbase + IMX_I2C_DATREG_OFF, IMX_I2C_XADDR2(dev->slave_addr));
        dev->bus_state = TX_ADDR_10BIT_LSB;
        return NULL;
    }

    /* Address phase complete - transition to transmit/receive data state */
    if (dev->bus_state == TX_ADDR_10BIT_LSB || dev->bus_state == TX_ADDR_7BIT) {
        if (dev->transaction == MASTER_SEND) {
            dev->bus_state = TX_DATA;
        } else if (dev->transaction == MASTER_RECEIVE) {
            dev->bus_state = RX_DATA;

            /*
             * To initiate the receive phase, do a dummy read of the data register
             * and don't send an ack if there's only one byte to be received.
             */
            out16( dev->regbase + IMX_I2C_CTRREG_OFF,
                    CTRREG_IEN | CTRREG_IIEN | CTRREG_MSTA |
                        (dev->rx_buf_len==1 ? CTRREG_TXAK : 0) );
            in16(dev->regbase + IMX_I2C_DATREG_OFF);

            /* Wait for the next interrupt - first received data byte */
            return NULL;
        }
    }

    /* Transmit data phase */
    if (dev->bus_state == TX_DATA) {
        if (dev->tx_buf_idx < dev->tx_buf_len) {
            /*Not done - send next byte */
            out16(dev->regbase + IMX_I2C_DATREG_OFF, dev->tx_buf[dev->tx_buf_idx]);
            dev->tx_buf_idx++;
            return NULL;  /* not done - don't unblock tx/rx function until complete */
        } else {
            /* Done - initiate a STOP to release bus if requested */
            if (dev->stop_req) {
                out16(dev->regbase + IMX_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN);
                dev->bus_state = STOPPED;
                dev->transaction_status = I2C_TRANSACTION_COMPLETE;
            } else {
                /* No STOP request:  next transaction will generate a restart */
                dev->bus_state = WAIT_RESTART;
                dev->transaction_status = I2C_TRANSACTION_COMPLETE;
            }
            return &dev->intrevent;
        }
    }

    /* Receive data phase */
    if (dev->bus_state == RX_DATA) {
        if (dev->rx_buf_idx == dev->rx_buf_len-1) {

            /* Last byte received - initiate a STOP to release bus if requested */
            if (dev->stop_req) {
                out16(dev->regbase + IMX_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MTX | CTRREG_TXAK);
                dev->bus_state = STOPPED;
                dev->transaction_status = I2C_TRANSACTION_COMPLETE;
            } else {
                /* No STOP request:  next transaction must generate a restart */
                out16(dev->regbase + IMX_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MTX | CTRREG_TXAK | CTRREG_MSTA);
                dev->bus_state = WAIT_RESTART;
                dev->transaction_status = I2C_TRANSACTION_COMPLETE;
            }
        } else if (dev->rx_buf_idx == dev->rx_buf_len-2) {

            /* Second last byte received - prepare to NACK last byte */
            out16(dev->regbase + IMX_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN | CTRREG_MSTA | CTRREG_TXAK);
        }

        /* Retrieve the received byte */
        dev->rx_buf[dev->rx_buf_idx] = in16(dev->regbase + IMX_I2C_DATREG_OFF) & 0xFF;
        dev->rx_buf_idx++;

        if (dev->rx_buf_idx != dev->rx_buf_len) {
            return NULL;  /* not done - don't unblock tx/rx function until complete */
        } else {
            return &dev->intrevent; /* Done - unblock tx/rx/function */
        }
    }

    /* Shouldn't get here - capture unhandled event */
    dev->transaction_error = 0xDEAD;

    return &dev->intrevent;
}

int imx_wait_bus_idle(imx_dev_t *dev)
{
    uint64_t timeout;

    /* Loop in 100us increments to test for an idle bus */
    timeout = dev->bus_busy_timeout*100;

    while (timeout-- >= 100) {
        if (!IMX_I2C_BUS_BUSY(dev)) {
            I2C_DEBUG_SLOG("Wait for bus not busy done, spin left %lld (CTRL=0x%x STS=0x%x)",
                            timeout,
                            in16(dev->regbase + IMX_I2C_CTRREG_OFF),
                            in16(dev->regbase + IMX_I2C_STSREG_OFF) );
            return 0;
        }
        timeout--;
    }

    /* Timeout - leave */
    I2C_ERROR_SLOG("Wait for bus not busy failed (CTRL=0x%x STS=0x%x)",
                    in16(dev->regbase + IMX_I2C_CTRREG_OFF),
                    in16(dev->regbase + IMX_I2C_STSREG_OFF) );
    return -1;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/imx/intr.c $ $Rev: 810496 $")
#endif

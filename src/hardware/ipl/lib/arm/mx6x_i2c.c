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


#include "hw/inout.h"
#include "private/mx6x_i2c.h"
#include "ipl.h"

void init_i2c_bus(mx6x_i2c_dev_t *dev)
{
    // init I2C clock rate
    out16(dev->base + I2C_IFDR, dev->div);
}

static int wait_op_done(unsigned int base, int is_tx)
{
    volatile unsigned short v;
    int i = WAIT_RXAK_LOOPS;

    while ((((v = in16(base + I2C_I2SR)) & I2C_I2SR_IIF) == 0 ||
           (v & I2C_I2SR_ICF) == 0) && --i > 0) {

        if (v & I2C_I2SR_IAL) {
            ser_putstr("[ERROR] Arbitration lost, line "); ser_putdec(__LINE__); ser_putstr("\n");
            return -1;
        }
    }

    if (i <= 0) {
        ser_putstr("[ERROR] Unexpected timeout, line "); ser_putdec(__LINE__); ser_putstr("\n");
        return -1;
    }
    if (is_tx) {
        if (v & I2C_I2SR_IAL) {
            ser_putstr("[ERROR] Arbitration lost, line "); ser_putdec(__LINE__); ser_putstr("\n");
            return -1;
        }
        if (v & I2C_I2SR_RXAK) {
            ser_putstr("[ERROR] No ACK received, line "); ser_putdec(__LINE__); ser_putstr("\n");
            return -1;
        }
    }
    return 0;
}

// For master TX, always expect a RXAK signal to be set!
static int tx_byte(unsigned char *data, unsigned int base)
{
    // clear both IAL and IIF bits
    out16(base + I2C_I2SR, 0);

    // transmit the data
    out16(base + I2C_I2DR, *data);

    if (wait_op_done(base, 1) != 0)
        return -1;

    return 0;
}

// For master RX
static int rx_byte(unsigned char *data, unsigned int base)
{
    unsigned short i2cr;

    if (wait_op_done(base, 0) != 0)
        return -1;

    // clear both IAL and IIF bits
    out16(base + I2C_I2SR, 0);

    // last byte --> generate STOP
    i2cr = in16(base + I2C_I2CR);
    out16(base + I2C_I2CR, (i2cr & ~(I2C_I2CR_MSTA | I2C_I2CR_MTX)));

    *data = in16(base + I2C_I2DR);

    return 0;
}

static inline int is_bus_free(unsigned int base)
{
    return ((in16(base + I2C_I2SR) & I2C_I2SR_IBB) == 0);
}

static inline int wait_till_busy(unsigned int base)
{
    int i = I2C_WAIT_CNT;

    while (((in16(base + I2C_I2SR) & I2C_I2SR_IBB) == 0) && (--i > 0)) {
        if (in16(base + I2C_I2SR) & I2C_I2SR_IAL) {
            ser_putstr("[ERROR] Arbitration lost, line "); ser_putdec(__LINE__); ser_putstr("\n");
           return -1;
        }
    }

    if (i <= 0) {
        ser_putstr("[ERROR] unexpected timeout line "); ser_putdec(__LINE__); ser_putstr("\n");
        return -1;
    }

    return 0;
}

static int i2c_xfer(mx6x_i2c_dev_t *dev, uint16_t reg, i2c_reg_size_t reg_addr_size, unsigned char* val, int dir)
{
    volatile int i;
    unsigned char data;
    unsigned short i2cr;
    int ret = 0;

    // reset and enable I2C1
    out16(dev->base + I2C_I2CR, 0);
    out16(dev->base + I2C_I2CR, I2C_I2CR_IEN);

    // wait at least 2 cycles of per_clk, declare i as volatile to avoid optimization 
    for (i = 0; i < 100; i++) ;

    // Step 1: generate START signal
    // 1.1 make sure bus is free
    if (!is_bus_free(dev->base)) {
        ser_putstr("[ERROR] Bus is not free, line "); ser_putdec(__LINE__); ser_putstr("\n");
        return -1;
    }

    // 1.2 clear both IAL and IIF bits
    out16(dev->base + I2C_I2SR, 0);

    // 1.3 assert START signal and also indicate TX mode
    i2cr = I2C_I2CR_IEN | I2C_I2CR_MSTA | I2C_I2CR_MTX;
    out16(dev->base + I2C_I2CR, i2cr);

    // 1.4 make sure bus is busy after the START signal
    if (wait_till_busy(dev->base) != 0) {
        ser_putstr("[ERROR] Bus is not busy, line "); ser_putdec(__LINE__); ser_putstr("\n");
        return -1;
    }

    // Step 2: send slave address + read/write at the LSB
    data = ((dev->slave << 1) | I2C_WRITE) & 0xFF;
    if (tx_byte(&data, dev->base) != 0) {
        ser_putstr("[ERROR] Failed to transmit slave address, line "); ser_putdec(__LINE__); ser_putstr("\n");
        return -1;
    }

    // Step 3: send I2C device register address
    if (reg_addr_size == I2C_REG_ADDR_16_BIT) {
        // Send MSB of register address
        data = (unsigned char)((reg & 0xFF00) >> 8);
        if (tx_byte(&data, dev->base) != 0) {
            ser_putstr("[ERROR] Failed to transmit device register address, line "); ser_putdec(__LINE__); ser_putstr("\n");
            return -1;
        }
    }

    // Send LSB of register address
    data = (unsigned char)(reg & 0x00FF);
    if (tx_byte(&data, dev->base) != 0) {
        ser_putstr("[ERROR] Failed to transmit device register address, line "); ser_putdec(__LINE__); ser_putstr("\n");
        return -1;
    }

    // Step 4: read/write data
    if (dir == I2C_READ) {
        // do repeat-start
        i2cr = in16(dev->base + I2C_I2CR);
        out16(dev->base + I2C_I2CR, (i2cr | I2C_I2CR_RSTA));

        // send slave address again, but indicate read operation
        data = (dev->slave << 1) | I2C_READ;
        if (tx_byte(&data, dev->base) != 0) {
            ser_putstr("[ERROR] Failed to transmit slave address, line "); ser_putdec(__LINE__); ser_putstr("\n");
            return -1;
        }

        // change to receive mode
        i2cr = in16(dev->base + I2C_I2CR);

        // read only one byte, make sure don't send ack
        i2cr |= I2C_I2CR_TXAK;
        i2cr &= ~I2C_I2CR_MTX;
        out16(dev->base + I2C_I2CR, i2cr);

        // dummy read
        in16(dev->base + I2C_I2DR);

        // now reading ...
        if (rx_byte(&data, dev->base) != 0) {
            ser_putstr("[ERROR] Failed to receive data, line "); ser_putdec(__LINE__); ser_putstr("\n");
            return -1;
        }
        *val = data;

    } else {    // I2C_WRITE
        data = *val & 0xFF;
        if((ret = tx_byte(&data, dev->base)) != 0) {
            ser_putstr("[ERROR] Failed to transmit data, line "); ser_putdec(__LINE__); ser_putstr("\n");
        }

        // generate STOP by clearing MSTA bit
        out16(dev->base + I2C_I2CR, (I2C_I2CR_IEN | I2C_I2CR_MTX));
    }

    return ret;
}

int i2c_write(mx6x_i2c_dev_t *dev, unsigned char reg, unsigned char* val)
{
	return i2c_xfer(dev, reg, I2C_REG_ADDR_8_BIT, val, I2C_WRITE);
}

int i2c_read(mx6x_i2c_dev_t *dev, unsigned char reg, unsigned char* val)
{
	return i2c_xfer(dev, reg, I2C_REG_ADDR_8_BIT, val, I2C_READ);
}

int i2c_write2(mx6x_i2c_dev_t *dev, uint16_t reg, i2c_reg_size_t reg_addr_size, unsigned char* val)
{
    return i2c_xfer(dev, reg, reg_addr_size, val, I2C_WRITE);
}

int i2c_read2(mx6x_i2c_dev_t *dev, uint16_t reg, i2c_reg_size_t reg_addr_size, unsigned char* val)
{
    return i2c_xfer(dev, reg, reg_addr_size, val, I2C_READ);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/arm/mx6x_i2c.c $ $Rev: 799736 $")
#endif

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

void imx_i2c_reset(imx_dev_t *dev)
{

    /* Reset I2C engine - clear any interrupts */
    out16(dev->regbase + IMX_I2C_CTRREG_OFF, 0);
    out16(dev->regbase + IMX_I2C_STSREG_OFF, 0);

    /* Re-enable I2C engine - IEN and IIEN must be on separate write accesses */
    out16(dev->regbase + IMX_I2C_CTRREG_OFF, CTRREG_IEN);
    out16(dev->regbase + IMX_I2C_STSREG_OFF, 0);
    out16(dev->regbase + IMX_I2C_CTRREG_OFF, CTRREG_IEN | CTRREG_IIEN);
    out16(dev->regbase + IMX_I2C_FRQREG_OFF, dev->i2c_freq_val);

    /* Bus is now ready */
    dev->bus_state = STOPPED;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/imx/reset.c $ $Rev: 810496 $")
#endif

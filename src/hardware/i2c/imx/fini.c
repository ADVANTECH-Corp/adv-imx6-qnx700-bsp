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

void
imx_i2c_fini(void *hdl)
{
    imx_dev_t  *dev = hdl;

    /* Stop I2C engine */
    out16(dev->regbase + IMX_I2C_CTRREG_OFF, 0);
    out16(dev->regbase + IMX_I2C_STSREG_OFF, 0);

    /* Free mutex and unmap device memory */
    pthread_mutex_destroy(&dev->mutex);
    munmap_device_io(dev->physbase, dev->reglen);

    /* Free the malloc! */
    free(dev);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/imx/fini.c $ $Rev: 810496 $")
#endif

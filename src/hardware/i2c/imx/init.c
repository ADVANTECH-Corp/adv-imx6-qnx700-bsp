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

void *imx_i2c_init(int argc, char *argv[])
{
    imx_dev_t      *dev;

    slogf( _SLOG_SETCODE(_SLOGC_CHAR,0), _SLOG_ERROR,"Starting i.MX family I2C driver (build date %s %s)", __DATE__, __TIME__);

    /* Set up privity */
    if (-1 == ThreadCtl(PRIVITY_FLAGS, 0)) {
        perror("ThreadCtl");
        return NULL;
    }

    /* Allocate driver struct and clear the space */
    dev = malloc(sizeof(imx_dev_t));
    if (!dev) {
        return NULL;
    }
    memset(dev, 0, sizeof(imx_dev_t));

    /*
     * Create a mutex to prevent multiple simultaneous I2C accesses
     * on the same bus
     */
    pthread_mutex_init(&dev->mutex, NULL);

    /* Process command line options */
    if(strstr(argv[argc-1], "controller") !=NULL) {
        sscanf(argv[argc-1],"controller=%u",&dev->unit);
        argc--;
    } else {
        dev->unit= -1;
    }

    if (-1 == imx_i2c_options(dev, argc, argv)) {
        goto fail;
    }

    if(dev->physbase == 0 || dev->intr == 0) {
        I2C_ERROR_SLOG("Invalid I2C controller physics based address or IRQ value.");
        I2C_ERROR_SLOG("Please check the command line or hwinfo default settings.");
        goto fail;
     }

    /* Map I2C register space */
    dev->regbase = mmap_device_io(dev->reglen, dev->physbase);
    if (dev->regbase == (uintptr_t)MAP_FAILED) {
        perror("mmap_device_io");
        goto fail;
    }


    /* Initialize interrupt handler event */
     if ((dev->chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1) {
         perror("ChannelCreate");
         goto fail;
     }

     if ((dev->coid = ConnectAttach(0, 0, dev->chid, _NTO_SIDE_CHANNEL, 0)) == -1) {
         perror("ConnectAttach");
         goto fail_chnl_dstr;
     }

     dev->intrevent.sigev_notify   = SIGEV_PULSE;
     dev->intrevent.sigev_coid     = dev->coid;
     dev->intrevent.sigev_code     = IMX_I2C_EVENT;
     dev->intrevent.sigev_priority = dev->intr_priority;

     /*
      * Attach interrupt handler
      */
     dev->iid = InterruptAttach(dev->intr, imx_i2c_intr, dev, 0, _NTO_INTR_FLAGS_TRK_MSK);
     if (dev->iid == -1) {
         perror("InterruptAttachEvent");
         goto fail_con_dtch;
     }

    /* Set clock prescaler using default baud */
    imx_i2c_set_bus_speed(dev, 100000, NULL);

    /* Set the address to which the I2C responds when addressed as a slave.
     * NOTE: This is not the address sent on the bus during the address transfer.
     * Presumably, this should only be required when running the controller in
     * slave mode. The data sheet initialization sequence shows it being set
     * regardless of whether the I2C block is configured as a master or slave.
     */
    out16(dev->regbase + IMX_I2C_ADRREG_OFF, dev->own_addr<<1);

    /* Start I2C module */
    imx_i2c_reset(dev);

    I2C_DEBUG_SLOG("Post reset: (I2CR=0x%x I2SR=0x%x)",
              in16(dev->regbase + IMX_I2C_CTRREG_OFF),
              in16(dev->regbase + IMX_I2C_STSREG_OFF) );

    return dev;

fail_con_dtch:
    ConnectDetach(dev->coid);

fail_chnl_dstr:
       ChannelDestroy(dev->chid);

fail:
    pthread_mutex_destroy(&dev->mutex);
    free(dev);
    return NULL;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/imx/init.c $ $Rev: 810496 $")
#endif

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

static const unsigned int I2C_Divs[64] = {
                      30,   32,   36,   42,   48,   52,   60,   72,
                      80,   88,  104,  128,  144,  160,  192,  240,
                     288,  320,  384,  480,  576,  640,  768,  960,
                    1152, 1280, 1536, 1920, 2304, 2560, 3072, 3840,
                      22,   24,   26,   28,   32,   36,   40,   44,
                      48,   56,   64,   72,   80,   96,  112,  128,
                     160,  192,  224,  256,  320,  384,  448,  512,
                     640,  768,  896, 1024, 1280, 1536, 1792, 2048 };

/* This index corresponds to a divider of 768 */
#define IMX_DEFAULT_IC  22

unsigned int
find_best_ic( unsigned int i2c_div ) {

    unsigned int best_ic;
    int     ic;

    /*
     * Find index to closest divider match that is greater than or equal
     * to divider and return that index.
     */
    best_ic = ~0;
    for ( ic = 0; ic < 64; ic++ ) {
        if (I2C_Divs[ic] >= i2c_div) {
            if ( best_ic == ~0 || I2C_Divs[ic] < I2C_Divs[best_ic] ) {
                    best_ic = ic;
            }
        }
    }

    if ( best_ic == ~0 ) {
        best_ic = IMX_DEFAULT_IC;
    }
    return (best_ic);
}

int
imx_i2c_set_bus_speed(void *hdl, unsigned int speed, unsigned int *ospeed)
{
    imx_dev_t      *dev = hdl;
    unsigned int    i2c_div, i2c_freq_val;

    if (speed > 400000) {
        errno = EINVAL;
        return -1;
    }

    if (speed != dev->speed) {
        i2c_div = dev->input_clk / speed;
        i2c_freq_val = find_best_ic( i2c_div );
        out16( dev->regbase + IMX_I2C_FRQREG_OFF, i2c_freq_val );
        dev->speed = speed; //save the speed, next time we don't have to recalculate
        dev->i2c_freq_val = i2c_freq_val;

        /*
         * Calculate the corresponding timeout value per byte of data sent.
         * The overall timeout value is expressed here - the units are below
         * the equation:
         *
         * timeout = 1,000,000,000/Freq x 10(1 + num_bytes_payload) x slack
         *   ns    =       ns/bit       x          bits             x no units
         *
         * A slack of 1 is the minimum time the transaction could possibly take.
         * The user can change the preset slack using slave_resp_timeout_factor option.
         */
        dev->slave_timeout_multiplier = 1000000000 / dev->speed;
        dev->slave_timeout_multiplier *= 10 * dev->slave_resp_timeout_slack;
        I2C_DEBUG_SLOG("I2C bus speed: %d, timeout slack: %d, timeout per byte: %llu ns",
                        dev->speed,
                        dev->slave_resp_timeout_slack,
                        dev->slave_timeout_multiplier);
    }

    if (ospeed)
        *ospeed = dev->input_clk / I2C_Divs[ dev->i2c_freq_val ];

    return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/imx/bus_speed.c $ $Rev: 810496 $")
#endif

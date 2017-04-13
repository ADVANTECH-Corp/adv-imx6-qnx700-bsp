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

#include <inttypes.h>
#include <sys/types.h>

void *memset(void *s, int c, size_t n) {
    volatile unsigned char      *p = s;

    /* Stuff unaligned addresses first */
    while(((uintptr_t)p & (sizeof(unsigned) - 1)) && n) {
        *p++ = c;
        n--;
    }

    /* Now stuff in native int size chunks if we can */
    if(n >= sizeof(unsigned)) {
#if __INT_BITS__ == 32
        unsigned        cc = 0x01010101 * (unsigned char)c;
#elif __INT_BITS__ == 64
        unsigned        cc = 0x0101010101010101 * (unsigned char)c;
#else
#error Unknown __INT_BITS__ size
#endif
        unsigned        *pp = (unsigned *)p - 1;

        while(n >= sizeof(unsigned)) {
            n -= sizeof(unsigned);
            *++pp = cc;
        }
        if(n) {
            p = (unsigned char *)(pp + 1);
        }
    }

    /* Get the remaining bytes */
    if(n) {
        p--;
        while(n) {
            n--;
            *++p = c;
        }
    }

    return s;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/memset.c $ $Rev: 811485 $")
#endif

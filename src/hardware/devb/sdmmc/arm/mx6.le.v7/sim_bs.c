/*
 * $QNXLicenseC:
 * Copyright 2015, QNX Software Systems.
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

// Module Description:  board specific interface

#include <sim_sdmmc.h>

int sim_bs_args( SIM_HBA *hba, char *options )
{
    SIM_SDMMC_EXT   *ext;
    char            *value;
    int              opt;
    static char     *opts[] = {
                        "dname",
                        NULL
                    };

    ext = (SIM_SDMMC_EXT *)hba->ext;

    while( options && *options != '\0' ) {
        if( ( opt = getsubopt( &options, opts, &value ) ) == -1 ) {
        }

        switch( opt ) {
            case 0:
                ext->eflags |= SDMMC_EFLAG_DEVNAME;
                if( value && !strncmp( value, "compat", strlen( "compat" ) ) ) {
                    ext->eflags |= SDMMC_EFLAG_BS;
                }
                break;

            default:
                break;
        }
    }

    return( EOK );
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/arm/mx6.le.v7/sim_bs.c $ $Rev: 819326 $")
#endif

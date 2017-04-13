
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

#include <inttypes.h>

#ifndef I2C_RESMGR_CLIENT_H_
#define I2C_RESMGR_CLIENT_H_

void* i2c_resmgr_client_init(unsigned physbase, unsigned intr, unsigned slave, unsigned xferbuf_len, void (*report_problem)(int severity, int reason, const char *msg));
void* i2c_resmgr_client_init_2(unsigned physbase, unsigned intr, unsigned slave, unsigned xferbuf_len, void (*report_problem)(int severity, int reason, const char *msg), const char* i2c_devname);
void* i2c_resmgr_client_init_3(unsigned physbase, unsigned intr, unsigned slave, unsigned xferbuf_len, void (*report_problem)(int severity, int reason, const char *msg), const char* i2c_devname, unsigned i2c_speed);
void i2c_resmgr_client_fini(void* hdl);
int i2c_resmgr_client_reset(void* hdl);
void i2c_resmgr_client_set_slave(void* hdl, unsigned slave);
int i2c_resmgr_client_set_reg_addr_len(void* hdl, unsigned len);
int i2c_resmgr_client_tweak_bus_speed_ns(void* hdl, int high_tweak, int low_tweak);

/* Read functions return raw data. Endianness considerations are the
 * responsibility of the client.
 */
_Uint8t* i2c_resmgr_client_read(void* dev, unsigned len);
_Uint8t* i2c_resmgr_client_read_reg(void* dev, _Uint32t reg, unsigned len);
_Uint8t* i2c_resmgr_client_read8(void* dev, _Uint32t reg);
_Uint16t* i2c_resmgr_client_read16(void* dev, _Uint32t reg);
_Uint32t* i2c_resmgr_client_read32(void* dev, _Uint32t reg);

/* Write functions; data is written as is. Endianness and register length
 * considerations are the responsibility of the client.
 */
int i2c_resmgr_client_write(void* dev, unsigned len, _Uint8t* buf);
int i2c_resmgr_client_write_reg(void* dev, _Uint32t reg, unsigned len, _Uint8t* buf);
int i2c_resmgr_client_write8(void* dev, _Uint32t reg, _Uint8t data);
int i2c_resmgr_client_write16(void* dev, _Uint32t reg, _Uint16t data);
int i2c_resmgr_client_write32(void* dev, _Uint32t reg, _Uint32t data);

#endif /* i2c_resmgr_client_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/lib/i2c-resmgr-client/public/hw/i2c_resmgr_client.h $ $Rev: 772209 $")
#endif

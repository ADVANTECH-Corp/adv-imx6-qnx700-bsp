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
#ifndef __PROTO_H_INCLUDED
#define __PROTO_H_INCLUDED

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <hw/i2c.h>
#include <sys/hwinfo.h>
#include <drvr/hwinfo.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>

/* Slog macros for I2C debug/error purposes - long, short or none */
//#define I2C_ERROR_SLOG(msg, ...) slogf( _SLOG_SETCODE(_SLOGC_CHAR,0), _SLOG_ERROR, "I2C file %s, line %d: " msg, __FILE__, __LINE__, ##__VA_ARGS__)
#define I2C_ERROR_SLOG(msg, ...) slogf( _SLOG_SETCODE(_SLOGC_CHAR,0), _SLOG_ERROR, msg, ##__VA_ARGS__)
//#define I2C_ERROR_SLOG(msg, ...)

//#define I2C_DEBUG_SLOG(msg, ...) slogf( _SLOG_SETCODE(_SLOGC_CHAR,0), _SLOG_INFO, "I2C file %s, line %d: " msg, __FILE__, __LINE__, ##__VA_ARGS__)
//#define I2C_DEBUG_SLOG(msg, ...) slogf( _SLOG_SETCODE(_SLOGC_CHAR,0), _SLOG_INFO, msg, ##__VA_ARGS__)
#define I2C_DEBUG_SLOG(msg, ...)

#if defined(__ARM__) && defined(_NTO_TCTL_IO_PRIV)
#define PRIVITY_FLAGS    _NTO_TCTL_IO_PRIV
#else
#define PRIVITY_FLAGS    _NTO_TCTL_IO
#endif

/* Type of I2C transaction requested */
typedef enum {
    MASTER_SEND,
    MASTER_RECEIVE,
    SLAVE_SEND,
    SLAVE_RECEIVE
} i2c_transaction_e;

/* Current state of the I2C bus */
typedef enum {
    STOPPED,
    WAIT_RESTART,
    TX_ADDR_7BIT,
    TX_ADDR_10BIT_MSB,
    TX_ADDR_10BIT_LSB,
    TX_DATA,
    RX_DATA
} i2c_state_e;

typedef enum {
    I2C_TRANSACTION_IN_PROGRESS,
    I2C_TRANSACTION_COMPLETE,
    I2C_TRANSACTION_FAILED
} i2c_trans_status_e;

typedef struct _imx_dev {

    /* Startup parameters */
    int                 unit;
    unsigned            reglen;
    uintptr_t           regbase;
    unsigned            physbase;
    unsigned            own_addr;
    unsigned            options;
    unsigned            input_clk;
    unsigned            speed;
    unsigned            slave_resp_timeout_slack;
    uint64_t            slave_timeout_multiplier;
    unsigned            i2c_freq_val;
    int                 intr;
    int                 intr_priority;
    int                 intr_count;
    int                 iid;
    int                 chid;
    int                 coid;
    struct sigevent     intrevent;
    uint64_t            bus_busy_timeout;

    /* Transaction State Parameters */
    pthread_mutex_t     mutex;
    unsigned            slave_addr;
    i2c_addrfmt_t       slave_addr_fmt;
    unsigned            stop_req;
    unsigned char      *tx_buf;
    unsigned            tx_buf_len;
    unsigned            tx_buf_idx;
    unsigned char      *rx_buf;
    unsigned            rx_buf_len;
    unsigned            rx_buf_idx;
    i2c_transaction_e   transaction;
    i2c_trans_status_e  transaction_status;
    unsigned            transaction_error;
    i2c_state_e         bus_state;
} imx_dev_t;

#define IMX_I2C_BUS_BUSY(dev)  (in16(dev->regbase + IMX_I2C_STSREG_OFF) & STSREG_IBB)
#define IMX_I2C_XADDR1(addr)   (0xf0 | (((addr) >> 7) & 0x6))
#define IMX_I2C_XADDR2(addr)  ((addr) & 0xff)

#define IMX_I2C_ADDR_RD         1
#define IMX_I2C_ADDR_WR         0

#define IMX_OPT_VERBOSE         0x00000002

/* I2C bus busy and slave response default timeouts (ns) */
#define IMX_I2C_RESP_TIMEOUT_SLACK  20
#define IMX_I2C_BUSY_TIMEOUT        500000

#define IMX_I2C_INPUT_CLOCK         60000000

#define IMX_I2C1_BASE_ADDR          0x43F80000
#define IMX_I2C2_BASE_ADDR          0x43F98000
#define IMX_I2C3_BASE_ADDR          0x43F84000

#define IMX_I2C1_IRQ                10
#define IMX_I2C2_IRQ                4
#define IMX_I2C3_IRQ                3
#define IMX_I2C_DEF_INTR_PRIORITY   21
#define IMX_I2C_EVENT               1

#define IMX_I2C_ADRREG_OFF      0x00
#define IMX_I2C_FRQREG_OFF      0x04
#define IMX_I2C_CTRREG_OFF      0x08
#define CTRREG_IEN              (1 << 7)
#define CTRREG_IIEN             (1 << 6)
#define CTRREG_MSTA             (1 << 5)
#define CTRREG_MTX              (1 << 4)
#define CTRREG_TXAK             (1 << 3)
#define CTRREG_RSTA             (1 << 2)
#define IMX_I2C_STSREG_OFF      0x0C
#define STSREG_ICF              (1 << 7)
#define STSREG_IAAS             (1 << 6)
#define STSREG_IBB              (1 << 5)
#define STSREG_IAL              (1 << 4)
#define STSREG_SRW              (1 << 2)
#define STSREG_IIF              (1 << 1)
#define STSREG_RXAK             (1 << 0)
#define IMX_I2C_DATREG_OFF      0x10
#define IMX_I2C_REGLEN          0x18
#define IMX_I2C_OWN_ADDR        0x00

#define IMX_I2C_REVMAJOR(rev)   (((rev) >> 8) & 0xff)
#define IMX_I2C_REVMINOR(rev)   ((rev) & 0xff)

/* Callback functions */
void *       imx_i2c_init(int argc, char *argv[]);
void         imx_i2c_fini(void *hdl);
i2c_status_t imx_i2c_recv(void *hdl, void *buf, unsigned int len, unsigned int stop);
i2c_status_t imx_i2c_send(void *hdl, void *buf, unsigned int len, unsigned int stop);
int          imx_i2c_set_slave_addr(void *hdl, unsigned int addr, i2c_addrfmt_t fmt);
int          imx_i2c_set_bus_speed(void *hdl, unsigned int speed, unsigned int *ospeed);
int          imx_i2c_version_info(i2c_libversion_t *version);
int          imx_i2c_driver_info(void *hdl, i2c_driver_info_t *info);

int          imx_i2c_options(imx_dev_t *dev, int argc, char *argv[]);
int          imx_devctl(void *hdl, int cmd, void *msg, int msglen, int *nbytes, int *info);
void         imx_sendaddr7( imx_dev_t *dev );
void         imx_sendaddr10(imx_dev_t *dev);
i2c_status_t imx_sendbyte(imx_dev_t *dev, uint8_t byte);
i2c_status_t imx_recvbyte(imx_dev_t *dev, uint8_t *byte, int nack, int stop);
void         imx_i2c_reset(imx_dev_t *dev);
int          imx_wait_bus_idle(imx_dev_t *dev);

const struct sigevent *imx_i2c_intr(void *context, int id);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/i2c/imx/proto.h $ $Rev: 810496 $")
#endif

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

#ifndef	NXP_H
#define	NXP_H

/* NXP TJA1100 Broadreach extended register set */

#define	BASIC_CONTROL					0
	#define	BC_RESET				(1 << 15)
	#define	BC_LOOPB				(1 << 14)
	#define	BC_SPEED_SEL				(1 << 13)
	#define	BC_DUPLEX				(1 << 8)

#define	BASIC_STATUS					1

#define	EXT_STATUS_REG					15

#define	EXT_CONTROL_REG					17
	#define	LINK_CONTROL				(1 << 15)
	#define	POWER_MODE_SHIFT			11
	#define	NORMAL_MODE				0x3
	#define	STANDBY_MODE				0xc
	#define	SLEEP_REQUEST				0xb
	#define	SLAVE_JITTER_TEST			(1 << 10)
	#define	TRAINING_RESTART			(1 << 9)
	#define	TEST_MODE_SHIFT				6
	#define	CABLE_TEST				(1 << 5)
	#define	LOOPBACK_MODE_SHIFT			3
	#define	INTERNAL_LOOPBACK			0x1
	#define	EXTERNAL_LOOPBACK			0x2
	#define	REMOTE_LOOPBACK				0x3
	#define	CONFIG_ENABLE				(1 << 2)
	#define	CONFIG_INH				(1 << 1)
	#define	WAKE_REQUEST				(1 << 0)

#define	CONFIG_REG_1					18
	#define	MASTER_SLAVE				(1 << 15)
	#define	AUTO_OP					(1 << 14)
	#define	LINK_LENGTH_SHIFT			12
	#define	TX_AMPLITUDE_SHIFT			10
	#define	AMP_500MV				0x0
	#define	AMP_750MV				0x1
	#define	AMP_1000MV				0x2
	#define	AMP_1250MV				0x3
	#define	MII_MODE_SHIFT				8
	#define	MII_MODE				0x0
	#define	RMII_MODE_50				0x1
	#define	RMII_MODE_25				0x2
	#define	REV_MII_MODE				0x3
	#define	MII_DRIVER				(1 << 7)
	#define	SLEEP_CONFIRM				(1 << 6)
	#define	LED_MODE_SHIFT				4
	#define	LED_ENABLE				(1 << 3)
	#define	CONFIG_WAKE				(1 << 2)
	#define	AUTO_PWD				(1 << 1)
	#define	LPS_ACTIVE				(1 << 0)

#define	CONFIG_REG_2					19
	#define	PHY_ADDR_SHIFT				11
	#define	SNR_AVERAGING_SHIFT			9
	#define	SNR_WLIMIT_SHIFT			6
	#define	SNR_FAIL_LIMIT_SHIFT			3
	#define	JUMBO_ENABLE				(1 << 2)
	#define	SLEEP_REQ_TO_P4				0
	#define	SLEEP_REQ_TO_1				1
	#define	SLEEP_REQ_TO_4				2
	#define	SLEEP_REQ_TO_16				3

#define	SYMBOL_ERR_CNT_REG				20

#define	INT_STATUS_REG					21
	#define	POW_ON					(1 << 15)
	#define	WAKEUP					(1 << 14)
	#define	WUR_RECEIVED				(1 << 13)
	#define	LPS_RECEIVED				(1 << 12)
	#define	PHY_INIT_FAIL				(1 << 11)
	#define	LINK_STATUS_FAIL			(1 << 10)
	#define	LINK_STATUS_UP				(1 << 9)
	#define	SYM_ERR					(1 << 8)
	#define	TRAINING_FAILED				(1 << 7)
	#define	SNR_WARNING				(1 << 6)
	#define	CONTROL_ERR				(1 << 5)
	#define	TXEN_CLAMPED				(1 << 4)
	#define	UV_ERR					(1 << 3)
	#define	UV_RECOVERY				(1 << 2)
	#define	TEMP_ERR				(1 << 1)
	#define	SLEEP_ABORT				(1 << 0)
#define	PHY_INT_ERR	(PHY_INIT_FAIL | LINK_STATUS_FAIL | SYM_ERR | CONTROL_ERR | UV_ERR | TEMP_ERR)

#define	INT_ENABLE_REG					22

#define	COMM_STATUS_REG					23
	#define	LINK_UP					(1 << 15)
	#define	TX_MODE_SHIFT				13
	#define	TX_DIS					0x0
	#define	TX_SEND_N				0x1
	#define	TX_SEND_I				0x2
	#define	TX_SEND_Z				0x3
	#define	LOC_RX_STATUS				(1 << 12)
	#define	REM_RX_STATUS				(1 << 11)
	#define	SCR_LOCKED				(1 << 10)
	#define	SSD_ERR					(1 << 9)
	#define	ESD_ERR					(1 << 8)
	#define	SNR_SHIFT				5
	#define	SNR_MASK				(0x07 << SNR_SHIFT)
	#define	RX_ERR					(1 << 4)
	#define	TX_ERR					(1 << 3)
	#define	PHY_STATE_IDLE				0x0
	#define	PHY_STATE_INIT				0x1
	#define	PHY_STATE_CONFIG			0x2
	#define	PHY_STATE_OFFLINE			0x3
	#define	PHY_STATE_ACTIVE			0x4
	#define	PHY_STATE_ISOLATE			0x5
	#define	PHY_CABLE_TEST				0x6
	#define	PHY_TEST_MODE				0x7
#define	COMM_STATUS_ERR	(SSD_ERR | ESD_ERR | RX_ERR | TX_ERR)

#define	GEN_STATUS_REG					24
	#define	INT_STATUS				(1 << 15)
	#define	PLL_LOCKED				(1 << 14)
	#define	LOCAL_WU				(1 << 13)
	#define	REMOTE_WU				(1 << 12)
	#define	DATA_DET_WU				(1 << 11)
	#define	EN_STATUS				(1 << 10)
	#define	RESET_STATUS				(1 << 9)
	#define	LINK_FAIL_CNT_SHIFT			3

#define	EXTERN_STATUS_REG				25
	#define	UV_VDDA_3V3				(1 << 14)
	#define	UV_VDDD_1V8				(1 << 13)
	#define	UV_VDDA_1V8				(1 << 12)
	#define	UV_VDDIO				(1 << 11)
	#define	TEMP_HIGH				(1 << 10)
	#define	TEMP_WARN				(1 << 9)
	#define	SHORT_DETECT				(1 << 8)
	#define	OPEN_DETECT				(1 << 7)
	#define	POL_DETECT				(1 << 6)
	#define	INTL_DETECT				(1 << 5)

#define	LINK_FAIL_CNT_REG				26

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/mx6x/nxp.h $ $Rev: 806401 $")
#endif

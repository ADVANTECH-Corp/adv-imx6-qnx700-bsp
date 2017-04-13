/*
 * $QNXLicenseC:
 * Copyright 2013-2014, QNX Software Systems.
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

#ifndef _SDMMC_H_
#define _SDMMC_H_

/*
 * SDMMC Block Size
 */
#define SDMMC_BLOCKSIZE				512

/* MultiMediaCard Command definitions */
#define MMC_GO_IDLE_STATE			0
#define MMC_SEND_OP_COND			1
#define MMC_ALL_SEND_CID			2
#define MMC_SET_RELATIVE_ADDR		3
#define MMC_SET_DSR					4
#define MMC_SWITCH					6
	/* For MMC CMD6 */
	#define MMC_SWITCH_MODE_OFF		24
	#define MMC_SWITCH_INDEX_OFF	16
	#define MMC_SWITCH_VALUE_OFF	8

	#define MMC_SWITCH_MODE_WRITE	0x3
	#define MMC_SWITCH_MODE_CLR		0x2
	#define MMC_SWITCH_MODE_SET		0x1
	#define MMC_SWITCH_MODE_CMD_SET 0x0
	#define MMC_SWITCH_CMDSET_DFLT	0x1

	#define ECSD_PART_CONFIG		179
	#define ECSD_HS_TIMING			185
	#define ECSD_HS_TIMING_HS200	0x2
	#define ECSD_HS_TIMING_HS		0x1
	#define ECSD_HS_TIMING_LS		0x0

	#define ECSD_BUS_WIDTH			183
	#define ECSD_BUS_WIDTH_1_BIT	0x0
	#define ECSD_BUS_WIDTH_4_BIT	0x1
	#define ECSD_BUS_WIDTH_8_BIT	0x2
	#define ECSD_BUS_WIDTH_DDR_4_BIT	0x5
	#define ECSD_BUS_WIDTH_DDR_8_BIT	0x6
	/* End of MMC CMD6 */

	/* For SD CMD6 */
	#define SD_SF_MODE_SET			0x1
	#define SD_SF_MODE_CHECK		0x0
	#define SD_SF_MODE_OFF			31

	#define SD_SF_GRP_BUS_SPD		0x0		// Bus Speed
	#define SD_SF_GRP_CMD_EXT		0x1		// Command System Extension
	#define SD_SF_GRP_DRV_STR		0x2		// Driver Strength
	#define SD_SF_GRP_CUR_LMT		0x3		// Current Limit
	#define SD_SF_GRP_DFT_VALUE		0xf
	#define SD_SF_GRP_WIDTH			0x4

	#define SD_SF_STATUS_SIZE		64
	/* End of SD CMD6 */

#define MMC_SEL_DES_CARD			7
#define MMC_IF_COND					8
#define MMC_SEND_EXT_CSD			8
	#define MMC_EXT_CSD_SIZE		512
#define MMC_SEND_CSD				9
#define MMC_SEND_CID				10
#define MMC_READ_DAT_UNTIL_STOP		11
#define MMC_STOP_TRANSMISSION		12
#define MMC_SEND_STATUS				13
#define MMC_GO_INACTIVE_STATE		15
#define MMC_SET_BLOCKLEN			16
#define MMC_READ_SINGLE_BLOCK		17
#define MMC_READ_MULTIPLE_BLOCK		18
#define MMC_WRITE_DAT_UNTIL_STOP	20
#define MMC_WRITE_BLOCK				24
#define MMC_WRITE_MULTIPLE_BLOCK	25
#define MMC_PROGRAM_CID				26
#define MMC_PROGRAM_CSD				27
#define MMC_SET_WRITE_PROT			28
#define MMC_CLR_WRITE_PROT			29
#define MMC_SEND_WRITE_PROT			30
#define MMC_TAG_SECTOR_START		32
#define MMC_TAG_SECTOR_END			33
#define MMC_UNTAG_SECTOR			34
#define MMC_TAG_ERASE_GROUP_START	35
#define MMC_TAG_ERASE_GROUP_END		36
#define MMC_UNTAG_ERASE_GROUP		37
#define MMC_ERASE					38
#define MMC_FAST_IO					39
#define MMC_GO_IRQ_STATE			40
#define MMC_LOCK_UNLOCK				42
#define MMC_SEND_SCR				51
	#define SD_SCR_SIZE				8
#define MMC_APP_CMD					55
#define MMC_GEN_CMD					56
#define MMC_READ_OCR				58
#define MMC_CRC_ON_OFF				59

#define SD_SET_BUS_WIDTH			((55 << 8) | 6)
#define SD_SEND_OP_COND				((55 << 8) | 41)

/*
 * SDMMC error codes
 */
#define SDMMC_OK					0		// no error
#define SDMMC_ERROR					-1		// SDMMC error

/* Card Status Response Bits */
#define MMC_OUT_OF_RANGE			(1 << 31)
#define MMC_ADDRESS_ERROR			(1 << 30)
#define MMC_BLOCK_LEN_ERROR			(1 << 29)
#define MMC_ERASE_SEQ_ERROR			(1 << 28)
#define MMC_ERASE_PARAM				(1 << 27)
#define MMC_WP_VIOLATION			(1 << 26)
#define MMC_CARD_IS_LOCKED			(1 << 25)
#define MMC_LOCK_UNLOCK_FAILED		(1 << 24)
#define MMC_COM_CRC_ERROR			(1 << 23)
#define MMC_ILLEGAL_COMMAND			(1 << 22)
#define MMC_CARD_ECC_FAILED			(1 << 21)
#define MMC_CC_ERROR				(1 << 20)
#define MMC_ERROR					(1 << 19)
#define MMC_UNDERRUN				(1 << 18)
#define MMC_OVERRUN					(1 << 17)
#define MMC_CID_CSD_OVERWRITE		(1 << 16)
#define MMC_WP_ERASE_SKIP			(1 << 15)
#define MMC_CARD_ECC_DISABLED		(1 << 14)
#define MMC_ERASE_RESET				(1 << 13)

/* Bits 9-12 define the CURRENT_STATE */
#define MMC_IDLE					(0 << 9)
#define MMC_READY					(1 << 9)
#define MMC_IDENT					(2 << 9)
#define MMC_STANDBY					(3 << 9)
#define MMC_TRAN					(4 << 9)
#define MMC_DATA					(5 << 9)
#define MMC_RCV						(6 << 9)
#define MMC_PRG						(7 << 9)
#define MMC_DIS						(8 << 9)
#define MMC_CURRENT_STATE			(0xF << 9)
/* End CURRENT_STATE */

#define MMC_READY_FOR_DATA			(1 << 8)
#define MMC_SWITCH_ERROR			(1 << 7)
#define MMC_URGENT_BKOPS			(1 << 6)
#define MMC_APP_CMD_S				(1 << 5)

/* Bus mode */
#define BUS_MODE_OPEN_DRAIN			0
#define BUS_MODE_PUSH_PULL			1

/*
 * SDMMC timing
 */
typedef enum
{
	TIMING_LS = 1,
	TIMING_HS,
	TIMING_DDR50,
	TIMING_SDR12,
	TIMING_SDR25,
	TIMING_SDR50,
	TIMING_SDR104,
	TIMING_HS200
} timing_t;

/* Max clock frequencies (KHz) */
#define DTR_MAX_SDR104				208000
#define DTR_MAX_SDR50				100000
#define DTR_MAX_DDR50				50000
#define DTR_MAX_SDR25				50000
#define DTR_MAX_SDR12				25000
#define DTR_MAX_HS200				200000
#define DTR_MAX_HS52				52000
#define DTR_MAX_HS26				26000

/*
 * SDMMC card type
 */
typedef enum
{
	NONE = 0, eMMC, eSDC, eSDC_V200, eSDC_HC, eMMC_HC
} card_type_t;

/*
 * SDMMC command structure
 */
typedef struct
{
	int			cmd;		// command to be issued
	unsigned	arg;		// command argument
	unsigned	*rsp;		// pointer to response buffer
	unsigned	bsize;		// data block size
	unsigned	bcnt;		// data block count
	void		*dbuf;		// pointer to data buffer
	short		erintsts;	// interrupt error status
} sdmmc_cmd_t;

#define IS_EMMC_CARD(_card)			((_card->type == eMMC) || (_card->type == eMMC_HC))
#define IS_SD_CARD(_card)			((_card->type == eSDC) || (_card->type == eSDC_V200) || (_card->type == eSDC_HC))
#define RELATIVE_CARD_ADDR(_sdmmc)	(_sdmmc->card.rca << 16)

/*
 * create a command structure
 */
#define CMD_CREATE(_cr, _cmd, _arg, _rsp, _bsize, _bcnt, _dbuf)	\
	do {						\
		(_cr).cmd = (_cmd);		\
		(_cr).arg = (_arg);		\
		(_cr).rsp = (_rsp);		\
		(_cr).bsize = (_bsize);	\
		(_cr).bcnt = (_bcnt);	\
		(_cr).dbuf = (_dbuf);	\
		(_cr).erintsts = 0;		\
	} while (0)

/*
 * SD/MMC CID
 */
typedef struct _sdmmc_cid_t {
	unsigned		mid;		// Manufacture ID
	unsigned		oid;		// OEM/Application ID
	unsigned char	pnm[8];		// Product name
	unsigned		prv;		// Product revision
	unsigned		psn;		// Product serial number
	unsigned		mdt;		// Manufacture date
} sdmmc_cid_t;

/*
 * SD/MMC CSD
 */
typedef struct _sdmmc_csd_t {
	unsigned char	csd_structure;	// CSD structure

#define CSD_SPEC_VER_0			0	// 1.0 - 1.2 (MMC)
#define CSD_SPEC_VER_1			1	// 1.4 -
#define CSD_SPEC_VER_2			2	// 2.0 - 2.2
#define CSD_SPEC_VER_3			3	// 3.1 - 3.2 - 3.31
#define CSD_SPEC_VER_4			4	// 4.0 - 4.1
	unsigned char	spec_vers;
	unsigned char	tran_speed;
	unsigned		ccc;
	unsigned char	read_bl_len;
	unsigned short	c_size;
	unsigned char	c_size_mult;
} sdmmc_csd_t;

/*
 * SD SCR
 */
typedef struct _sd_scr_t {
	unsigned char	scr_structure;	// SCR structure

#define SCR_SPEC_VER_0			0	// Version 1.0 - 1.01
#define SCR_SPEC_VER_1			1	// Version 1.10
#define SCR_SPEC_VER_2			2	// Version 2.0 or Version 3.0X
#define SCR_SPEC_VER_3			3	// Version 40
	unsigned char	sd_spec;		// Physical layer specification
	unsigned char	sd_bus_widths;
	unsigned char	sd_spec3;
} sd_scr_t;

/*
 * MMC Extend CSD
 */
typedef struct _mmc_ecsd_t {
#define ECSD_CARD_TYPE					196
	#define ECSD_CARD_TYPE_SDR_1_2V		(1<<5)	// Card can run at SDR 200MHz 1.2V
	#define ECSD_CARD_TYPE_SDR_1_8V		(1<<4)	// Card can run at SDR 200MHz 1.8V
	#define ECSD_CARD_TYPE_DDR_1_2V		(1<<3)	// Card can run at 52MHz 1.2V
	#define ECSD_CARD_TYPE_DDR_1_8V		(1<<2)	// Card can run at 52MHz 1.8V - 3.0V
	#define ECSD_CARD_TYPE_52			(1<<1)	// Card can run at 52MHz
	#define ECSD_CARD_TYPE_26			(1<<0)	// Card can run at 26MHz
	#define ECSD_CARD_TYPE_DDR			(ECSD_CARD_TYPE_DDR_1_8V | ECSD_CARD_TYPE_DDR_1_2V)
	#define ECSD_CARD_TYPE_52MHZ		(ECSD_CARD_TYPE_DDR_1_8V | ECSD_CARD_TYPE_DDR_1_2V | ECSD_CARD_TYPE_52)
	#define ECSD_CARD_TYPE_MSK			0x3f
	unsigned card_type;

#define ECSD_REV				192
	#define ECSD_REV_V4_5		6
	#define ECSD_REV_V4_41		5
	#define ECSD_REV_V4_4		4
	#define ECSD_REV_V4_3		3
	#define ECSD_REV_V4_2		2
	#define ECSD_REV_V4_1		1
	#define ECSD_REV_V4			0
	unsigned ext_csd_rev;
} mmc_ecsd_t;

/*
 * SDMMC card structure
 */
typedef struct card_s
{
	unsigned		state;		// current state
	card_type_t		type;		// card type
	unsigned		blk_size;	// block size
	unsigned		blk_num;	// number of blocks
	unsigned		speed;		// clock frequency in kHz
	unsigned short	rca;		// relative card address

/* The followings only apply to SD card */
#define SD_BUS_MODE_LS			(1 << 0)
#define SD_BUS_MODE_SDR12		(1 << 0)
#define SD_BUS_MODE_HS			(1 << 1)
#define SD_BUS_MODE_SDR25		(1 << 1)
#define SD_BUS_MODE_SDR50		(1 << 2)
#define SD_BUS_MODE_SDR104		(1 << 3)
#define SD_BUS_MODE_DDR50		(1 << 4)
#define SD_BUS_MODE_UHS			(SD_BUS_MODE_SDR50 | SD_BUS_MODE_SDR104 | SD_BUS_MODE_DDR50)
#define SD_BUS_MODE_MSK			0x1f
	unsigned	bus_mode;

#define SD_DRV_TYPE_B			0x01
#define SD_DRV_TYPE_A			0x02
#define SD_DRV_TYPE_C			0x04
#define SD_DRV_TYPE_D			0x08
#define SD_DRV_TYPE_MSK			0x0f
	unsigned	drv_type;

#define SD_CURR_LIMIT_200		(1 << 0)
#define SD_CURR_LIMIT_400		(1 << 1)
#define SD_CURR_LIMIT_600		(1 << 2)
#define SD_CURR_LIMIT_800		(1 << 3)
#define SD_CURR_LIMIT_MSK		0xf
	unsigned	curr_limit;
} card_t;

typedef struct _sdmmc		sdmmc_t;
typedef struct _sdmmc_hc	sdmmc_hc_t;

/*
 * SDMMC host controller structure
 */
struct _sdmmc_hc
{
	unsigned	sdmmc_pbase;		// Base address
	unsigned	clock;				// SDHC clock
	void		*dma_ext;			// Specific DMA handler
	void		(*set_clk)(sdmmc_t *, unsigned);
	void		(*set_bus_width)(sdmmc_t *, int);
	void		(*set_bus_mode)(sdmmc_t *, int);
	void		(*set_timing)(sdmmc_t *, int);
	int			(*send_cmd)(sdmmc_t *);
	int			(*pio_read)(sdmmc_t *, void *, unsigned);
	int			(*init_hc)(sdmmc_t *);
	int			(*dinit_hc)(sdmmc_t *);
	int			(*signal_voltage)(sdmmc_t *, int);
	int			(*tuning)(sdmmc_t *, int);

	/* DMA */
	int			(*max_blks)(sdmmc_t *sdmmc);	// Maximum blocks for one DMA transfer?
	int			(*setup_dma)(sdmmc_t *sdmmc);	// Setup DMA transfer
	int			(*dma_done)(sdmmc_t *sdmmc);	// Waiting for DMA finish
};

/*
 * SDMMC device structure
 */
struct _sdmmc
{
	sdmmc_hc_t	*hc;

#define SD_SIC_VHS_27_36V		0x1aa
	unsigned	icr;			// interface condition register

#define OCR_VDD_17_195			(1 << 7)	// VDD 1.7 - 1.95
#define OCR_VDD_20_21			(1 << 8)	// VDD 2.0 - 2.1
#define OCR_VDD_21_22			(1 << 9)	// VDD 2.1 - 2.2
#define OCR_VDD_22_23			(1 << 10)	// VDD 2.2 - 2.3
#define OCR_VDD_23_24			(1 << 11)	// VDD 2.3 - 2.4
#define OCR_VDD_24_25			(1 << 12)	// VDD 2.4 - 2.5
#define OCR_VDD_25_26			(1 << 13)	// VDD 2.5 - 2.6
#define OCR_VDD_26_27			(1 << 14)	// VDD 2.6 - 2.7
#define OCR_VDD_27_28			(1 << 15)	// VDD 2.7 - 2.8
#define OCR_VDD_28_29			(1 << 16)	// VDD 2.8 - 2.9
#define OCR_VDD_29_30			(1 << 17)	// VDD 2.9 - 3.0
#define OCR_VDD_30_31			(1 << 18)	// VDD 3.0 - 3.1
#define OCR_VDD_31_32			(1 << 19)	// VDD 3.1 - 3.2
#define OCR_VDD_32_33			(1 << 20)	// VDD 3.2 - 3.3
#define OCR_VDD_33_34			(1 << 21)	// VDD 3.3 - 3.4
#define OCR_VDD_34_35			(1 << 22)	// VDD 3.4 - 3.5
#define OCR_VDD_35_36			(1 << 23)	// VDD 3.5 - 3.6
#define OCR_S18R				(1 << 24)	// Switching to 1.8V Request
#define OCR_S18A				(1 << 24)	// Switching to 1.8V Accepted
#define OCR_XPC					(1 << 28)	// SDXC Power Control > 150ma
#define OCR_UHS_II				(1 << 29)	// UHS-II Card
#define OCR_HCS					(1 << 30)	// Card Capacity Status
#define OCR_PWRUP_CMP			(1 << 31)
	unsigned	ocr;						// operation condition register

#define SDMMC_CAP_HS			(1 << 0)
#define SDMMC_CAP_DDR			(1 << 1)
#define SDMMC_CAP_SDR			(1 << 2)
#define SDMMC_CAP_HS200			(1 << 3)
/* We don't support HS200 for now */
#define SDMMC_CAP_UHS			(SDMMC_CAP_DDR | SDMMC_CAP_SDR)
#define SDMMC_CAP_SKIP_IDENT	(1 << 31)
	unsigned	caps;

	sdmmc_cmd_t	cmd;			// command request structure
	card_t		card;			// card structure

	sdmmc_cid_t	cid;			// CID
	sdmmc_csd_t	csd;			// CSD
	sd_scr_t	scr;			// SD SCR

	mmc_ecsd_t		mmc_ecsd;	// Ext CSD

#define SDMMC_VERBOSE_LVL_0		0	// Error only
#define SDMMC_VERBOSE_LVL_1		1	// Warnings and some info
#define SDMMC_VERBOSE_LVL_2		2	// More info, such as clock, bus width, etc.
#define SDMMC_VERBOSE_LVL_3		3	// Command details
	int		verbose;
	int		timing;
};

/*
* SDMM functions
*/
extern int sdmmc_write(void *ext, void *buf, unsigned blkno, unsigned blkcnt);
extern int sdmmc_read (void *ext, void *buf, unsigned blkno, unsigned blkcnt);
extern int sdmmc_init_sd(sdmmc_t *sdmmc);
extern int sdmmc_init_mmc(sdmmc_t *sdmmc);
extern int sdmmc_powerup_card(sdmmc_t *sdmmc);
extern int sdmmc_fini(sdmmc_t *sdmmc);
#endif /* #ifndef _SDMMC_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/private/sdmmc.h $ $Rev: 808052 $")
#endif

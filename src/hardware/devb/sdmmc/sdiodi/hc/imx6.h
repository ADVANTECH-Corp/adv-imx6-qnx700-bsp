/*
 * $QNXLicenseC:
 * Copyright 2013, QNX Software Systems.
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

#ifndef	_SDHCI_H_INCLUDED
#define	_SDHCI_H_INCLUDED

#include <internal.h>

#define imx6_sdhcx_in8			in8
#define imx6_sdhcx_in16			in16
#define imx6_sdhcx_in32			in32
#define imx6_sdhcx_in32s		in32s
#define imx6_sdhcx_out8			out8
#define imx6_sdhcx_out16		out16
#define imx6_sdhcx_out32		out32
#define imx6_sdhcx_out32s		out32s

#define IMX6_SDHCX_TUNING_RETRIES	40
#define IMX6_SDHCX_CLOCK_TIMEOUT	10000
#define IMX6_SDHCX_COMMAND_TIMEOUT	1000000  /* The card could take very long time to process the transfered data */
#define IMX6_SDHCX_TRANSFER_TIMEOUT	1000000

#define IMX6_SDHCX_SIZE				4096

#define	IMX6_SDHCX_DS_ADDR			0x00

#define IMX6_CLOCK_DEFAULT			198000000

#define	IMX6_SDHCX_BLK_ATT			0x04
	#define IMX6_SDHCX_BLK_SDMA_BOUNDARY_512K	( 3 << 8 )
	#define IMX6_SDHCX_BLK_BLKSIZE_MASK			0x00001fff
	#define IMX6_SDHCX_BLK_BLKCNT_MASK			0xffff0000
	#define IMX6_SDHCX_BLK_BLKCNT_SHIFT			16

#define	IMX6_SDHCX_ARG				0x08

#define	IMX6_SDHCX_CMD				0x0C		// Command and transfer mode register
	#define	IMX6_SDHCX_CMD_RSP_TYPE_136	(1 << 16)	// Response length 136 bit
	#define	IMX6_SDHCX_CMD_RSP_TYPE_48	(2 << 16)	// Response length 48 bit
	#define	IMX6_SDHCX_CMD_RSP_TYPE_48b	(3 << 16)	// Response length 48 bit with busy after response
	#define	IMX6_SDHCX_CMD_CCCE			(1 << 19)	// Comamnd CRC Check Enable
	#define	IMX6_SDHCX_CMD_CICE			(1 << 20)	// Comamnd Index Check Enable
	#define	IMX6_SDHCX_CMD_DP			(1 << 21)	// Data Present
	#define	IMX6_SDHCX_CMD_TYPE_CMD12	(3 << 22)	// CMD12 or CMD52 "I/O Abort"

#define	IMX6_SDHCX_RESP0				0x10
#define	IMX6_SDHCX_RESP1				0x14
#define	IMX6_SDHCX_RESP2				0x18
#define	IMX6_SDHCX_RESP3				0x1C

#define	IMX6_SDHCX_DATA				0x20

#define	IMX6_SDHCX_PSTATE			0x24	// Present State
	#define IMX6_SDHCX_PSTATE_DLSL_MSK	(0xFF << 24)
 //Data line 0 level. Checked for card's busy state after write transaction
    #define IMX6_SDHCX_PSTATE_DL3SL     (1 << 27)
    #define IMX6_SDHCX_PSTATE_DL2SL     (1 << 26)
    #define IMX6_SDHCX_PSTATE_DL1SL     (1 << 25)
    #define IMX6_SDHCX_PSTATE_DL0SL     (1 << 24)
    #define IMX6_SDHCX_PSTATE_CLSL	    (1 << 23)
	#define	IMX6_SDHCX_PSTATE_WP		(1 << 19)
	#define	IMX6_SDHCX_PSTATE_CD		(1 << 18)
	#define	IMX6_SDHCX_PSTATE_CI		(1 << 16)
	#define	IMX6_SDHCX_PSTATE_RTR		(1 << 12)	// Re-Tuning Request
	#define	IMX6_SDHCX_PSTATE_BRE		(1 << 11)	// Buffer Read Ready
	#define	IMX6_SDHCX_PSTATE_BWE		(1 << 10)	// Buffer Write Ready
	#define	IMX6_SDHCX_PSTATE_RTA		(1 <<  9)	// Read Transfer Active
	#define	IMX6_SDHCX_PSTATE_WTA		(1 <<  8)	// Write Transfer Active
	#define	IMX6_SDHCX_PSTATE_SDOFF		(1 <<  7)	// SD clock gated off
	#define	IMX6_SDHCX_PSTATE_PEROFF	(1 <<  6)	// ipg_perclk gated off
	#define	IMX6_SDHCX_PSTATE_HCKOFF	(1 <<  5)	// hckl gated off
	#define	IMX6_SDHCX_PSTATE_IPGOFF	(1 <<  4)	// ipg_clk gated off
	#define	IMX6_SDHCX_PSTATE_SDSTB		(1 <<  3)	// SD clock stable
	#define	IMX6_SDHCX_PSTATE_DLA		(1 <<  2)	// data line active
	#define	IMX6_SDHCX_PSTATE_DATI		(1 <<  1)	// Command inhibit
	#define	IMX6_SDHCX_PSTATE_CMDI		(1 <<  0)	// Command inhibit

	#define	IMX6_SDHCX_CARD_STABLE		(IMX6_SDHCX_PSTATE_CD | \
                                         IMX6_SDHCX_PSTATE_SDSTB | \
                                         IMX6_SDHCX_PSTATE_CI)
#define	IMX6_SDHCX_DMA_IDLE         (IMX6_SDHCX_PSTATE_DL0SL | \
                                     IMX6_SDHCX_PSTATE_CLSL_MSK | \
                                     IMX6_SDHCX_PSTATE_DATI | \
                                     IMX6_SDHCX_PSTATE_CMDI)


#define	IMX6_SDHCX_PCTL				0x28	// Protocol Control
	#define	IMX6_SDHCX_PCTL_LED         (1 << 0)	// LED Control
	#define	IMX6_SDHCX_PCTL_DTW1		(0 << 1)	// Data Bus Width 1 bit
	#define	IMX6_SDHCX_PCTL_DTW4		(1 << 1)	// Data Bus Width 4 bit
	#define	IMX6_SDHCX_PCTL_DTW8		(2 << 1)	// Data Bus Width 8 bit
	#define	IMX6_SDHCX_PCTL_D3CD		(1 << 3)	// Data 3 card detection
	#define IMX6_SDHCX_PCTL_BEM		    (0 << 4)	// Big endian mode
	#define IMX6_SDHCX_PCTL_HWEM		(1 << 4)	// Half-word endian mode
	#define IMX6_SDHCX_PCTL_LEM		    (2 << 4)	// little endian mode
	#define	IMX6_SDHCX_PCTL_CDTL		(1 << 6)	// card detect test level
	#define	IMX6_SDHCX_PCTL_CDSS		(1 << 7)	// card detect signal sel
	#define IMX6_SDHCX_PCTL_DMA_MSK		(3 << 8)    // DMA select mask
	#define IMX6_SDHCX_PCTL_SDMA		(0 << 8)    // simple DMA select
	#define IMX6_SDHCX_PCTL_ADMA1		(1 << 8)    // ADMA1 select
	#define IMX6_SDHCX_PCTL_ADMA2		(2 << 8)    // ADMA2 select
	#define IMX6_SDHCX_PCTL_SABGREQ		(1 << 16)   // stop at block gap request
	#define IMX6_SDHCX_PCTL_CREQ		(1 << 17)   // Continue request
	#define IMX6_SDHCX_PCTL_RWCTL		(1 << 18)   // read wait control
	#define IMX6_SDHCX_PCTL_IABG		(1 << 19)   // interrupt at block gap
	#define IMX6_SDHCX_PCTL_RDN8		(1 << 20)   // read done no 8 clock
	#define IMX6_SDHCX_PCTL_WECINT		(1 << 24)   // wake event enable on interrupt
	#define IMX6_SDHCX_PCTL_WECINS		(1 << 25)   // wake event enable on insertion
	#define IMX6_SDHCX_PCTL_WECRM		(1 << 26)   // wake event enable on removal
	#define IMX6_SDHCX_PCTL_BLE			(1 << 27)   // burst length enabled for INCR
	#define IMX6_SDHCX_PCTL_BLE_16		(2 << 27)   // burst length enabled for INCR16
	#define IMX6_SDHCX_PCTL_BLE_16WRAP	(4 << 27)   // burst length enabled for INCR16 wrap

#define	IMX6_SDHCX_SYSCTL			0x2C	// Clock Control/Timeout/Software reset
	#define	IMX6_SDHCX_SYSCTL_DVS_SHIFT		(4)	// clock divisor shift
	#define	IMX6_SDHCX_SYSCTL_DVS_MSK		(0xf << 4)	// clock divisor
    #define IMX6_SDHCX_SYSCTL_SDCLKFS_SHIFT (8)
	#define IMX6_SDHCX_SYSCTL_SDCLKFS_MSK	0xff00
	#define	IMX6_SDHCX_SYSCTL_DTO_SHIFT		(16)	// Data timeout counter shift
	#define	IMX6_SDHCX_SYSCTL_DTO_MSK		(0xF << 16)	// Data timeout counter
	#define	IMX6_SDHCX_SYSCTL_DTO_MAX		(0xF << 16)	// Timeout = TCF x 2^27
	#define	IMX6_SDHCX_SYSCTL_SRA			(1 << 24)	// Software reset for all
	#define	IMX6_SDHCX_SYSCTL_SRC			(1 << 25)	// Software reset for mmci_cmd line
	#define	IMX6_SDHCX_SYSCTL_SRD			(1 << 26)	// Software reset for mmci_dat line
	#define	IMX6_SDHCX_SYSCTL_INITA			(1 << 27)	// Initialization active
	#define IMX6_SDHCX_CLK_MAX_DIV_SPEC_VER_3	2046

#define	IMX6_SDHCX_IS				0x30	// Interrupt status register
#define	IMX6_SDHCX_IE				0x34	// Interrupt SD enable register
#define	IMX6_SDHCX_ISE				0x38	// Interrupt signal enable register
	#define	IMX6_SDHCX_INTR_CC			(1 <<  0)	// Command Complete
	#define	IMX6_SDHCX_INTR_TC			(1 <<  1)	// Transfer Complete
	#define	IMX6_SDHCX_INTR_BGE			(1 <<  2)	// Block Gap Event
	#define IMX6_SDHCX_INTR_DMA			(1 <<  3)	// DMA interupt
	#define	IMX6_SDHCX_INTR_BWR			(1 <<  4)	// Buffer Write Ready interrupt
	#define	IMX6_SDHCX_INTR_BRR			(1 <<  5)	// Buffer Read Ready interrupt
	#define IMX6_SDHCX_INTR_CINS		(1 <<  6)	// Card Insertion
	#define IMX6_SDHCX_INTR_CREM		(1 <<  7)	// Card Removal
	#define	IMX6_SDHCX_INTR_CIRQ		(1 <<  8)	// Card interrupt
	#define IMX6_SDCHX_INTR_RETUNE		(1 << 12)	// Re-Tuning
	#define IMX6_SDCHX_INTR_TP			(1 << 14)	// Re-Tuning
	#define	IMX6_SDHCX_INTR_CTO			(1 << 16)	// Command Timeout error
	#define	IMX6_SDHCX_INTR_CCRC		(1 << 17)	// Command CRC error
	#define	IMX6_SDHCX_INTR_CEB			(1 << 18)	// Command End Bit error
	#define	IMX6_SDHCX_INTR_CIE			(1 << 19)	// Command Index error
	#define	IMX6_SDHCX_INTR_DTO			(1 << 20)	// Data Timeout error
	#define	IMX6_SDHCX_INTR_DCRC		(1 << 21)	// Data CRC error
	#define	IMX6_SDHCX_INTR_DEB			(1 << 22)	// Data End Bit error
	#define	IMX6_SDHCX_INTR_ACE			(1 << 24)	// ACMD12 error
	#define IMX6_SDHCX_INTR_TNE		    (1 << 26)	// Tuning  Error
    #define IMX6_SDHCX_INTR_DMAE	    (1 << 28)	// DMA Error

	#define IMX6_SDHCX_IE_DFLTS			(IMX6_SDHCX_INTR_CTO | IMX6_SDHCX_INTR_CCRC \
										| IMX6_SDHCX_INTR_CEB | IMX6_SDHCX_INTR_CIE \
										| IMX6_SDHCX_INTR_ACE \
										| IMX6_SDHCX_INTR_CINS | IMX6_SDHCX_INTR_CREM)

	#define IMX6_SDHCX_IS_ERRI		    (IMX6_SDHCX_INTR_DMAE \
										| IMX6_SDHCX_INTR_CCRC | IMX6_SDHCX_INTR_CTO \
										| IMX6_SDHCX_INTR_CEB | IMX6_SDHCX_INTR_CIE \
										| IMX6_SDHCX_INTR_DTO | IMX6_SDHCX_INTR_DCRC \
										| IMX6_SDHCX_INTR_DEB | IMX6_SDHCX_INTR_ACE)
	#define IMX6_SDHCX_ISE_DFLTS		0x117FF1FF
	#define IMX6_SDHCX_INTR_CLR_ALL		0xffffffff

#define	IMX6_SDHCX_AC12				0x3C

#define	IMX6_SDHCX_CAP				0x40	// Capability Registers bits 0 to 31
	#define	IMX6_SDHCX_CAP_S18			(1 << 26)	// 1.8V support
	#define	IMX6_SDHCX_CAP_S30			(1 << 25)	// 3.0V support
	#define	IMX6_SDHCX_CAP_S33			(1 << 24)	// 3.3V support
	#define	IMX6_SDHCX_CAP_SRS			(1 << 23)	// Suspend/Resume support
	#define	IMX6_SDHCX_CAP_DMA			(1 << 22)	// DMA support
	#define	IMX6_SDHCX_CAP_HS			(1 << 21)	// High-Speed support
	#define	IMX6_SDHCX_CAP_ADMAS		(1 << 20)	// ADMA support
	#define	IMX6_SDHCX_CAP_MBL512		(0 << 16)	// Max block length 512
	#define	IMX6_SDHCX_CAP_MBL1024		(1 << 16)	// Max block length 512
	#define	IMX6_SDHCX_CAP_MBL2048		(2 << 16)	// Max block length 2048
	#define	IMX6_SDHCX_CAP_MBL4096		(3 << 16)	// Max block length 2048

#define IMX6_SDHCX_CAP_BASE_CLK( _c, _v )	( ( ( _v ) >= IMX6_SDHCX_SPEC_VER_3 ? \
											( ( ( _c ) >> 8 ) & 0xff ) :			\
											( ( ( _c ) >> 8 ) & 0x3f ) ) * 1000 * 1000 )

#define IMX6_SDHCX_WATML                     0x44
  #define IMX6_SDHCX_WATML_WRBRSTLENSHIFT           24
  #define IMX6_SDHCX_WATML_WRBRSTLENMASK    0x1F000000
  #define IMX6_SDHCX_WATML_WRWMLSHIFT               16
  #define IMX6_SDHCX_WATML_WRWMLMASK          0xFF0000
  #define IMX6_SDHCX_WATML_RDBRSTLENSHIFT            8
  #define IMX6_SDHCX_WATML_RDBRSTLENMASK        0x1F00
  #define IMX6_SDHCX_WATML_RDWMLSHIFT                0
  #define IMX6_SDHCX_WATML_RDWMLMASK              0xFF
  #define IMX6_SDHCX_WATML_WR						(128)
  #define IMX6_SDHCX_WATML_RD						IMX6_SDHCX_WATML_WR

#define IMX6_SDHCX_MIX_CTRL            0x48
	#define	IMX6_SDHCX_MIX_CTRL_DE			(1 <<  0)	// DMA Enable
	#define	IMX6_SDHCX_MIX_CTRL_BCE			(1 <<  1)	// Block Count Enable
	#define	IMX6_SDHCX_MIX_CTRL_ACMD12		(1 <<  2)	// Auto CMD12 Enable
	#define	IMX6_SDHCX_MIX_CTRL_DDR_EN		(1 <<  3)	// DDR enable
	#define	IMX6_SDHCX_MIX_CTRL_DDIR		(1 <<  4)	// Data Transfer Direction Read
	#define	IMX6_SDHCX_MIX_CTRL_MBS			(1 <<  5)	// Multi Block Select
	#define	IMX6_SDHCX_MIX_CTRL_ACMD23		(1 <<  7)	// Auto CMD23 Enable
	#define	IMX6_SDHCX_MIX_CTRL_EXE_TUNE	(1 << 22)
	#define	IMX6_SDHCX_MIX_CTRL_SMP_CLK_SEL	(1 << 23)
	#define	IMX6_SDHCX_MIX_CTRL_AUTOTUNE_EN	(1 << 24)
	#define	IMX6_SDHCX_MIX_CTRL_FBCLK_SEL	(1 << 25)
#define IMX6_SDHCX_FORCE_EVENT      0x50
#define	IMX6_SDHCX_FORCE_EVENT_INT      (1 << 31) // force interrupt
#define IMX6_SDHCX_ADMA_ES			0x54
#define IMX6_SDHCX_ADMA_ADDRL		0x58
#define IMX6_SDHCX_ADMA_ADDRH		0x5C

#define IMX6_SDHCX_DLL_CTRL			0x60
	#define IMX6_SDHCX_DLL_CTRL_ENABLE				(1 << 0)			
	#define IMX6_SDHCX_DLL_CTRL_RESET				(1 << 1)
	#define IMX6_SDHCX_DLL_CTRL_SLV_FORCE_UPD		(1 << 2)
	#define IMX6_SDHCX_DLL_CTRL_SLV_DLY_TARGET0_3	(1 << 3)
	#define IMX6_SDHCX_DLL_CTRL_GATE_UPDATE			(1 << 7)
	#define IMX6_SDHCX_DLL_CTRL_SLV_OVERIDE			(1 << 8)
	#define IMX6_SDHCX_DLL_CTRL_SLV_OVERIDE_VAL		(1 << 9)
	#define IMX6_SDHCX_DLL_CTRL_SLV_DLY_TARGET4_6	(1 << 16)
	#define IMX6_SDHCX_DLL_CTRL_SLV_UPDATE			(1 << 20)
	#define IMX6_SDHCX_DLL_CTRL_REF_UPDATE			(1 << 28)

#define IMX6_SDHCX_DLL_STATUS		0x64
	#define IMX6_SDHCX_DLL_STATUS_SLV_LOCK			(1 << 0)
	#define IMX6_SDHCX_DLL_STATUS_REF_LOCK			(1 << 1)
	#define IMX6_SDHCX_DLL_STATUS_SLV_SEL			(1 << 2)
	#define IMX6_SDHCX_DLL_STATUS_REF_SEL			(1 << 9)

#define IMX6_SDHCX_TUNE_CTRL_STATUS	0x68
	#define IMX6_SDHCI_TUNE_DLY_CELL_SET_PRE_OFF	8
	#define IMX6_SDHCI_TUNE_DLY_CELL_SET_PRE_MSK	(0x7F00)
	#define IMX6_SDHCI_TUNE_CTRL_MIN				0x0
	#define IMX6_SDHCI_TUNE_CTRL_MAX				((1 << 7) - 1)
	#define IMX6_SDHCI_TUNE_STEP					1

#define IMX6_SDHCX_VEND_SPEC        0xC0
	#define	IMX6_SDHCX_VEND_SPEC_SDVS1V8		(1 << 1)	// SD bus voltage 1.8V
	#define	IMX6_SDHCX_VEND_SPEC_SDVS3V0		(0 << 1)	// SD bus voltage 3.0V
	#define	IMX6_SDHCX_VEND_SPEC_SDVS_MSK		(1 << 1)
	#define	IMX6_SDHCX_VEND_SPEC_FRC_SDCLK_ON	(1 << 8)	// SD force SDCLK on

#define IMX6_SDHCX_MMC_BOOT         0xC4
#define IMX6_SDHCX_VEND_SPEC2       0xC8

#define	IMX6_SDHCX_SLOT_IS			0xFC

#define	IMX6_SDHCX_HCTL_VERSION		0xFC
	#define IMX6_SDHCX_SPEC_VER_MSK		0xff
	#define IMX6_SDHCX_SPEC_VER_3		0x3
	#define IMX6_SDHCX_SPEC_VER_1		0x0


#define IMX6_SDHCX_ADMA2_MAX_XFER	(1024 * 60)

// 32 bit ADMA descriptor defination
typedef struct _imx6_sdhcx_adma32_t {
	uint16_t	attr;
	uint16_t	len;
	uint32_t	addr;
} imx6_sdhcx_adma32_t;

#define IMX6_SDHCX_ADMA2_VALID	    (1 << 0)	// valid
#define IMX6_SDHCX_ADMA2_END		(1 << 1)    // end of descriptor, transfer complete interrupt will be generated
#define IMX6_SDHCX_ADMA2_INT		(1 << 2)	// generate DMA interrupt, will not be used
#define IMX6_SDHCX_ADMA2_NOP		(0 << 4)	// no OP, go to the next desctiptor
#define IMX6_SDHCX_ADMA2_SET		(1 << 4)	// no OP, go to the next desctiptor
#define IMX6_SDHCX_ADMA2_TRAN		(2 << 4)	// transfer data
#define IMX6_SDHCX_ADMA2_LINK	    (3 << 4)	// link to another descriptor

typedef	struct _imx6_usdhcx_hc {
	void			*bshdl;
	uintptr_t		base;
    imx6_sdhcx_adma32_t*  adma;
	uint32_t		admap;
#define SF_USE_SDMA			0x01
#define SF_USE_ADMA			0x02
#define SF_TUNE_SDR50		0x04
#define SF_SDMA_ACTIVE		0x10
#define ADMA_DESC_MAX		256
	sdio_sge_t		sgl[ADMA_DESC_MAX];
	uint32_t		flags;
	int				sdma_iid;	// SDMA interrupt id
	uintptr_t		sdma_irq;
	uintptr_t		sdma_base;
    uint32_t        mix_ctrl;
    uint32_t        intmask;
	_Uint64t		usdhc_addr;	// Used to determine which controller it belongs to
} imx6_sdhcx_hc_t;

extern int imx6_sdhcx_init( sdio_hc_t *hc );
extern int imx6_sdhcx_dinit( sdio_hc_t *hc );

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/sdiodi/hc/imx6.h $ $Rev: 743172 $")
#endif

/*
 * $QNXLicenseC:
 * Copyright 2007, 2008, QNX Software Systems.
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

#ifndef _DW_MSHC_H_
#define _DW_MSHC_H_

#include <internal.h>

#define DW_MS_ITER				8
#define DW_ROR8( _w, _s )		( ( (_w) >> (_s) ) | ( (_w) << (8 - (_s) ) ) )

#define DW_TUNING_RETRIES		40
#define DW_TUNING_TIMEOUT		150

#define DW_CTRL					0x000			// Control Register
	#define DW_CTRL_USE_IDMAC		(1 << 25)
	#define DW_CTRL_CEATA_INT_EN	(1 << 11)
	#define DW_CTRL_SEND_AS_CCSD	(1 << 10)
	#define DW_CTRL_SEND_CCSD		(1 << 9)
	#define DW_CTRL_ABRT_READ_DATA	(1 << 8)
	#define DW_CTRL_SEND_IRQ_RESP	(1 << 7)
	#define DW_CTRL_READ_WAIT		(1 << 6)
	#define DW_CTRL_DMA_ENABLE		(1 << 5)
	#define DW_CTRL_INT_ENABLE		(1 << 4)
	#define DW_CTRL_DMA_RESET		(1 << 2)
	#define DW_CTRL_FIFO_RESET		(1 << 1)
	#define DW_CTRL_RESET			(1 << 0)
	#define DW_CTRL_RESET_ALL		( DW_CTRL_RESET | DW_CTRL_FIFO_RESET | DW_CTRL_DMA_RESET )

#define DW_PWREN				0x004
	#define DW_PWREN_POWER_ENABLE	( 1 << 0 )

#define DW_CLKDIV				0x008
#define DW_CLKSRC				0x00c

#define DW_CLKENA				0x010
	#define DW_CLKEN_LOW_PWR		(1 << 16)
	#define DW_CLKEN_ENABLE			(1 << 0)

#define DW_TMOUT				0x014
	#define DW_TMOUT_DATA(n)		((n) << 8)
	#define DW_TMOUT_DATA_MSK		0xFFFFFF00
	#define DW_TMOUT_DATA_MAX		0xFFFFFF00
	#define DW_TMOUT_RESP(n)		((n) & 0xFF)
	#define DW_TMOUT_RESP_MSK		0xFF
	#define DW_TMOUT_RESP_MAX		0xFF

#define DW_CTYPE				0x018
	#define DW_CTYPE_8BIT			(1 << 16)
	#define DW_CTYPE_4BIT			(1 << 0)
	#define DW_CTYPE_1BIT			0

#define DW_BLKSIZ				0x01c
#define DW_BYTCNT				0x020
#define DW_INTMASK				0x024
#define DW_CMDARG				0x028
#define DW_CMD					0x02c
	#define DW_CMD_START			(1 << 31)
	#define DW_CMD_USE_HOLD_REG		(1 << 29)
	#define DW_CMD_VOLT_SWITCH		(1 << 28)
	#define DW_CMD_CCS_EXP			(1 << 23)
	#define DW_CMD_CEATA_RD			(1 << 22)
	#define DW_CMD_UPD_CLK			(1 << 21)
	#define DW_CMD_INIT				(1 << 15)
	#define DW_CMD_STOP				(1 << 14)
	#define DW_CMD_PRV_DAT_WAIT		(1 << 13)
	#define DW_CMD_SEND_STOP		(1 << 12)
	#define DW_CMD_STRM_MODE		(1 << 11)
	#define DW_CMD_DAT_WR			(1 << 10)
	#define DW_CMD_DAT_EXP			(1 << 9)
	#define DW_CMD_RESP_CRC			(1 << 8)
	#define DW_CMD_RESP_LONG		(1 << 7)
	#define DW_CMD_RESP_EXP			(1 << 6)
	#define DW_CMD_INDX(n)			((n) & 0x1F)

#define DW_RESP0				0x030
#define DW_RESP1				0x034
#define DW_RESP2				0x038
#define DW_RESP3				0x03c

#define DW_MINTSTS				0x040
#define DW_RINTSTS				0x044
	#define DW_INT_SDIO(n)		((1 << 16) + (n))
	#define DW_INT_EBE			(1 << 15)		// End Bit Error
	#define DW_INT_ACD			(1 << 14)		// Auto Command Done
	#define DW_INT_SBE			(1 << 13)		// Start Bit Error
	#define DW_INT_HLE			(1 << 12)		// Hardware Locked Write Error
	#define DW_INT_FRUN			(1 << 11)		// FIFO underrun/overrun error
	#define DW_INT_HTO			(1 << 10)		// Data starvation by Host Timeout
	#define DW_INT_VOLT_SWITCH	(1 << 10)		// Voltage Switch (same as HTO)
	#define DW_INT_DRTO			(1 << 9)		// Data Read Timeout
	#define DW_INT_RTO			(1 << 8)		// Response Timeout
	#define DW_INT_DCRC			(1 << 7)		// Data CRC Error
	#define DW_INT_RCRC			(1 << 6)		// Response CRC Error
	#define DW_INT_RXDR			(1 << 5)		// Receive FIFO Data Request
	#define DW_INT_TXDR			(1 << 4)		// Transmit FIFO Data Request
	#define DW_INT_DTO			(1 << 3)		// Data Transfer Over
	#define DW_INT_CD			(1 << 2)		// Command Done
	#define DW_INT_RE			(1 << 1)		// Response Error
	#define DW_INT_CDET			(1 << 0)		// Card Detect
	#define DW_INT_ERRORS		( DW_INT_RCRC | DW_INT_DCRC |	\
									DW_INT_RTO | DW_INT_DRTO | DW_INT_HTO |		\
									DW_INT_FRUN | DW_INT_HLE | DW_INT_SBE |		\
									DW_INT_EBE )
	#define DW_INT_DFLTS		( DW_INT_ERRORS | DW_INT_CDET )
									

#define DW_STATUS					0x048
	#define DW_STATUS_FIFO_COUNT( _x )	( ( ( _x ) >> 17 ) & 0x1FFF )
	#define DW_STATUS_FIFO_FULL			(1 << 3)
	#define DW_STATUS_FIFO_EMPTY		(1 << 2)

#define DW_FIFOTH					0x04c
      	#define DW_FIFOTH_RX_WMARK( _fifoth )	( ( ( _fifoth ) >> 16 ) & 0xfff )
	#define DW_FIFOTH_RX_WMARK_SHFT			16
	#define DW_FIFOTH_MSIZE_1				(0<<28)
	#define DW_FIFOTH_MSIZE_4				(1<<28)
	#define DW_FIFOTH_MSIZE_8				(2<<28)
	#define DW_FIFOTH_MSIZE_16				(3<<28)
	#define DW_FIFOTH_MSIZE_32				(4<<28)
	#define DW_FIFOTH_MSIZE_64				(5<<28)
	#define DW_FIFOTH_MSIZE_128				(6<<28)
	#define DW_FIFOTH_MSIZE_256				(7<<28)
	#define DW_FIFO_SZ_128					0x80
	#define DW_FIFO_SZ_32					0x20

#define DW_CDETECT					0x050
#define DW_WRTPRT					0x054
#define DW_GPIO						0x058
#define DW_TCBCNT					0x05c
#define DW_TBBCNT					0x060
#define DW_DEBNCE					0x064
	#define DW_DEBNCE_25MS			0xffffff

#define DW_USRID					0x068
#define DW_VERID					0x06c
	#define DW_VERID_CID(_x)		( (_x) >> 16 )
	#define DW_VERID_CREV(_x)		( (_x) & 0xffff )
	#define DW_VERID_CREV_240A		0x240a

#define DW_HCON						0x070
	#define DW_HCON_DATA_WIDTH( _hcon )		( ( ( _hcon ) >> 7 ) & 0x7 )
	#define DW_HCON_DATA_WIDTH16			0x00
	#define DW_HCON_DATA_WIDTH32			0x01
	#define DW_HCON_DATA_WIDTH64			0x02
	
#define DW_HCON_NUM_CARD( _hcon )		( ( ( _hcon ) >> 1 ) & 0x1f )

#define DW_UHS_REG					0x074
	#define DW_DDR(_x)				( ( 1 << ((_x) + 16) ) )
	#define DW_DDR_REG_MSK(_x)		( ~( 1 << ((_x) + 16) ) )
	#define DW_VOLT_1_8(_x)			( ( 1 << (_x) ) )
	#define DW_VOLT_REG_MSK(_x)		( ~( 1 << (_x) ) )

#define DW_RST						0x078
	#define DW_RST_ACTIVE			0x01

#define DW_BMOD						0x080			// Bus Mode
	#define DW_IDMAC_ENABLE			(1 << 7)
	#define DW_IDMAC_FB				(1 << 1)
	#define DW_IDMAC_SWRESET		(1 << 0)

#define DW_PLDMND					0x084
	#define DW_PLDMND_PD			1

#define DW_DBADDR					0x088

#define DW_IDSTS					0x08c
	#define DW_IDSTS_FSM_MSK			(0xf << 13)
	#define DMAC_FSM_DMA_IDLE			(0 << 13)
	#define DMAC_FSM_DMA_SUSPEND		(1 << 13)
	#define DMAC_FSM_DESC_RD			(2 << 13)
	#define DMAC_FSM_DESC_CHK			(3 << 13)
	#define DMAC_FSM_DMA_RD_REQ_WAIT	(4 << 13)
	#define DMAC_FSM_DMA_WR_REQ_WAIT	(5 << 13)
	#define DMAC_FSM_DMA_RD				(6 << 13)
	#define DMAC_FSM_DMA_WR				(7 << 13)
	#define DMAC_FSM_DESC_CLOSE			(8 << 13)

#define DW_IDINTEN					0x090	// Internal DMAC Interrupt Enable
	#define DW_IDMAC_INT_AI			(1 << 9)
	#define DW_IDMAC_INT_NI			(1 << 8)
	#define DW_IDMAC_INT_CES		(1 << 5)
	#define DW_IDMAC_INT_DU			(1 << 4)
	#define DW_IDMAC_INT_FBE		(1 << 2)
	#define DW_IDMAC_INT_RI			(1 << 1)
	#define DW_IDMAC_INT_TI			(1 << 0)
	#define DW_IDMAC_INT_MSK		(	DW_IDMAC_INT_TI | DW_IDMAC_INT_RI |	\
										DW_IDMAC_INT_FBE | DW_IDMAC_INT_DU	| \
										DW_IDMAC_INT_CES | DW_IDMAC_INT_NI	| \
										DW_IDMAC_INT_AI )

#define DW_DSCADDR					0x094
#define DW_BUFADDR					0x098
#define DW_CLKSEL					0x09c
	#define DW_DIVRATIO_4			4		// dflt divration used by (ddr/sdr)
	#define DW_DIVRATIO_MSK			0x07
	#define DW_DIVRATIO_SHFT		24
	#define DW_CLK_SMPL_MSK			0x07
	#define DW_CLK_SMPL_MAX			0x07

#define DW_CARDTHRCTL				0x100
	#define DW_CARDTHRCTL_EN		(1 << 0)
	#define DW_CARDTHRCTL_THRES(_x)	( (_x) << 16 )

#define DW_BACK_END_POWER			0x104
#define DW_EMMC_DDR_REG				0x10c
#define DW_DDR200_RDDQS_EN			0x110
#define DW_DDR200_ASYNC_FIFO_CTRL	0x114
#define DW_DDR200_DLINE_CTRL		0x118

//#define DW_DATA_220A				0x100
#define DW_DATA_240A				0x200
#define DW_DATA						DW_DATA_240A

typedef struct _dw_idmac_desc {
	uint32_t		des0;				// Control Descriptor
#define IDMAC_DES0_DIC	(1 << 1)		// Disable Interrupt on Completion
#define IDMAC_DES0_LD	(1 << 2)		// Last Descriptor
#define IDMAC_DES0_FD	(1 << 3)		// First Descriptor
#define IDMAC_DES0_CH	(1 << 4)		// Second Address Chained
#define IDMAC_DES0_ER	(1 << 5)		// End of Ring
#define IDMAC_DES0_CES	(1 << 30)		// Card Error Summary
#define IDMAC_DES0_OWN	(1 << 31)		// IDMAC Descriptor Owned

	uint32_t		des1;				// Buffer Sizes 1 & 2

	uint32_t		des2;				// Buffer Address Pointer 1

	uint32_t		des3;				// Buffer Address Pointer 2
} dw_idmac_desc_t;

typedef	struct _dw_hc_msh {
	void			*bshdl;

#define MF_SEND_INIT_STREAM			0x01
#define MF_XFER_DMA					0x02
	uint32_t		flags;

	uint32_t		ncards;
	uint32_t		cardno;

	uint32_t		fifo_size;
	uint32_t		divratio;
	uint32_t		dshft;

// This will change when we start using the new clk lib
#define DW_SCLK				266000000LL		// value from uboot
#define DW_SCLK_800MHZ		800000000LL
#define DW_SCLK_640MHZ		640000000LL
#define DW_CLK_MIN			400000
#define DW_CLK_MAX			50000000
#define DW_CLK_MAX_HS200	200000000
	uint64_t		sclk;

	uint32_t		sdr;				// CLKSEL values sdr/ddr
	uint32_t		ddr;

	uintptr_t		base;
	unsigned		pbase;

	uint32_t		xlen;

#define DW_DMA_DESC_MAX		256
	sdio_sge_t		sgl[DW_DMA_DESC_MAX];

#define IDMA_MAX_XFER 		0x1000				// 4k max
	dw_idmac_desc_t	*idma;
	uint32_t		idmap;

		// PIO data ins/outs fcns (depends on data width)
	void			*(*dins)( void *__buff, unsigned __len, _Uintptrt __addr );
	void			*(*douts)( const void *__buff, unsigned __len, _Uintptrt __addr );
} dw_hc_msh_t;

static __inline__ _Uint64t __attribute__((__unused__))
dw_in64(_Uintptrt __addr)
{
	_Uint64t        __data;

	__data = *(volatile _Uint64t *)__addr;
	return(__data);
}

static __inline__ void __attribute__((__unused__))
dw_out64(_Uintptrt __addr, _Uint64t __data)
{
	*(volatile _Uint64t *)__addr = __data;
}

static __inline__ void * __attribute__((__unused__))
dw_out64s(const void *__buff, unsigned __len, _Uintptrt __addr)
{
	_Uint64t        *__p = (_Uint64t *)__buff;

	while(__len > 0) {
		*(volatile _Uint64t *)__addr = *__p++;
		--__len;
	}
	return(__p);
}

static __inline__ void * __attribute__((__unused__))
dw_in64s(void *__buff, unsigned __len, _Uintptrt __addr)
{
	_Uint64t	*__p = (_Uint64t *)__buff;

	while(__len > 0) {
		*__p++ = *(volatile _Uint64t *)__addr;
		--__len;
	}
	return(__p);
}

extern int dw_init( sdio_hc_t *hc );
extern int dw_dinit( sdio_hc_t *hc );

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/sdiodi/hc/dwmshc.h $ $Rev: 756962 $")
#endif

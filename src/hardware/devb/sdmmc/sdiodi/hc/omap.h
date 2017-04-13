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

#ifndef	_OMAP_H_INCLUDED
#define	_OMAP_H_INCLUDED

#include <internal.h>

//#define OMAP_DEBUG

// MMCHS base address, irq
#define MMCHS1_BASE					0x4809c000
#define MMCHS2_BASE					0x480b4000
#define MMCHS3_BASE					0x480ad000
#define MMCHS4_BASE					0x480d1000
#define MMCHS5_BASE					0x480d5000

#define MMCHS_CLK_MIN				400000
#define MMCHS_CLK_MAX( _did )		( ( (_did) >= OMAP_DID_54XX ? MMCHS_CLK_REF_192MHZ : MMCHS_CLK_REF_96MHZ ) )
#define MMCHS_CLK_REF( _did )		( ( (_did) >= OMAP_DID_54XX ? MMCHS_CLK_REF_192MHZ : MMCHS_CLK_REF_96MHZ ) )
#define MMCHS_CLK_REF_64MHZ			64000000
#define MMCHS_CLK_REF_96MHZ			96000000
#define MMCHS_CLK_REF_192MHZ		192000000

#define	MMCHS_SIZE					512

#define	MMCHS_FIFO_SIZE				512

#define MMCHS_PWR_SWITCH_TIMEOUT	5000	// 5ms

#define MMCHS_TUNING_RETRIES		40
#define MMCHS_TUNING_TIMEOUT		150

#define MMCHS_SET_LDO_RETRIES		5

// Register Descriptions
#define	MMCHS_HL_REV					0x000	// (OMAP 4 only)
#define	MMCHS_HL_HWINFO					0x004	// (OMAP 4 only)
	#define HWINFO_MADMA_EN				0x01
#define	MMCHS_HL_SYSCONFIG				0x010	// (OMAP 4 only)
	#define HL_SYSCONFIG_IDLEMODE_MSK	0x0c
	#define HL_SYSCONFIG_IDLEMODE_SMART	0x08

#define	MMCHS_SYSCONFIG	0x110	// System Configuration Register
	#define	SYSCONFIG_STANDBYMODE_MSK			(3 << 12)
	#define	SYSCONFIG_STANDBYMODE_SMART_WAKEUP	(3 << 12)
	#define	SYSCONFIG_STANDBYMODE_SMART			(2 << 12)
	#define	SYSCONFIG_STANDBYMODE_NOSTANBY		(1 << 12)

	#define	SYSCONFIG_CLOCKACTIVITY_MSK			(3 << 8)
	#define	SYSCONFIG_CLOCKACTIVITY_IFACE_FUNC	(3 << 8)
	#define	SYSCONFIG_CLOCKACTIVITY_FUNC		(2 << 8)
	#define	SYSCONFIG_CLOCKACTIVITY_IFACE		(1 << 8)

	#define	SYSCONFIG_SIDLEMODE_MSK				(3 << 3)
	#define	SYSCONFIG_SIDLEMODE_SMART_WAKEUP	(3 << 3)
	#define	SYSCONFIG_SIDLEMODE_SMART			(2 << 3)
	#define	SYSCONFIG_SIDLEMODE_IGNORE			(1 << 3)
	#define	SYSCONFIG_ENAWAKEUP					(1 << 2)	// Wakeup feature enable
	#define	SYSCONFIG_SOFTRESET					(1 << 1)	// Software reset
	#define	SYSCONFIG_AUTOIDLE					(1 << 0)	// Clock auto idle

	#define SYSCONFIG_CONFIG				(	SYSCONFIG_AUTOIDLE |				\
												SYSCONFIG_ENAWAKEUP |				\
												SYSCONFIG_SIDLEMODE_SMART_WAKEUP |	\
												SYSCONFIG_CLOCKACTIVITY_FUNC |		\
												SYSCONFIG_STANDBYMODE_SMART_WAKEUP )

	#define SYSCONFIG_CONFIG_MSK			(	SYSCONFIG_AUTOIDLE |			\
												SYSCONFIG_ENAWAKEUP |			\
												SYSCONFIG_SIDLEMODE_MSK |		\
												SYSCONFIG_CLOCKACTIVITY_MSK |	\
												SYSCONFIG_STANDBYMODE_MSK )

#define	MMCHS_SYSSTATUS		0x114	// System Status Register
	#define	SYSSTATUS_RESETDONE	(1 << 0)	// Reset done

#define	MMCHS_CSRE			0x124	// Card status response error
#define	MMCHS_SYSTEST		0x128	// System Test Register
#define	MMCHS_CON			0x12C	// Configuration register
	#define	CON_OD				(1 <<  0)	// Card Open Drain Mode
	#define	CON_INIT			(1 <<  1)	// Send initialization stream
	#define	CON_DW8				(1 <<  5)	// MMC 8-bit width
	#define CON_DEBOUNCE_1MS	(2 <<  9)
	#define CON_DEBOUNCE_8MS	(3 <<  9)
	#define	CON_PADEN			(1 << 15)	// Control Power for MMC Lines
	#define	CON_CLKEXTFREE		(1 << 16)	// External clock free running
	#define CON_DDR				(1 << 19)	// Dual Data Rate mode (omap 4)
	#define CON_DMA_MNS			(1 << 20)	// DMA Master Select (omap 4)

#define	MMCHS_PWCNT			0x130	// Power counter register
#define	MMCHS_DLL			0x134	// DLL control and status register
	#define DLL_SLAVE_RATIO_SHIFT	6			// Position of SLAVE_RATIO in MMCHS_DLL register
	#define DLL_FORCE_SRC_SHIFT		13			// Position of FORCE_SR_C in MMCHS_DLL register
	#define DLL_FORCE_SRC_MAX		0x7f
	#define DLL_FORCE_SRC_MSK		(0x7f << 13)
	#define DLL_LOCK				(1 << 0)	// Master DLL lock status
	#define DLL_CALIB				(1 << 1)	// Enable slave DLL to update delay values
	#define DLL_UNLOCK_STICKY		(1 << 2)	// Asserted when single period  exceeds
	#define DLL_UNLOCK_CLEAR		(1 << 3)	// Clear unlock sticky flag
	#define DLL_FORCE_VALUE			(1 << 12)	// Put forced values to slave DLL
	#define DLL_SLAVE_RATIO_MSK		(0x3f << 6)	// Slave delay ratio mask

#define	MMCHS_SDMASA		0x200
#define	MMCHS_BLK			0x204	// Transfer Length Configuration register
#define	MMCHS_ARG			0x208	// Command argument Register
#define	MMCHS_CMD			0x20C	// Command and transfer mode register
	#define	CMD_DE				(1 <<  0)	// DMA Enable
	#define	CMD_BCE				(1 <<  1)	// Block Count Enable
	#define	CMD_ACMD12			(1 <<  2)	// Auto CMD12 Enable
	#define	CMD_ACMD23			(1 <<  3)	// Auto CMD23 Enable
	#define	CMD_DDIR			(1 <<  4)	// Data Transfer Direction Read
	#define	CMD_MBS				(1 <<  5)	// Multi Block Select
	#define	CMD_RSP_TYPE_136	(1 << 16)	// Response length 136 bit
	#define	CMD_RSP_TYPE_48		(2 << 16)	// Response length 48 bit
	#define	CMD_RSP_TYPE_48b	(3 << 16)	// Response length 48 bit with busy after response
	#define	CMD_CCCE			(1 << 19)	// Comamnd CRC Check Enable
	#define	CMD_CICE			(1 << 20)	// Comamnd Index Check Enable
	#define	CMD_DP				(1 << 21)	// Data Present
	#define	CMD_TYPE_CMD12		(3 << 22)	// CMD12 or CMD52 "I/O Abort"

#define	MMCHS_RSP10			0x210	// Command response[31:0] Register
#define	MMCHS_RSP32			0x214	// Command response[63:32] Register
#define	MMCHS_RSP54			0x218	// Command response[95:64] Register
#define	MMCHS_RSP76			0x21C	// Command response[127:96] Register
#define	MMCHS_DATA			0x220	// Data Register
#define	MMCHS_PSTATE		0x224	// Present state register
	#define	PSTATE_CMDI			(1 <<  0)	// Command inhibit (mmci_cmd).
	#define	PSTATE_DATI			(1 <<  1)	// Command inhibit (mmci_dat).
	#define	PSTATE_DLA			(1 <<  2)	// mmci_dat line active
	#define	PSTATE_WTA			(1 <<  8)	// Write Transfer Active
	#define	PSTATE_RTA			(1 <<  9)	// Read Transfer Active
	#define	PSTATE_BWE			(1 << 10)	// Buffer Write Ready
	#define	PSTATE_BRE			(1 << 11)	// Buffer Read Ready
	#define	PSTATE_CINS			(1 << 16)	// Card inserted, debounced value
	#define	PSTATE_CDPL			(1 << 18)	// Card Detect Pin Level
	#define	PSTATE_WP			(1 << 19)	// Write Protect
	#define PSTATE_DLEV_MSK		        (0xf << 20)
	#define PSTATE_CLEV			(1 << 24)

#define	MMCHS_HCTL			0x228	// Host Control register
	#define	HCTL_LED			(1 << 0)	// LED (not supported)
	#define	HCTL_DTW4			(1 << 1)	// Data transfer width 4 bit
	#define HCTL_HSE			(1 << 2)	// High Speed (omap 4)
	#define HCTL_DMAS_ADMA2		(2 << 3)	// DMA Select ADMA2 (omap 4)
	#define	HCTL_SDBP			(1 << 8)	// SD bus power
	#define	HCTL_SDVS1V8		(5 << 9)	// SD bus voltage 1.8V
	#define	HCTL_SDVS3V0		(6 << 9)	// SD bus voltage 3.0V
	#define	HCTL_SDVS3V3		(7 << 9)	// SD bus voltage 3.3V
	#define	HCTL_SDVS_MSK		(7 << 9)

#define	MMCHS_SYSCTL		0x22C	// SD system control register
	#define	SYSCTL_ICE			(1 << 0)	// Internal Clock Enable
	#define	SYSCTL_ICS			(1 << 1)	// Internal Clock Stable
	#define	SYSCTL_CEN			(1 << 2)	// Clock Enable
	#define	SYSCTL_CGS_PROG		(1 << 5)	// Clock Generator Select (programmable)
	#define	SYSCTL_DTO_MSK		(0xF << 16)	// Data timeout counter value and busy timeout
	#define	SYSCTL_DTO_MAX		(0xE << 16)	// Timeout = TCF x 2^27
	#define	SYSCTL_SRA			(1 << 24)	// Software reset for all
	#define	SYSCTL_SRC			(1 << 25)	// Software reset for mmci_cmd line
	#define	SYSCTL_SRD			(1 << 26)	// Software reset for mmci_dat line
	#define CLK_MAX_DIV			2046

#define	MMCHS_STAT			0x230	// Interrupt status register
#define	MMCHS_IE			0x234	// Interrupt SD enable register
#define	MMCHS_ISE			0x238	// Interrupt signal enable register
	#define	INTR_CC				(1 <<  0)	// Command Complete
	#define	INTR_TC				(1 <<  1)	// Transfer Complete
	#define	INTR_BGE			(1 <<  2)	// Block Gap Event
	#define INTR_DMA			(1 <<  3)	// DMA interupt
	#define	INTR_BWR			(1 <<  4)	// Buffer Write Ready interrupt
	#define	INTR_BRR			(1 <<  5)	// Buffer Read Ready interrupt
	#define INTR_CINS			(1 <<  6)	// Card Insertion
	#define INTR_CREM			(1 <<  7)	// Card Removal
	#define	INTR_CIRQ			(1 <<  8)	// Card interrupt
	#define	INTR_OBI			(1 <<  9)	// Out-Of-Band interrupt
	#define INTR_BSR			(1 << 10)	// Boot Status
	#define INTR_RETUNE			(1 << 12)	// Re-Tuning
	#define	INTR_ERRI			(1 << 15)	// Error interrupt
	#define	INTR_CTO			(1 << 16)	// Command Timeout error
	#define	INTR_CCRC			(1 << 17)	// Command CRC error
	#define	INTR_CEB			(1 << 18)	// Command End Bit error
	#define	INTR_CIE			(1 << 19)	// Command Index error
	#define	INTR_DTO			(1 << 20)	// Data Timeout error
	#define	INTR_DCRC			(1 << 21)	// Data CRC error
	#define	INTR_DEB			(1 << 22)	// Data End Bit error
	#define	INTR_ACE			(1 << 24)	// ACMD12 error
	#define INTR_ADMAE			(1 << 25)	// ADMA Error
	#define	INTR_CERR			(1 << 28)	// Card error
	#define	INTR_BADA			(1 << 29)	// Bad Access to Data Space
	#define INTR_DFLTS			0x110f8000
	#define INTR_ALL			0x33ff87ff // 0x117F8033
	#define STAT_CLR_MSK		0x117f8033

#define	MMCHS_AC12				0x23C	// Auto CMD12 Error Status Register

#define MMCHS_HCTL2				0x23C
	#define HCTL2_PRESET_VAL		(1 << 31)
	#define HCTL2_ASYNC_INT			(1 << 30)
	#define HCTL2_TUNED_CLK			(1 << 23)
	#define HCTL2_EXEC_TUNING		(1 << 22)

	#define HCTL2_DRV_TYPE_MSK		(0x3 << 20)
	#define HCTL2_DRV_TYPE_D		(0x3 << 20)
	#define HCTL2_DRV_TYPE_C		(0x2 << 20)
	#define HCTL2_DRV_TYPE_A		(0x1 << 20)
	#define HCTL2_DRV_TYPE_B		(0x0 << 20)

	#define HCTL2_SIG_1_8V			(1 << 19)

	#define HCTL2_MODE( _c )		( (_c) & HCTL2_MODE_MSK )
	#define HCTL2_MODE_MSK			(0x7 << 16)
	#define HCTL2_MODE_HS200		(0x5 << 16)
	#define HCTL2_MODE_DDR50		(0x4 << 16)
	#define HCTL2_MODE_SDR104		(0x3 << 16)
	#define HCTL2_MODE_SDR50		(0x2 << 16)
	#define HCTL2_MODE_SDR25		(0x1 << 16)

#define	MMCHS_CAPA			0x240	// Capabilities register
	#define CAP_BIT64				(1 << 28)	// 64 bit System Bus Support
	#define	CAP_VS1V8				(1 << 26)	// 1.8V
	#define	CAP_VS3V0				(1 << 25)	// 3.0V
	#define	CAP_VS3V3				(1 << 24)	// 3.3V
	#define CAP_SRS					(1 << 23)	// Suspend/Resume Support
	#define CAP_DS					(1 << 22)	// DMA Support
	#define CAP_HS					(1 << 21)	// High Speed Support
	#define CAP_AD2S				(1 << 19)	// ADMA2 Support

#define	MMCHS_CAPA2				0x244	// bits 32 to 64
	#define CAP_CLK_MULT( _cap )	( ( ( _cap ) & 0xff0000 ) >> 16 )
	#define CAP_TUNING_MODE( _cap )	( ( ( _cap ) & 0xe000 ) >> 14 )
	#define CAP_TUNE_SDR50			(1 << 13)
	#define CAP_TIMER_COUNT( _cap )	( ( ( _cap ) & 0xf00 ) >> 8 )
		#define CAP_TIMER_OTHER		0x0f	// get info from other source

	#define CAP_DRIVE_TYPE_D		(1 << 6)
	#define CAP_DRIVE_TYPE_C		(1 << 5)
	#define CAP_DRIVE_TYPE_A		(1 << 4)
	#define CAP_DDR50				(1 << 2)
	#define CAP_SDR104				(1 << 1)
	#define CAP_SDR50				(1 << 0)

#define	MMCHS_MCCAP				0x248	// Maximum Current Capabilities
	#define MCCAP_330( _cap )		( (_cap) & 0x0000ff )
	#define MCCAP_300( _cap )		( ( (_cap) & 0x00ff00 ) >> 8 )
	#define MCCAP_180( _cap )		( ( (_cap) & 0xff0000 ) >> 16 )
	#define MCCAP_MULT				4	// 4mA steps

#define	MMCHS_FE				0x250	// Force Event (OMAP 4 only)
#define	MMCHS_ADMAES			0x254	// ADMA Error Status (OMAP 4 only)
#define	MMCHS_ADMASAL			0x258	// ADMA System Address (OMAP 4 only)

#define	MMCHS_REV				0x2FC	// Version Register
	#define REV_SREV(_r)			( ( (_r) >> 16 ) & 0xff)
	#define REV_SREV_V1				0x0
	#define REV_SREV_V2				0x1
	#define REV_SREV_V3				0x2


#define ADMA2_MAX_XFER	(1024 * 60)

// 32 bit ADMA descriptor definition
typedef struct _omap_adma32_t {
	uint16_t	attr;
	uint16_t	len;
	uint32_t	addr;
} omap_adma32_t;

#define ADMA2_VALID	(1 << 0)	// valid
#define ADMA2_END	(1 << 1)	// end of descriptor, transfer complete interrupt will be generated
#define ADMA2_INT	(1 << 2)	// generate DMA interrupt, will not be used
#define ADMA2_NOP	(0 << 4)	// no OP, go to the next desctiptor
#define ADMA2_TRAN	(2 << 4)	// transfer data
#define ADMA2_LINK	(3 << 4) 	// link to another descriptor

//	Basic data for the EDMA (version 3)
typedef	struct {

	//	Current state
	int state;	// Nonzero if there is a transfer active
	int active_channel;	// Currently active channel
	uintptr_t active_base; // Base pointer for the active channel
	uint32_t active_mask; // Bit mask for the active channel

	//	General configuration
	uintptr_t base;	// Pointer to the base of the EDMA register window
	int receive_channel; // Our receive channel number
	int transmit_channel; // Our transmit channel number

	//	PaRAM configuration.
	//
	//	Each EDMA channel has one dedicated	PaRAM set.
	//
	//	We need some more (max 127) for the scatter/gather implementation.
	//	The range of sets available to us is given below.
	//
	//	The same PaRAM sets are used by both receive and transmit,
	//	but obviously not at the same time.
	//
	int first_extra_param_index; // Index of our first extra PaRAM set
	int extra_param_count; // Number of extra PaRAM sets available

} edma3_data_t;

//	Possible states.
#define EDMA3_IDLE 			0
#define EDMA3_RECEIVING 	1
#define EDMA3_TRANSMITTING	2

typedef	struct _omap_mmchs {
	void			*bshdl;
#define OF_SEND_INIT_STREAM	0x01
#define OF_USE_EDMA			0x02
#define OF_USE_ADMA			0x04
#define OF_TUNE_SDR50		0x08
#define OF_SDMA_ACTIVE		0x10
#define OF_EDMA_ACTIVE		0x20

	uint32_t		flags;

	uint32_t		clk_ref;

	uint32_t		clk_mul;

#define TUNING_MODE_1	0x0
#define TUNING_MODE_2	0x1
#define TUNING_MODE_3	0x2
	uint32_t		tuning_mode;

	uintptr_t		mmc_base;
	unsigned		mmc_pbase;

	uintptr_t		ctrl_base;

	uintptr_t		clkctrl_reg;	/* MMCx_CLKCTRL register */
	uintptr_t		context_reg;	/* MMCx_CONTEXT register */

	uint32_t		cs;

#define DMA_DESC_MAX		256
	sdio_sge_t		sgl[DMA_DESC_MAX];

// adma specific
	omap_adma32_t	*adma;
	uint32_t		admap;

// sdma specific
	sdio_sge_t		*sgp;
	uint32_t		sgc;
	int				sdma_iid;	// SDMA interrupt id
	uintptr_t		sdma_irq;
	uintptr_t		sdma_base;
	uintptr_t		sdma_cbase;
	int				sdma_chnl;
	int				sdma_rreq;	// DMA request line for Rx
	int				sdma_treq;	// DMA request line for Tx
	uint32_t		sdma_csr;
	struct sigevent	sdma_ev;

//
//	EDMA Specific data
//
	edma3_data_t	edma3;
	volatile void	*so_ptr; //used to do bus sync
} omap_hc_mmchs_t;

// DMA4
#define OMAP_DMA4_SIZE			0x1000
#define OMAP_DMA4_BASE			0x4a056000
#define OMAP_BASE_IRQ			256
#define	OMAP_MMC1_TX			61
#define	OMAP_MMC1_RX			62
#define	OMAP_MMC2_TX			47
#define	OMAP_MMC2_RX			48
#define	OMAP_MMC3_TX			77
#define	OMAP_MMC3_RX			78
#define	OMAP_MMC4_TX			57
#define	OMAP_MMC4_RX			58
#define	OMAP_MMC5_TX			59
#define	OMAP_MMC5_RX			60

#define	DMA4_IRQSTATUS(j)		(0x08 + (j) * 4)	// j = 0 - 3
#define	DMA4_IRQENABLE(j)		(0x18 + (j) * 4)	// j = 0 - 3
#define	DMA4_SYSSTATUS			(0x28)
#define	DMA4_OCP_SYSCONFIG		(0x2C)
#define	DMA4_CAPS_0				(0x64)
#define	DMA4_CAPS_2				(0x6C)
#define	DMA4_CAPS_3				(0x70)
#define	DMA4_CAPS_4				(0x74)
#define	DMA4_GCR				(0x78)

#define	DMA4_CH_BASE(i)			((i) * 0x60)	// i = 0 - 31
#define	DMA4_CCR				0x80
#define	DMA4_CLNK_CTRL			0x84
#define	DMA4_CICR				0x88
#define	DMA4_CSR				0x8C
	#define	DMA4_CSR_ERROR		((1 << 11) | (1 << 10) | (1 << 8))
	#define DMA4_CSR_BLOCK		(1 << 5) 
#define	DMA4_CSDP				0x90
#define	DMA4_CEN				0x94
#define	DMA4_CFN				0x98
#define	DMA4_CSSA				0x9C
#define	DMA4_CDSA				0xA0
#define	DMA4_CSE				0xA4
#define	DMA4_CSF				0xA8
#define	DMA4_CDE				0xAC
#define	DMA4_CDF				0xB0
#define	DMA4_CSAC				0xB4
#define	DMA4_CDAC				0xB8
#define	DMA4_CCEN				0xBC
#define	DMA4_CCFN				0xC0
#define	DMA4_COLOR				0xC4
#define	DMA4_CCR_SYNCHRO_CONTROL(s)		(((s) & 0x1F) | (((s) >> 5) << 19))

#define CONTROL_PBIAS_BASE( _did )			( ( (_did) >= OMAP_DID_54XX ? CONTROL_PBIAS_OMAP5_BASE : CONTROL_PBIAS_OMAP4_BASE ) )
#define CONTROL_PBIAS_OMAP4_BASE			0x4a100600
#define CONTROL_PBIAS_OMAP5_BASE			0x4a002e00
#define CONTROL_PBIAS_SIZE					0x100

#define CONTROL_PBIASLITE				0x00
	#define PBIASLITE1_HIZ_MODE				( 0x1 << 31 )
	#define PBIASLITE1_SUPPLY_HI_OUT		( 0x1 << 30 )
	#define PBIASLITE1_VMODE_ERROR			( 0x1 << 29 )
	#define PBIASLITE1_PWRDNZ				( 0x1 << 28 )
	#define PBIASLITE1_VMODE_3V				( 0x1 << 27 )
	#define PBIASLITE1_VMODE_1_8V			( 0 )

	#define MMC1_PWRDNZ						( 0x1 << 26 )
	#define MMC1_PBIASLITE_HIZ_MODE			( 0x1 << 25 )
	#define MMC1_PBIASLITE_SUPPLY_HI_OUT	( 0x1 << 24 )
	#define MMC1_PBIASLITE_VMODE_ERROR		( 0x1 << 23 )
	#define MMC1_PBIASLITE_PWRDNZ			( 0x1 << 22 )
	#define MMC1_PBIASLITE_VMODE_3V			( 0x1 << 21 )
	#define MMC1_PBIASLITE_VMODE_1_8V		( 0 )
	#define USBC1_ICUSB_PWRDNZ				( 0x1 << 20 )

#define CONTROL_MMC1					0x28
	#define SDMMC1_PUSTRENGTH_GRP0_10K	( 0x1 << 31 )
	#define SDMMC1_PUSTRENGTH_GRP1_10K	( 0x1 << 30 )
	#define SDMMC1_PUSTRENGTH_GRP2_10K	( 0x1 << 29 )
	#define SDMMC1_PUSTRENGTH_GRP3_10K	( 0x1 << 29 )

	#define SDMMC1_PUSTRENGTH_GRP_10K	( SDMMC1_PUSTRENGTH_GRP0_10K |		\
											SDMMC1_PUSTRENGTH_GRP1_10K |	\
											SDMMC1_PUSTRENGTH_GRP2_10K |	\
											SDMMC1_PUSTRENGTH_GRP3_10K )

	#define SDMMC1_DR0_SPEEDCTRL_65M	( 0x1 << 27 )
	#define SDMMC1_DR1_SPEEDCTRL_65M	( 0x1 << 26 )
	#define SDMMC1_DR2_SPEEDCTRL_65M	( 0x1 << 25 )
	#define SDMMC1_DR_SPEEDCTRL_65M		( SDMMC1_DR0_SPEEDCTRL_65M | 		\
											SDMMC1_DR1_SPEEDCTRL_65M |		\
											SDMMC1_DR2_SPEEDCTRL_65M )

#define OMAP_DID_34XX							0x3430
#define OMAP_DID_44XX							0x4430
#define OMAP_DID_54XX							0x5430
#define OMAP_DID_54XX_ES2						0x5432
#define OMAP_DID_DRA7XX							0x7000
#define OMAP_DID_DRA7XX_ES2						0x7002

#define OMAP54XX_CONTROL_ID_CODE                0x4a002204
	#define OMAP54XX_CONTROL_ID_CODE_REV_MASK	0xf0000000
	#define OMAP54XX_CONTROL_ID_CODE_REV_SHIFT	28
	#define OMAP54XX_CONTROL_ID_CODE_REV_ES1_0	0x00
	#define OMAP54XX_CONTROL_ID_CODE_REV_ES2_0	0x01

#define DRA74X_CTRL_WKUP_ID_CODE                0x4AE0C204
	#define DRA7xx_CTRL_WKUP_ID_CODE_ES1_0		0x0B99002F
	#define DRA7xx_CTRL_WKUP_ID_CODE_ES1_1		0x1B99002F
	#define DRA7xx_CTRL_WKUP_ID_CODE_ES2_0		0x2B99002F

#define MMC_REGISTER_SIZE						4

/* CLKCTRL/CONTEXT registers for DRA7xx */
#define DRA7XX_MMC1_CLKCTRL						0x4A009328
#define DRA7XX_MMC2_CLKCTRL						0x4A009330
#define DRA7XX_MMC3_CLKCTRL						0x4A009820
#define DRA7XX_MMC4_CLKCTRL						0x4A009828

#define DRA7XX_MMC1_CONTEXT						0x4AE0732C
#define DRA7XX_MMC2_CONTEXT						0x4AE07334
#define DRA7XX_MMC3_CONTEXT						0x4AE07524
#define DRA7XX_MMC4_CONTEXT						0x4AE0752C

/* CLKCTRL/CONTEXT registers for OMAP5 ES2.0 */
#define OMAP5_ES2_MMC1_CLKCTRL					0x4A009628
#define OMAP5_ES2_MMC2_CLKCTRL					0x4A009630
#define OMAP5_ES2_MMC3_CLKCTRL					0x4A009120
#define OMAP5_ES2_MMC4_CLKCTRL					0x4A009128
#define OMAP5_ES2_MMC5_CLKCTRL					0x4A009160

#define OMAP5_ES2_MMC1_CONTEXT					0x4AE0762C
#define OMAP5_ES2_MMC2_CONTEXT					0x4AE07634
#define OMAP5_ES2_MMC3_CONTEXT					0x4AE07124
#define OMAP5_ES2_MMC4_CONTEXT					0x4AE0712C
#define OMAP5_ES2_MMC5_CONTEXT					0x4AE07164

/* CLKCTRL/CONTEXT registers for OMAP5 ES1.0 */
#define OMAP5_ES1_MMC1_CLKCTRL					0x4A009328
#define OMAP5_ES1_MMC2_CLKCTRL					0x4A009330
#define OMAP5_ES1_MMC3_CLKCTRL					0x4A009520
#define OMAP5_ES1_MMC4_CLKCTRL					0x4A009528
#define OMAP5_ES1_MMC5_CLKCTRL					0x4A009560

#define OMAP5_ES1_MMC1_CONTEXT					0x4AE0732C
#define OMAP5_ES1_MMC2_CONTEXT					0x4AE07334
#define OMAP5_ES1_MMC3_CONTEXT					0x4AE07524
#define OMAP5_ES1_MMC4_CONTEXT					0x4AE0752C
#define OMAP5_ES1_MMC5_CONTEXT					0x4AE07564

/* CLKCTRL/CONTEXT registers for OMAP4 */
#define OMAP4_MMC1_CLKCTRL						0x4A009328
#define OMAP4_MMC2_CLKCTRL						0x4A009330

#define OMAP4_MMC1_CONTEXT						0x4A30732C
#define OMAP4_MMC2_CONTEXT						0x4A307334

/* Bit definitions of CLKCTRL register */
#define CLKCTRL_CLKSEL_DIV_MSK					0x06000000
#define CLKCTRL_CLKSEL_MAX						0x01000000
#define CLKCTRL_STBYST							0x00040000
#define CLKCTRL_IDLEST_MSK						0x00030000
#define CLKCTRL_MODULEMODE_MSK					0x00000003
#define CLKCTRL_MODULEMODE_ENB					0x00000002

/* Bit definitions of CONTEXT register */
#define CONTEXT_LOSTCONTEXT_RFF					0x00000002

extern int omap_init( sdio_hc_t *hc );
extern int omap_dinit( sdio_hc_t *hc );

#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/sdiodi/hc/omap.h $ $Rev: 802182 $")
#endif

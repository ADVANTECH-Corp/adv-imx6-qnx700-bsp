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

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <hw/inout.h>
#include <sys/mman.h>
#include <sys/rsrcdbmgr.h>

#include <internal.h>

#ifdef SDIO_HC_OMAP

#ifdef SDIO_OMAP_BUS_SYNC
#include <arm/mmu.h>
#include <fcntl.h>
#endif

//#define OMAP_DEBUG

#include <omap.h>

static int omap_pwr( sdio_hc_t *hc, int vdd );
static int omap_drv_type( sdio_hc_t *hc, int type );
static int omap_bus_mode( sdio_hc_t *hc, int bus_mode );
static int omap_bus_width( sdio_hc_t *hc, int width );
static int omap_clk( sdio_hc_t *hc, int clk );
static int omap_timing( sdio_hc_t *hc, int timing );
static int omap_signal_voltage( sdio_hc_t *hc, int signal_voltage );
static int omap_cd( sdio_hc_t *hc );
static int omap_pm( sdio_hc_t *hc, int op );
static int omap_cmd( sdio_hc_t *hc, sdio_cmd_t *cmd );
static int omap_abort( sdio_hc_t *hc, sdio_cmd_t *cmd );
static int omap_event( sdio_hc_t *hc, sdio_event_t *ev );
static int omap_tune( sdio_hc_t *hc, int op );
static int omap_preset( sdio_hc_t *hc, int enable );
static int omap_set_dll( sdio_hc_t *hc, int delay );

static sdio_hc_entry_t omap_hc_entry =	{ 16,
										omap_dinit, omap_pm,
										omap_cmd, omap_abort,
										omap_event, omap_cd, omap_pwr,
										omap_clk, omap_bus_mode,
										omap_bus_width, omap_timing,
										omap_signal_voltage, omap_drv_type,
										NULL, omap_tune, omap_preset,
										};

//	EDMA is "Enhanced Direct Memory Access"

//	EDMA initialization
static int edma3_init(const sdio_hc_t *hc);

//	EDMA termination
static int edma3_dinit(edma3_data_t* edma);

//	Setup a scatter receive (transfer from a device I/O port to memory buffers),
//	or a gather transmit (transfer from memory buffers to a device I/O port).
//	Works fine also with just one buffer, so there's no need to have
//	separate code for single buffer I/O.
//
//	Returns 0 if no errors were detected, nonzero otherwise.
//
//	Currently the only possible error is EINVAL - Invalid argument.
//
static int edma3_setup_transfer(
	edma3_data_t* edma,
	unsigned port,	// The data I/O port
	int receiving,	// Nonzero if receiving, zero if transmitting
	unsigned block_size,	// Block size to be used
	const sdio_sge_t* sgp,	// Pointer to the scatter gather fragment descriptors
	unsigned fragment_count	// Number of scatter gather fragments
);

//	Verify that a transfer has really completed.
static void edma3_verify_completion(edma3_data_t* edma);

//	Cleanup at the end of a completed transfer
static void edma3_transfer_done(edma3_data_t* edma);

//	Cleanup at the end of a failed or aborted transfer
static void edma3_transfer_failed(edma3_data_t* edma);

// this is the registers database, per device and per block
typedef struct {
	uint32_t	physbase;		// MC module physical base address
	uint32_t	clkctrl_phys;	// PRCM clkctrl physical address
	uint32_t	context_phys;	// PRCM context register physical address
} pm_info_t;

// Definition of OMAP4 PM info
pm_info_t		omap4_info[] = {
	{ .physbase = MMCHS1_BASE, .clkctrl_phys = OMAP4_MMC1_CLKCTRL, .context_phys = OMAP4_MMC1_CONTEXT },
	{ .physbase = MMCHS2_BASE, .clkctrl_phys = OMAP4_MMC2_CLKCTRL, .context_phys = OMAP4_MMC2_CONTEXT },
	{ .physbase = -1, .clkctrl_phys = -1, .context_phys = -1 },
};

// Definition of OMAP5 ES1 PM info
pm_info_t		omap5_es1_info[] = {
	{ .physbase = MMCHS1_BASE, .clkctrl_phys = OMAP5_ES1_MMC1_CLKCTRL, .context_phys = OMAP5_ES1_MMC1_CONTEXT },
	{ .physbase = MMCHS2_BASE, .clkctrl_phys = OMAP5_ES1_MMC2_CLKCTRL, .context_phys = OMAP5_ES1_MMC2_CONTEXT },
	{ .physbase = MMCHS3_BASE, .clkctrl_phys = OMAP5_ES1_MMC3_CLKCTRL, .context_phys = OMAP5_ES1_MMC3_CONTEXT },
	{ .physbase = MMCHS4_BASE, .clkctrl_phys = OMAP5_ES1_MMC4_CLKCTRL, .context_phys = OMAP5_ES1_MMC4_CONTEXT },
	{ .physbase = MMCHS5_BASE, .clkctrl_phys = OMAP5_ES1_MMC5_CLKCTRL, .context_phys = OMAP5_ES1_MMC5_CONTEXT },
	{ .physbase = -1, .clkctrl_phys = -1, .context_phys = -1 },
};

// Definition of OMAP5 ES2 PM info
pm_info_t		omap5_es2_info[] = {
	{ .physbase = MMCHS1_BASE, .clkctrl_phys = OMAP5_ES2_MMC1_CLKCTRL, .context_phys = OMAP5_ES2_MMC1_CONTEXT },
	{ .physbase = MMCHS2_BASE, .clkctrl_phys = OMAP5_ES2_MMC2_CLKCTRL, .context_phys = OMAP5_ES2_MMC2_CONTEXT },
	{ .physbase = MMCHS3_BASE, .clkctrl_phys = OMAP5_ES2_MMC3_CLKCTRL, .context_phys = OMAP5_ES2_MMC3_CONTEXT },
	{ .physbase = MMCHS4_BASE, .clkctrl_phys = OMAP5_ES2_MMC4_CLKCTRL, .context_phys = OMAP5_ES2_MMC4_CONTEXT },
	{ .physbase = MMCHS5_BASE, .clkctrl_phys = OMAP5_ES2_MMC5_CLKCTRL, .context_phys = OMAP5_ES2_MMC5_CONTEXT },
	{ .physbase = -1, .clkctrl_phys = -1, .context_phys = -1 },
};

pm_info_t		dra7xx_info[] = {
	{ .physbase = MMCHS1_BASE, .clkctrl_phys = DRA7XX_MMC1_CLKCTRL, .context_phys = DRA7XX_MMC1_CONTEXT },
	{ .physbase = MMCHS2_BASE, .clkctrl_phys = DRA7XX_MMC2_CLKCTRL, .context_phys = DRA7XX_MMC2_CONTEXT },
	{ .physbase = MMCHS3_BASE, .clkctrl_phys = DRA7XX_MMC3_CLKCTRL, .context_phys = DRA7XX_MMC3_CONTEXT },
	{ .physbase = MMCHS4_BASE, .clkctrl_phys = DRA7XX_MMC4_CLKCTRL, .context_phys = DRA7XX_MMC4_CONTEXT },
	{ .physbase = -1, .clkctrl_phys = -1, .context_phys = -1 },
};

#ifdef OMAP_DEBUG
static int omap_reg_dump( sdio_hc_t *hc, const char *func, int line )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: line %d", func, line );
	if( hc->cfg.did == OMAP_DID_34XX ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: SYSSTATUS %x, REV %x, CAPA %x, CAPA2 %x, MCCAP %x, SYSTEST %x, FE %x",
			__FUNCTION__,
			in32( base + MMCHS_SYSSTATUS ),
			in32( base + MMCHS_REV ),
			in32( base + MMCHS_CAPA ),
			in32( base + MMCHS_CAPA2 ),
			in32( base + MMCHS_MCCAP ),
			in32( base + MMCHS_SYSTEST ),
			in32( base + MMCHS_FE ) );
	}
	else {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: HL_REV %x, HL_HWINFO %x, HL_SYSCONFIG %x, SYSSTATUS %x, REV %x, CAPA %x, CAPA2 %x, MCCAP %x, SYSTEST %x, FE %x",
			__FUNCTION__,
			in32( base + MMCHS_HL_REV ),
			in32( base + MMCHS_HL_HWINFO ),
			in32( base + MMCHS_HL_SYSCONFIG ),
			in32( base + MMCHS_SYSSTATUS ),
			in32( base + MMCHS_REV ),
			in32( base + MMCHS_CAPA ),
			in32( base + MMCHS_CAPA2 ),
			in32( base + MMCHS_MCCAP ),
			in32( base + MMCHS_SYSTEST ),
			in32( base + MMCHS_FE ) );
	}

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: CSRE %x, CON %x, PWCNT %x, BLK %x, ARG %x, CMD %x, RSP10 %x, RSP32 %x, RSP54 %x,  RSP76 %x",
		__FUNCTION__,
		in32( base + MMCHS_CSRE ),
		in32( base + MMCHS_CON ),
		in32( base + MMCHS_PWCNT ),
		in32( base + MMCHS_BLK ),
		in32( base + MMCHS_ARG ),
		in32( base + MMCHS_CMD ),
		in32( base + MMCHS_RSP10 ),
		in32( base + MMCHS_RSP32 ),
		in32( base + MMCHS_RSP54 ),
		in32( base + MMCHS_RSP76 ) );

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: PSTATE %x, HCTL %x, SYSCTL %x, STAT %x, IE %x, ISE %x, AC12 %x, ADMAES %x ADMASAL %x",
		__FUNCTION__,
		in32( base + MMCHS_PSTATE ),
		in32( base + MMCHS_HCTL ),
		in32( base + MMCHS_SYSCTL ),
		in32( base + MMCHS_STAT ),
		in32( base + MMCHS_IE ),
		in32( base + MMCHS_ISE ),
		in32( base + MMCHS_AC12 ),
		in32( base + MMCHS_ADMAES ),
		in32( base + MMCHS_ADMASAL ) );
	return( EOK );
}
#endif

#ifdef SDIO_OMAP_BUS_SYNC
int omap_so_init( sdio_hc_t *hc )
{
	int 			fd;
	int 			r = EOK;
	char 			name[128];
	omap_hc_mmchs_t	*mmchs = (omap_hc_mmchs_t *)hc->cs_hdl;

	mmchs->so_ptr = NULL;
	snprintf(name, sizeof(name), "/SDMMC_SOMEMBLK:%d:%x", getpid(), mmchs->mmc_pbase);

	// Create a region of strongly ordered memory for a
	// chip errata workaround
	shm_unlink(name);
	fd = shm_open(name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
	if (fd == -1) {
		r = errno;
	} 
	else if (shm_ctl_special(fd, SHMCTL_ANON, 0, 0x1000, ARM_PTE_V6_S | ARM_PTE_V6_SP_XN) == -1) {
		r = errno;
	} 
	else if ((mmchs->so_ptr = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		r = errno;
	}

	close(fd);
	shm_unlink(name);

	return( r );
}

int omap_so_dinit( sdio_hc_t *hc )
{
	omap_hc_mmchs_t		*mmchs = (omap_hc_mmchs_t *)hc->cs_hdl;

	if(mmchs->so_ptr){
		munmap((void *)mmchs->so_ptr, 0x1000);
	}
	return (EOK);
}

void omap_bus_sync( sdio_hc_t *hc )
{
	omap_hc_mmchs_t		*mmchs = (omap_hc_mmchs_t *)hc->cs_hdl;
	/*
	* As recommended by TI, in order to push the stale data out
	* we need to have strongly ordered access between dsb and isb 
	*/
	asm volatile("dsb":::"memory");
	*(volatile unsigned *)(mmchs->so_ptr) = *(volatile unsigned *)(mmchs->so_ptr); 
	asm volatile("isb":::"memory");
}
#endif

static int omap_waitmask( sdio_hc_t *hc, uint32_t reg, uint32_t mask, uint32_t val, uint32_t usec )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;
	uint32_t			cnt;
	int					stat;
	int					rval;
	uint32_t			iter;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;
	stat	= ETIMEDOUT;
	rval	= 0;

		// fast poll for 1ms - 1us intervals
	for( cnt = min( usec, 1000 ); cnt; cnt-- ) {
		if( ( ( rval = in32( base + reg ) ) & mask ) == val ) {
			stat = EOK;
			break;
		}
		nanospin_ns( 1000L );
	}

	if( usec > 1000 && stat ) {
		iter	= usec / 1000L;
		for( cnt = iter; cnt; cnt-- ) {
			if( ( ( rval = in32( base + reg ) ) & mask ) == val ) {
				stat = EOK;
				break;
			}
			delay( 1 );
		}
	}

#ifdef OMAP_DEBUG
	if( !cnt ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: reg %x, mask %x, val %x, rval %x", __FUNCTION__, reg, mask, val, in32( base + reg ) );
	}
#endif

	return( stat );
}

const struct sigevent *omap_sdma_intr( void *hdl, int id )
{
	sdio_hc_t			*hc;
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			cbase;

	hc				= hdl;
	mmchs			= (omap_hc_mmchs_t *)hc->cs_hdl;
	cbase			= mmchs->sdma_cbase;

	mmchs->sdma_csr = in32( cbase + DMA4_CSR );

	out32( cbase + DMA4_CICR, 0 );
	out32( cbase + DMA4_CCR, 0 );			// Disable this DMA event
	out32( cbase + DMA4_CSR, 0x5DFE );		// Clear all status bits

	out32( mmchs->sdma_base + DMA4_IRQSTATUS( 0 ), 1 << mmchs->sdma_chnl );

	return( &mmchs->sdma_ev );
}

static int omap_sdma_xfer( sdio_hc_t *hc, uint32_t flags, uint32_t blksz, sdio_sge_t *sgp )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			cbase;

	mmchs			= (omap_hc_mmchs_t *)hc->cs_hdl;
	cbase			= mmchs->sdma_cbase;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  %s, blksz %d, addr 0x%x len %d", __FUNCTION__, ( flags & SCF_DIR_IN ) ? "in" : "out", blksz, sgp->sg_address, sgp->sg_count );
#endif

	mmchs->flags	|= OF_SDMA_ACTIVE;

	out32( cbase + DMA4_CSR, 0x1FFE );		// Clear all status bits
	out32( cbase + DMA4_CEN, sgp->sg_count >> 2 );
	out32( cbase + DMA4_CFN, 1 );			// Number of frames
	out32( cbase + DMA4_CSE, 1 );
	out32( cbase + DMA4_CDE, 1 );
	out32( cbase + DMA4_CICR, DMA4_CSR_ERROR | DMA4_CSR_BLOCK );

	if( ( flags & SCF_DIR_IN ) ) {		// setup receive SDMA channel
		out32( cbase + DMA4_CSDP, (2 <<  0)		// DATA_TYPE = 0x2:  32 bit element
				| (0 <<  2)		// RD_ADD_TRSLT = 0: Not used
				| (0 <<  6)		// SRC_PACKED = 0x0: Cannot pack source data
				| (0 <<  7)		// SRC_BURST_EN = 0x0: Cannot burst source
				| (0 <<  9)		// WR_ADD_TRSLT = 0: Undefined
				| (0 << 13)		// DST_PACKED = 0x0: No packing
				| (3 << 14)		// DST_BURST_EN = 0x3: Burst at 16x32 bits
				| (1 << 16)		// WRITE_MODE = 0x1: Write posted
				| (0 << 18)		// DST_ENDIAN_LOCK = 0x0: Endianness adapt
				| (0 << 19)		// DST_ENDIAN = 0x0: Little Endian type at destination
				| (0 << 20)		// SRC_ENDIAN_LOCK = 0x0: Endianness adapt
				| (0 << 21) );	// SRC_ENDIAN = 0x0: Little endian type at source

		out32( cbase + DMA4_CCR, DMA4_CCR_SYNCHRO_CONTROL( mmchs->sdma_rreq )	// Synchro control bits
				| (1 <<  5)		// FS = 1: Packet mode with BS = 0x1
				| (0 <<  6)		// READ_PRIORITY = 0x0: Low priority on read side
				| (0 <<  7)		// ENABLE = 0x0: The logical channel is disabled.
				| (0 <<  8)		// DMA4_CCRi[8] SUSPEND_SENSITIVE = 0
				| (0 << 12)		// DMA4_CCRi[13:12] SRC_AMODE = 0x0: Constant address mode
				| (1 << 14)		// DMA4_CCRi[15:14] DST_AMODE = 0x1: Post-incremented address mode
				| (1 << 18)		// DMA4_CCRi[18] BS = 0x1: Packet mode with FS = 0x1
				| (1 << 24)		// DMA4_CCRi[24] SEL_SRC_DST_SYNC = 0x1: Transfer is triggered by the source. The packet element number is specified in the DMA4_CSFI register.
				| (0 << 25) );	// DMA4_CCRi[25] BUFFERING_DISABLE = 0x0

		out32( cbase + DMA4_CSF, blksz >> 2 );
		out32( cbase + DMA4_CDSA, sgp->sg_address );
		out32( cbase + DMA4_CSSA, mmchs->mmc_pbase + MMCHS_DATA );
	}
	else {									// setup transmit SDMA channel
		out32( cbase + DMA4_CSDP, (2 <<  0)		// DATA_TYPE = 0x2:  32 bit element
				| (0 <<  2)		// RD_ADD_TRSLT = 0: Not used
				| (0 <<  6)		// SRC_PACKED = 0x0: Cannot pack source data
				| (3 <<  7)		// SRC_BURST_EN = 0x3: Burst at 16x32 bits
				| (0 <<  9)		// WR_ADD_TRSLT = 0: Undefined
				| (0 << 13)		// DST_PACKED = 0x0: No packing
				| (0 << 14)		// DST_BURST_EN = 0x0: Cannot Burst
				| (0 << 16)		// WRITE_MODE = 0x0: Write not posted
				| (0 << 18)		// DST_ENDIAN_LOCK = 0x0: Endianness adapt
				| (0 << 19)		// DST_ENDIAN = 0x0: Little Endian type at destination
				| (0 << 20)		// SRC_ENDIAN_LOCK = 0x0: Endianness adapt
				| (0 << 21) );	// SRC_ENDIAN = 0x0: Little endian type at source

		out32( cbase + DMA4_CCR, DMA4_CCR_SYNCHRO_CONTROL( mmchs->sdma_treq )
				| (1 <<  5)		// FS = 1: Packet mode with BS = 0x1
				| (0 <<  6)		// READ_PRIORITY = 0x0: Low priority on read side
				| (0 <<  7)		// ENABLE = 0x0: The logical channel is disabled.
				| (0 <<  8)		// DMA4_CCRi[8] SUSPEND_SENSITIVE = 0
				| (1 << 12)		// DMA4_CCRi[13:12] SRC_AMODE = 0x1: Post-incremented address mode
				| (0 << 14)		// DMA4_CCRi[15:14] DST_AMODE = 0x0: Constant address mode
				| (1 << 18)		// DMA4_CCRi[18] BS = 0x1: Packet mode with FS = 0x1
				| (0 << 24)		// DMA4_CCRi[24] SEL_SRC_DST_SYNC = 0x0: Transfer is triggered by the source. The packet element number is specified in the DMA4_CSFI register.
				| (0 << 25) );	// DMA4_CCRi[25] BUFFERING_DISABLE = 0x0

		out32( cbase + DMA4_CDF, blksz >> 2 );
		out32( cbase + DMA4_CSSA, sgp->sg_address );
		out32( cbase + DMA4_CDSA, mmchs->mmc_pbase + MMCHS_DATA );
	}

	mmchs->sgc--;
	mmchs->sgp++;

		// Enable DMA event
	out32( cbase + DMA4_CCR, in32( cbase + DMA4_CCR ) | 1 << 7 );

	return( EOK );
}


static int omap_edma_xfer(omap_hc_mmchs_t *mmchs, uint32_t flags, uint32_t blksz, sdio_sge_t *sgp )
{
	edma3_data_t* edma = &mmchs->edma3;

	//	The data port
	unsigned port = mmchs->mmc_pbase + MMCHS_DATA;

	//	Direction of the transfer
	int receiving = ((flags & SCF_DIR_IN) != 0);

	int status = edma3_setup_transfer(edma, port, receiving, blksz, mmchs->sgp, mmchs->sgc);
	if (status == EOK)
	{
		mmchs->flags |= OF_EDMA_ACTIVE;
	}

	return status;
}


static int omap_sdma_stop( sdio_hc_t *hc )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			cbase;

	mmchs			= (omap_hc_mmchs_t *)hc->cs_hdl;
	cbase			= mmchs->sdma_cbase;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: ", __FUNCTION__ );
#endif

	mmchs->flags	&= ~OF_SDMA_ACTIVE;

	out32( cbase + DMA4_CICR, 0 );
	out32( cbase + DMA4_CCR, 0 );			// Disable this DMA event
	out32( cbase + DMA4_CSR, 0x5DFE );		// Clear all status bits

	return( EOK );
}


static int omap_edma_stop( sdio_hc_t *hc )
{
	omap_hc_mmchs_t* mmchs = (omap_hc_mmchs_t *)hc->cs_hdl;
	mmchs->flags &= ~OF_EDMA_ACTIVE;
	edma3_transfer_failed(&mmchs->edma3);
	return EOK;
}

/*
 * J6 errata i832: DLL SW Reset Bit Does Not Reset to 0 After Execution
 * When autoidle is enabled, clock gets cut off and the reset completion signal
 * would not be recorded by the processor. Hence, though the reset executed,
 * the flag will remain asserted and another soft reset will be ignored
 *
 * Guidelines: Disable autoidle before DLL reset and re-enable autoidle after the reset.
 */
static int omap_mmchs_softreset( sdio_hc_t *hc )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;
	int					status;
	int					sysconfig;

	mmchs = (omap_hc_mmchs_t *)hc->cs_hdl;
	base = mmchs->mmc_base;
	sysconfig = in32( base + MMCHS_SYSCONFIG );

	if( sysconfig & SYSCONFIG_AUTOIDLE )
		out32( base + MMCHS_SYSCONFIG, sysconfig & ~SYSCONFIG_AUTOIDLE );

	out32( base + MMCHS_SYSCONFIG, in32( base + MMCHS_SYSCONFIG ) | SYSCONFIG_SOFTRESET );
	status = omap_waitmask( hc, MMCHS_SYSSTATUS, SYSSTATUS_RESETDONE, SYSSTATUS_RESETDONE, 10000 );

	if( sysconfig & SYSCONFIG_AUTOIDLE )
		out32( base + MMCHS_SYSCONFIG, sysconfig );

	return status;
}

static int omap_restore_context( sdio_hc_t *hc )
{
	omap_hc_mmchs_t		*mmchs;
	uint32_t			con;
	uint32_t			capa;
	uint32_t			hctl;
	uintptr_t			base;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;
	hctl	= 0;
	con		= CON_DEBOUNCE_8MS;

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 1, "%s (path %d): ", __FUNCTION__, hc->path );

	if( hc->vdd ) {
		out32( base + MMCHS_IE, 0 );		// disable interrupts
		out32( base + MMCHS_ISE, 0 );

		if( omap_waitmask( hc, MMCHS_SYSSTATUS, SYSSTATUS_RESETDONE, SYSSTATUS_RESETDONE, 10000 ) ) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  RESETDONE 1 timeout", __FUNCTION__ );
		}

		if( omap_mmchs_softreset( hc ) ) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  RESETDONE timeout", __FUNCTION__ );
		}

		if( ( mmchs->flags & OF_USE_ADMA ) ) {
			con = CON_DMA_MNS;
			hctl |= HCTL_DMAS_ADMA2;
		}

		out32( base + MMCHS_CON, con );
		capa = in32( base + MMCHS_CAPA );
		if( ( hc->ocr & ( OCR_VDD_30_31 | OCR_VDD_29_30 ) ) ) {
			capa |= CAP_VS3V0;
		}

		if( ( hc->ocr & OCR_VDD_17_195 ) ) {
			capa |= CAP_VS1V8;
		}

		hctl |= ( ( hc->vdd <= OCR_VDD_17_195 ) ? HCTL_SDVS1V8 : HCTL_SDVS3V0 );
		out32( base + MMCHS_HCTL, hctl );
		out32( base + MMCHS_CAPA, capa );
		out32( base + MMCHS_HCTL, hctl | HCTL_SDBP );
		if( omap_waitmask( hc, MMCHS_HCTL, HCTL_SDBP, HCTL_SDBP, 10000 ) ) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  SDBP timeout", __FUNCTION__ );
		}

		out32( base + MMCHS_ISE, INTR_ALL );			// enable intrs

		omap_bus_mode( hc, hc->bus_mode );
		omap_bus_width( hc, hc->bus_width );
		omap_drv_type( hc, hc->drv_type );
		omap_timing( hc, hc->timing );
		omap_clk( hc, hc->clk ? hc->clk : hc->clk_min );
	}
	return( EOK );
}

static int omap_set_ldo( sdio_hc_t *hc, int ldo, uint32_t voltage )
{
	omap_hc_mmchs_t		*mmchs;
	int					cnt;
	uintptr_t			base;
	int					pbias;
	int					retry;
	int					status;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  ldo %s, voltage %d", __FUNCTION__, ( ldo == SDIO_LDO_VCC ) ? "VCC" : "VCC_IO", voltage );
#endif

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->ctrl_base;

	if( !voltage || ldo == SDIO_LDO_VCC ) {
		return( bs_set_ldo( hc, ldo, voltage ) );
	}

	if( base == 0 ) {
		return( EOK );
	}

	for( retry = 0; retry < MMCHS_SET_LDO_RETRIES; retry++ ) {
		pbias	= in32( base + CONTROL_PBIASLITE );
		pbias	&= ~( MMC1_PBIASLITE_PWRDNZ | MMC1_PWRDNZ | MMC1_PBIASLITE_VMODE_3V );
		out32( base + CONTROL_PBIASLITE, pbias );

		if( ( status = bs_set_ldo( hc, ldo, voltage ) ) != EOK ) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: bs_set_ldo (status %d), retry %d", __FUNCTION__, status, retry );
			continue;
		}

		pbias	= in32( base + CONTROL_PBIASLITE );
		pbias	|= MMC1_PBIASLITE_PWRDNZ | MMC1_PWRDNZ;

		if( voltage == 3000 ) {
			pbias |= MMC1_PBIASLITE_VMODE_3V;
		}

		out32( base + CONTROL_PBIASLITE, pbias );

		delay( 1 );							// wait 4us for comparator

		for( cnt = 500; cnt; cnt-- ) {		// wait up to 5ms for error to clear
			if( !( ( pbias = in32( base + CONTROL_PBIASLITE ) ) & MMC1_PBIASLITE_VMODE_ERROR ) ) {
				break;
			}
			nanospin_ns( 10000L );
		}

		if( voltage && ( pbias & MMC1_PBIASLITE_VMODE_ERROR ) ) {	// power down MMC IO on error
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: PBIAS Voltage != LDO (voltage %d, pbias 0x%x) retry %d", __FUNCTION__, voltage, pbias, retry );
			bs_set_ldo( hc, ldo, 0 );
			pbias &= ~MMC1_PBIASLITE_PWRDNZ;
			out32( base + CONTROL_PBIASLITE, pbias );
			status = EIO;
			continue;
		}
		break;
	}
	return( status );
}

static int omap_pm( sdio_hc_t *hc, int pm )
{
	omap_hc_mmchs_t		*mmchs;
	int					wait;
	uint32_t			cval;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:   pm %d", __FUNCTION__, pm );
#endif

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;

	if( mmchs->clkctrl_reg == 0 ) {
		return( EOK );
	}

	switch( pm ) {
		case PM_IDLE:
#ifdef BS_PM_IDLE
				// force idle
			out32( mmchs->mmc_base + MMCHS_SYSCONFIG,
				in32( mmchs->mmc_base + MMCHS_SYSCONFIG ) & ~SYSCONFIG_CONFIG_MSK );

			out32( mmchs->clkctrl_reg, in32( mmchs->clkctrl_reg ) & ~CLKCTRL_MODULEMODE_MSK );
#endif
			break;

		case PM_ACTIVE:
			if( hc->pm_state == PM_SLEEP ) {
				omap_set_ldo( hc, SDIO_LDO_VCC, ( hc->vdd <= OCR_VDD_17_195 ) ? 1800 : 3000 );
			}

			out32( mmchs->clkctrl_reg, in32( mmchs->clkctrl_reg ) | CLKCTRL_MODULEMODE_ENB );
			for( wait = 10000; wait; wait-- ) {		// wait up to 10ms
				cval = in32( mmchs->clkctrl_reg );
				if( ( cval & CLKCTRL_MODULEMODE_MSK ) &&
						!( cval & CLKCTRL_IDLEST_MSK ) ) {
					break;
				}
				nanospin_ns( 1000L );
			}
				// smart idle
			out32( mmchs->mmc_base + MMCHS_SYSCONFIG,
				( in32( mmchs->mmc_base + MMCHS_SYSCONFIG ) &
					~SYSCONFIG_CONFIG_MSK ) | SYSCONFIG_CONFIG );

			if( in32( mmchs->context_reg ) & CONTEXT_LOSTCONTEXT_RFF ) {
				out32( mmchs->context_reg, CONTEXT_LOSTCONTEXT_RFF );
				omap_restore_context( hc );
			}

			break;

		case PM_SLEEP:
			omap_set_ldo( hc, SDIO_LDO_VCC, 0 );		// pwr off device

#ifdef BS_PAD_CONF
			bs_pad_conf( hc, SDIO_FALSE );
#endif

#ifdef BS_PM_IDLE
				// force idle
			out32( mmchs->mmc_base + MMCHS_SYSCONFIG,
				in32( mmchs->mmc_base + MMCHS_SYSCONFIG ) & ~SYSCONFIG_CONFIG_MSK );

			out32( mmchs->clkctrl_reg, in32( mmchs->clkctrl_reg ) & ~CLKCTRL_MODULEMODE_MSK );
#endif
			break;

		default:
			break;
	}

	return( EOK );
}

static int omap_reset( sdio_hc_t *hc, uint32_t rst )
{
	omap_hc_mmchs_t		*mmchs;
	sdio_hc_cfg_t		*cfg;
	uintptr_t			base;
	int					stat;
	uint32_t			sctl;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;
	cfg		= &hc->cfg;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  rst %x", __FUNCTION__, rst );
#endif

	sctl	= in32( base + MMCHS_SYSCTL );

		// wait up to 100 ms for reset to complete
	out32( base + MMCHS_SYSCTL, sctl | rst );

		// on OMAP54XX the reset bit is cleared before we can test it, therefore this call will always time out.
	if ( cfg->did < OMAP_DID_54XX ) {
		stat = omap_waitmask( hc, MMCHS_SYSCTL, rst, rst, 10000 );
	}
	stat = omap_waitmask( hc, MMCHS_SYSCTL, rst, 0, 100000 );

	if( stat ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  timeout", __FUNCTION__ );
	}

	return( stat );
}

static int omap_pio_xfer( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;
	int					len;
	int					msk;
	int					blksz;
	uint8_t				*addr;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;
	msk		= ( cmd->flags & SCF_DIR_IN ) ? PSTATE_BRE : PSTATE_BWE;

	while( ( in32( base + MMCHS_PSTATE ) & msk ) ) {
		blksz	= cmd->blksz;
		while( blksz ) {
			if( sdio_sg_nxt( hc, &addr, &len, blksz ) ) {
				break;
			}
			blksz	-= len;
			len		/= 4;
			if( ( cmd->flags & SCF_DIR_IN ) ) {
				in32s( addr, len, base + MMCHS_DATA );
			}
			else {
				out32s( addr, len, base + MMCHS_DATA );
			}
		}
	}

	return( EOK );
}

static int omap_intr_event( sdio_hc_t *hc )
{
	omap_hc_mmchs_t		*mmchs;
	sdio_cmd_t			*cmd;
	uint32_t			sts;
	uint32_t			cs;
	uintptr_t			base;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;

	cmd		= hc->wspc.cmd;
	cs		= CS_CMD_INPROG;

	sts = in32( base + MMCHS_STAT );

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  STAT %x - %x, BLK %x",
		__FUNCTION__, sts, sts & ( in32( base + MMCHS_IE ) | INTR_ERRI ), in32( mmchs->mmc_base + MMCHS_BLK ) );
#endif

	sts &= in32( base + MMCHS_IE ) | INTR_ERRI;
	out32( base + MMCHS_STAT, sts );

	if( ( sts & ( INTR_RETUNE ) ) ) {
		sdio_hc_event( hc, HC_EV_TUNE );
	}

	if( cmd == NULL ) {
		return( EOK );
	}

	if( ( sts & ( INTR_ERRI | INTR_ADMAE ) ) ) {			// Check of errors
		if(sts & (INTR_DCRC | INTR_CCRC |INTR_ADMAE | INTR_ACE |INTR_CEB)){
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: error: STAT %x - %x, BLK %x",
				__FUNCTION__, sts, sts & ( in32( base + MMCHS_IE ) | INTR_ERRI ), in32( mmchs->mmc_base + MMCHS_BLK ) );
		}
		if( sts & INTR_DTO )	cs = CS_DATA_TO_ERR;
		if( sts & INTR_DCRC )	cs = CS_DATA_CRC_ERR;
		if( sts & INTR_DEB )	cs = CS_DATA_END_ERR;
		if( sts & INTR_CTO )	cs = CS_CMD_TO_ERR;
		if( sts & INTR_CCRC )	cs = CS_CMD_CRC_ERR;
		if( sts & INTR_CEB )	cs = CS_CMD_END_ERR;
		if( sts & INTR_CIE )	cs = CS_CMD_IDX_ERR;

		if( sts & INTR_ADMAE ) {
			cs = CS_DATA_TO_ERR;
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: MMCHS_ADMAES = 0x%08x", __FUNCTION__, in32( base + MMCHS_ADMAES ) );
		}

		if( sts & INTR_ACE )	cs = CS_DATA_TO_ERR;
		if( !cs )				cs = CS_CMD_CMP_ERR;
		if( ( mmchs->flags & OF_SDMA_ACTIVE ) )	omap_sdma_stop( hc );
		if( ( mmchs->flags & OF_EDMA_ACTIVE ) )	omap_edma_stop( hc );
		omap_reset( hc, SYSCTL_SRD );
		omap_reset( hc, SYSCTL_SRC );
	}
	else {
		if( ( sts & INTR_CC ) && !( cmd->flags & SCF_DATA_MSK ) ) {
			if( cmd->flags & SCF_RSP_136 ) {
				cmd->rsp[0] = in32( base + MMCHS_RSP76 );
				cmd->rsp[1] = in32( base + MMCHS_RSP54 );
				cmd->rsp[2] = in32( base + MMCHS_RSP32 );
				cmd->rsp[3] = in32( base + MMCHS_RSP10 );
			}
			else if( ( cmd->flags & SCF_RSP_PRESENT ) ) {
				cmd->rsp[0] = in32( base + MMCHS_RSP10 );
			}

			cs = CS_CMD_CMP;
		}

		if( sts & ( INTR_BWR | INTR_BRR ) ) {
			cs = omap_pio_xfer( hc, cmd );
			// according to spec, the tuning command will only fire a BRR interrupt, but no TC.
			if ( ( cs == EOK ) && ( ( cmd->opcode == MMC_SEND_TUNING_BLOCK ) || ( cmd->opcode == SD_SEND_TUNING_BLOCK ) ) ) {
				cs = CS_CMD_CMP;
			}
		}

		if( ( sts & INTR_TC ) ) {
			cs = CS_CMD_CMP;
			cmd->rsp[0] = in32( base + MMCHS_RSP10 );
		}
	}

	if( cs != CS_CMD_INPROG ) {
		if( ( mmchs->flags & OF_SDMA_ACTIVE ) ) {
#ifdef OMAP_DEBUG
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  cmd cmplt sdma active", __FUNCTION__ );
#endif
			mmchs->cs = cs;
			return( EOK );
		}

		//	EDMA completion
		if ((mmchs->flags & OF_EDMA_ACTIVE)) {
			edma3_verify_completion(&mmchs->edma3);
			mmchs->flags &= ~OF_EDMA_ACTIVE;
			edma3_transfer_done(&mmchs->edma3);
		}

#ifdef OMAP_DEBUG
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  cmd cmplt", __FUNCTION__ );
#endif

		sdio_cmd_cmplt( hc, cmd, cs );
	}

	return( EOK );
}

int omap_sdma_event( sdio_hc_t *hc )
{
	omap_hc_mmchs_t		*mmchs;
	sdio_cmd_t			*cmd;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  csr 0x%x, flgs 0x%x", __FUNCTION__, mmchs->sdma_csr, mmchs->flags );
#endif

	if( ( cmd = hc->wspc.cmd ) == NULL || !( mmchs->flags & OF_SDMA_ACTIVE ) ) {
		return( EOK );
	}

	mmchs->flags &= ~OF_SDMA_ACTIVE;
	if( ( mmchs->sdma_csr & DMA4_CSR_ERROR ) || !( mmchs->sdma_csr & DMA4_CSR_BLOCK ) ) {
		omap_reset( hc, SYSCTL_SRD );
		sdio_cmd_cmplt( hc, cmd, CS_CMD_CMP_ERR );
	}
	else if( mmchs->sgc ) {
		omap_sdma_xfer( hc, cmd->flags, cmd->blksz, mmchs->sgp );
	}
	else {
		if( mmchs->cs ) {
			sdio_cmd_cmplt( hc, cmd, mmchs->cs );
		}
	}

	return( EOK );
}

static int omap_event( sdio_hc_t *hc, sdio_event_t *ev )
{
	int		status;

	switch( ev->code ) {
		case HC_EV_INTR:
			status = omap_intr_event( hc );
			InterruptUnmask( hc->cfg.irq[0], hc->hc_iid );
			break;

		case HC_EV_DMA:
			status = omap_sdma_event( hc );
			break;

		default:
#ifdef BS_EVENT
			status = bs_event( hc, ev );
#else
			status = ENOTSUP;
#endif
			break;
	}

	return( status );
}

static int omap_sdma_setup( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	omap_hc_mmchs_t		*mmchs;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;

	if( !( cmd->flags & SCF_DATA_PHYS ) ) {
		sdio_vtop_sg( cmd->sgl, mmchs->sgl, cmd->sgc, cmd->mhdl );
		mmchs->sgp = mmchs->sgl;
	}
	else {
		mmchs->sgp = cmd->sgl;
	}

	mmchs->sgc		= cmd->sgc;

	omap_sdma_xfer( hc, cmd->flags, cmd->blksz, mmchs->sgp );

	return( EOK );
}

static int omap_edma_setup( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	omap_hc_mmchs_t* mmchs = (omap_hc_mmchs_t *)hc->cs_hdl;

	if( !( cmd->flags & SCF_DATA_PHYS ) ) {
		sdio_vtop_sg( cmd->sgl, mmchs->sgl, cmd->sgc, cmd->mhdl );
		mmchs->sgp = mmchs->sgl;
	}
	else {
		mmchs->sgp = cmd->sgl;
	}

	mmchs->sgc = cmd->sgc;

	//	If the setup fails, return an error code to the caller.
	//
	//	This makes it possible to fall back to PIO if for example
	//	the number of configured PaRAM sets was insufficient for
	//	the requested number of scatter gather fragments.
	//
	//	The virtual addresses of the I/O buffers were converted to
	//	physical for DMA, and they are now unsuitable for PIO.
	//	Fortunately, the PIO code will grab the buffer addresses
	//	from somewhere else. Therefore it will get the original
	//	virtual addresses, and it'll all be fine and dandy.
	//
	return omap_edma_xfer(mmchs, cmd->flags, cmd->blksz, mmchs->sgp);
}

static int omap_adma_setup( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	omap_hc_mmchs_t		*mmchs;
	omap_adma32_t		*adma;
	sdio_sge_t			*sgp;
	int					sgc;
	int					sgi;
	int					acnt;
	int					alen;
	int					sg_count;
	paddr_t				paddr;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	adma	= (omap_adma32_t *)mmchs->adma;

	sgc = cmd->sgc;
	sgp = cmd->sgl;

	if( !( cmd->flags & SCF_DATA_PHYS ) ) {
		sdio_vtop_sg( sgp, mmchs->sgl, sgc, cmd->mhdl );
		sgp = mmchs->sgl;
	}

	for( sgi = 0, acnt = 0; sgi < sgc; sgi++, sgp++ ) {
		paddr		= sgp->sg_address;
		sg_count	= sgp->sg_count;
		while( sg_count ) {
			alen		= min( sg_count, ADMA2_MAX_XFER );
			adma->attr	= ADMA2_VALID | ADMA2_TRAN;
			adma->addr	= paddr;
			adma->len	= alen;
			sg_count	-= alen;
			paddr		+= alen;
			adma++;
			if( ++acnt > DMA_DESC_MAX ) {
				return( ENOTSUP );
			}
		}
	}

	adma--;
	adma->attr |= ADMA2_END;

#ifdef SDIO_OMAP_BUS_SYNC
	omap_bus_sync(hc);
#endif

	out32( mmchs->mmc_base + MMCHS_ADMASAL, mmchs->admap );

	return( EOK );
}

static int omap_xfer_setup( sdio_hc_t *hc, sdio_cmd_t *cmd, uint32_t *command, uint32_t *imask )
{
	omap_hc_mmchs_t		*mmchs;
	int					status;

	mmchs		= (omap_hc_mmchs_t *)hc->cs_hdl;
	status		= EOK;
	*command	|= ( cmd->flags & SCF_DIR_IN ) ? ( CMD_DP | CMD_DDIR ) : CMD_DP;
	*imask		|= INTR_DTO | INTR_DCRC | INTR_DEB | INTR_TC;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  %s, blksz %d, nblks %d", __FUNCTION__, ( cmd->flags & SCF_DIR_IN ) ? "in" : "out", cmd->blksz, cmd->blks );
#endif

		// set timeout

	sdio_sg_start( hc, cmd->sgl, cmd->sgc );

	// Do not enable DMA for tuning commands, just enable BRR interrupt.
	if (cmd->opcode == MMC_SEND_TUNING_BLOCK || cmd->opcode == SD_SEND_TUNING_BLOCK ) {
		// As in SD Host Controller Spec version 3.00 Feb 18, 2010, the controller is supposed to inhibit
		// all interrupts except BRR (block read ready) during the tuning procedure (ET=1).
		*imask = INTR_BRR;
	}
	else {
		//	Default is to use DMA if it's available
		int use_dma = ((hc->caps & HC_CAP_DMA) != 0);

		//	If the DMA setup fails, fall back to PIO.
		//	This seemed to be the logic in the original code too.
		if (use_dma) {
			if( ( mmchs->flags & OF_USE_ADMA ) ) {
				status = omap_adma_setup( hc, cmd );
				*imask |= INTR_ADMAE;
			}
			else if (mmchs->flags & OF_USE_EDMA) {
				status = omap_edma_setup( hc, cmd );
			}
			else {
				status = omap_sdma_setup( hc, cmd );
			}

			if( status == EOK ) {
				*command |= CMD_DE;
			}
			else {
				//	DMA setup failed.
				//	If PIO isn't possible, return the error status.
				if (cmd->flags & SCF_DATA_PHYS) {
					return status;
				}
				status = EOK; // Silently forget the error.
				use_dma = 0; // Then do PIO instead.
			}
		}

		if (!use_dma) {  // Use PIO
			if( ( cmd->flags & SCF_DATA_PHYS ) ) {
				//	PIO needs a virtual address, and we have only physical.
				//	Clearly no I/O can be done, but why return OK in this case?
				return( status );
			}

			//	Add "read ready" or "write ready" interrupt to the interrupt mask.
			*imask |= ( cmd->flags & SCF_DIR_IN ) ? INTR_BRR : INTR_BWR;
		}

		//	If block count is more than one, add flags for multiple block transfer.
		if( cmd->blks > 1 ) {
			*command |= CMD_MBS | CMD_BCE;
			if( ( hc->caps & HC_CAP_ACMD23 ) && ( cmd->flags & SCF_SBC ) ) {
				*command |= CMD_ACMD23;
				out32( mmchs->mmc_base + MMCHS_SDMASA, cmd->blks );
			}
			else if( ( hc->caps & HC_CAP_ACMD12 ) ) {
				*command |= CMD_ACMD12;
			}
		}
	}

	//	Set the block count register.
	//	Both block count and block size are always set, even though the
	//	count might not be strictly required when it's equal to one.
	out32( mmchs->mmc_base + MMCHS_BLK, ( cmd->blks << 16 ) | cmd->blksz );

	return( status );
}

static int omap_cmd( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;
	uint32_t			imask;
	int					status;
	uint32_t			command;

	mmchs			= (omap_hc_mmchs_t *)hc->cs_hdl;
	base			= mmchs->mmc_base;
	imask			= INTR_DFLTS;

	mmchs->cs		= cmd->status = CS_CMD_INPROG;

#ifdef BS_POWMAN
	bs_powman( hc, POWMAN_ALIVE );
#endif

	out32( base + MMCHS_STAT, STAT_CLR_MSK );	// Clear Status

	command	= cmd->opcode << 24;
	if( cmd->opcode == MMC_STOP_TRANSMISSION ) {
		command |= CMD_TYPE_CMD12;
	}

	if( ( cmd->flags & SCF_DATA_MSK ) ) {
		if( ( status = omap_xfer_setup( hc, cmd, &command, &imask ) ) != EOK ) {
			return( status );
		}
//	The file "hardware/devb/mmcsd/chipsets/sim_omap3.c" has the following
//	piece of code in roughly this kind of place.
//	Find out if this is needed in here too.
#ifdef MMCSD_VENDOR_TI_JACINTO5
		/*
		 * Errata SPRZ382A, the J5/J5eco chips check certain errors
		 * only when the corresponding bits in CSRE register are set
		 */
		out32(base + MMCHS_CSRE, 0xFDFFA080);
#endif
	}
	else {
		imask |= INTR_CC;						// Enable command complete intr
	}

	if( ( cmd->flags & SCF_RSP_PRESENT ) ) {
		if( ( cmd->flags & SCF_RSP_136 ) ) {
			command |= CMD_RSP_TYPE_136;
		}
		else if( ( cmd->flags & SCF_RSP_BUSY ) ) {	// Response with busy check
			command |= CMD_RSP_TYPE_48b;
		}
		else {
			command |= CMD_RSP_TYPE_48;
		}

		if( ( cmd->flags & SCF_RSP_OPCODE ) ) {		// Index check
			command |= CMD_CICE;
		}

		if( ( cmd->flags & SCF_RSP_CRC ) ) {		// CRC check
			command |= CMD_CCCE;
		}
	}

	out32( base + MMCHS_IE,  imask );
	out32( base + MMCHS_ARG, cmd->arg );
	out32( base + MMCHS_CMD, command );

	return( EOK );
}

static int omap_abort( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	omap_hc_mmchs_t		*mmchs;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;

	if( ( mmchs->flags & OF_SDMA_ACTIVE ) ) {
		omap_sdma_stop( hc );
	}

	if( ( mmchs->flags & OF_EDMA_ACTIVE ) ) {
		omap_edma_stop(hc);
	}

	delay( 2 );			// allow pending events to complete

	return( EOK );
}

static int omap_pwr( sdio_hc_t *hc, int vdd )
{
	omap_hc_mmchs_t		*mmchs;
	uint32_t			con;
	uint32_t			capa;
	uint32_t			hctl;
	uintptr_t			base;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  vdd 0x%x", __FUNCTION__, vdd );
#endif

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;

	omap_pm( hc, PM_ACTIVE );

	out32( base + MMCHS_IE, 0 );		// disable interrupts
	out32( base + MMCHS_ISE, 0 );

	if( !vdd ) {
		(void)omap_mmchs_softreset( hc );
		omap_reset( hc, SYSCTL_SRA );

		hctl	= 0;
		con		= CON_DEBOUNCE_8MS;
		if( ( mmchs->flags & OF_USE_ADMA ) ) {
			con = CON_DMA_MNS;
			hctl |= HCTL_DMAS_ADMA2;
		}
		out32( base + MMCHS_CON, con );

		capa = in32( base + MMCHS_CAPA );
		if( ( hc->ocr & ( OCR_VDD_30_31 | OCR_VDD_29_30 ) ) ) {
			capa |= CAP_VS3V0;
		}
		if( ( hc->ocr & OCR_VDD_17_195 ) ) {
			capa |= CAP_VS1V8;
		}

		out32( base + MMCHS_CAPA, capa );
		out32( base + MMCHS_HCTL, hctl );

		if( mmchs->mmc_pbase != MMCHS2_BASE ) {		// don't turn off eMMC
			omap_set_ldo( hc, SDIO_LDO_VCC, 0 );	// pwr off device
// don't turn off IO signals
//			omap_set_ldo( hc, SDIO_LDO_VCC_IO, 0 );	// pwr off ctrl/iface
#ifdef BS_PAD_CONF
			bs_pad_conf( hc, SDIO_FALSE );
#endif
		}

		omap_pm( hc, PM_IDLE );
	}
	else {
		mmchs->flags |= OF_SEND_INIT_STREAM;

		hctl = in32( base + MMCHS_HCTL ) & ~HCTL_SDVS_MSK;
		out32( base + MMCHS_HCTL, hctl );

#ifdef BS_PAD_CONF
		bs_pad_conf( hc, SDIO_TRUE );
#endif

		omap_set_ldo( hc, SDIO_LDO_VCC, ( vdd <= OCR_VDD_17_195 ) ? 1800 : 3000 );
		omap_set_ldo( hc, SDIO_LDO_VCC_IO, 3000 );

		hctl |= ( ( vdd <= OCR_VDD_17_195 ) ? HCTL_SDVS1V8 : HCTL_SDVS3V0 );
		out32( base + MMCHS_HCTL, hctl );
		out32( base + MMCHS_HCTL, hctl | HCTL_SDBP );
		omap_waitmask( hc, MMCHS_HCTL, HCTL_SDBP, HCTL_SDBP, 10000 );

		out32( base + MMCHS_ISE, INTR_ALL );			// enable intrs
	}

	hc->vdd = vdd;

	return( EOK );
}

static int omap_bus_mode( sdio_hc_t *hc, int bus_mode )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;
	uint32_t			con;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  bus_mode %d", __FUNCTION__, bus_mode );
#endif

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;

	con = in32( base + MMCHS_CON ) & ~CON_OD;
	if( bus_mode == BUS_MODE_OPEN_DRAIN ) {
		con |= CON_OD;
	}

	out32( base + MMCHS_CON, con );

	hc->bus_mode = bus_mode;

	return( EOK );
}

static int omap_bus_width( sdio_hc_t *hc, int width )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;
	uint32_t			con;
	uint32_t			hctl;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  width %d", __FUNCTION__, width );
#endif

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;
	con		= in32( base + MMCHS_CON ) & ~CON_DW8;
	hctl	= in32( base + MMCHS_HCTL ) & ~HCTL_DTW4;

	switch( width ) {
		case BUS_WIDTH_8:
			con |= CON_DW8; break;
		case BUS_WIDTH_4:
			hctl |= HCTL_DTW4; break;
		case BUS_WIDTH_1:
		default:
			break;
	}

	out32( base + MMCHS_CON, con );
	out32( base + MMCHS_HCTL, hctl );

	hc->bus_width = width;

	return( EOK );
}

static int omap_clk( sdio_hc_t *hc, int clk )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;
	uint32_t			con;
	uint32_t			clk_ref;
	uint32_t			sysctl;
	int					clkd;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  clk %d", __FUNCTION__, clk );
#endif

	mmchs		= (omap_hc_mmchs_t *)hc->cs_hdl;
	base		= mmchs->mmc_base;
	clk_ref		= mmchs->clk_ref;

	//	If the clkctrl register hasn't been set, this code can be skipped.
	if( mmchs->clkctrl_reg != 0 )
	{
		uint32_t clkctrl = in32( mmchs->clkctrl_reg ) & ~CLKCTRL_CLKSEL_MAX;

		if( hc->cfg.did == OMAP_DID_44XX && ( hc->timing == TIMING_SDR50 ) ) {
			clk_ref = MMCHS_CLK_REF_64MHZ;
		}
		else {
			if( hc->cfg.did >= OMAP_DID_54XX ) {
				// clear divider settings to ensure maximum clock rate
				// clear bits 25 and 26 for J6. Bit 26 is reserved (unused) on OMAP54xx.
				clkctrl &= ~CLKCTRL_CLKSEL_DIV_MSK;
			}
			// on OMAP543X this setting enables clock of 192MHz, on OMAP3 and OMAP4 96MHz
			clkctrl |= CLKCTRL_CLKSEL_MAX;
		}

		out32( mmchs->clkctrl_reg, clkctrl );
	}

#if 1
		// clock can not be higher than reference clock, unless we have a multiplier.
	if ( clk > mmchs->clk_ref ) {
		clk = mmchs->clk_ref;
	}
		// without limiting clk to clk_ref, this calculation will compute an incorrect divider.
	clkd	= ( ( clk_ref + clk ) - 1 ) / clk;
#else
	if( mmchs->clk_mul ) {
		if( !( in32( base + MMCHS_HCTL2 ) & HCTL2_PRESET_VAL ) ) {
			for( clkd = 1; clkd < 1024; clkd++ ) {
				if( ( ( hc->clk_max * mmchs->clk_mul ) / clkd ) <= clk ) {
					break;
				}
			}
			sctl |= SYSCTL_CGS_PROG;
			clkd--;
		}
	}
	else {
			// 10 bit Divided Clock Mode
		for( clkd = 2; clkd < CLK_MAX_DIV; clkd += 2 ) {
			if( ( hc->clk_max / clkd ) <= clk ) {
				break;
			}
		}
		if( hc->clk_max <= clk ) {
			clkd = 1;
		}
		clkd >>= 1;
	}
#endif

	sysctl = in32( base + MMCHS_SYSCTL );

		// Stop clock
	sysctl &= ~(SYSCTL_ICE | SYSCTL_CEN | SYSCTL_DTO_MSK);
	sysctl |= SYSCTL_DTO_MAX | SYSCTL_SRC | SYSCTL_SRD;
	out32( base + MMCHS_SYSCTL, sysctl );

		// Enable internal clock
	sysctl &= ~(0x3FF << 6);
	sysctl |= (clkd << 6) | SYSCTL_ICE;
	out32( base + MMCHS_SYSCTL, sysctl );

		// Wait for the clock to be stable
	omap_waitmask( hc, MMCHS_SYSCTL, SYSCTL_ICS, SYSCTL_ICS, 10000 );

		// Enable clock to the card
	out32( base + MMCHS_SYSCTL, sysctl | SYSCTL_CEN );

	if( ( mmchs->flags & OF_SEND_INIT_STREAM ) ) {
		mmchs->flags &= ~OF_SEND_INIT_STREAM;
		con = in32( base + MMCHS_CON );
		out32( base + MMCHS_CON, con | CON_INIT );
		delay( 5 );
		out32( base + MMCHS_CON, con );
	}

//	hc->clk = clk;
	hc->clk = clk_ref / clkd;

	return( EOK );
}

static int omap_timing( sdio_hc_t *hc, int timing )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;
	uint32_t			con;
	uint32_t			hctl;
	uint32_t			hctl2;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: timing %d", __FUNCTION__, timing );
#endif

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;

	con		= in32( base + MMCHS_CON ) & ~CON_DDR;
	hctl	= in32( base + MMCHS_HCTL ) & ~HCTL_HSE;
	hctl2	= 0;

	if( hc->version >= REV_SREV_V3 ) {
		hctl2	= in32( base + MMCHS_HCTL2 ) & ~HCTL2_MODE_MSK;
	}

	switch( timing ) {
		case TIMING_HS200:
			hctl |= HCTL_HSE;
				// HCTL2_MODE_HS200 undefined on OMAP54XX
			if ( ( hc->cfg.did == OMAP_DID_54XX ) || ( hc->cfg.did == OMAP_DID_54XX_ES2 ) ||
			     ( hc->cfg.did == OMAP_DID_DRA7XX ) || ( hc->cfg.did == OMAP_DID_DRA7XX_ES2 ))
				hctl2 |= HCTL2_MODE_SDR104;
			else
				hctl2 |= HCTL2_MODE_HS200;
			break;

		case TIMING_SDR104:
			hctl |= HCTL_HSE;
			hctl2 |= HCTL2_MODE_SDR104;
			break;

		case TIMING_SDR50:
			hctl |= HCTL_HSE;
			hctl2 |= HCTL2_MODE_SDR50;
			break;

		case TIMING_SDR25:
			hctl |= HCTL_HSE;
			hctl2 |= HCTL2_MODE_SDR25;
			break;

		case TIMING_SDR12:
			hctl |= HCTL_HSE;
			break;

		case TIMING_DDR50:
			con |= CON_DDR;
			hctl2 |= HCTL2_MODE_DDR50;
			break;

		case TIMING_HS:
			break;

		case TIMING_LS:
			break;

		default:
			break;
	}

	hctl &= ~HCTL_HSE;		// TI doesn't recommend HSE

	if( hc->version >= REV_SREV_V3 ) {
			// check for preset value enable and disble clock
		if( ( hctl2 & HCTL2_PRESET_VAL ) ) {
			out32( base + MMCHS_SYSCTL, in32( base + MMCHS_SYSCTL ) & ~SYSCTL_CEN );
		}

		out32( base + MMCHS_HCTL, hctl );
		out32( base + MMCHS_HCTL2, hctl2 );

		if( ( hctl2 & HCTL2_PRESET_VAL ) ) {
			omap_clk( hc, hc->clk );
		}
	}

	out32( base + MMCHS_CON, con );
	out32( base + MMCHS_HCTL, hctl );

	hc->timing = timing;

	return( EOK );
}

static int omap_signal_voltage( sdio_hc_t *hc, int signal_voltage )
{
	omap_hc_mmchs_t		*mmchs;
	sdio_hc_cfg_t		*cfg;
	uintptr_t			base;
	uint32_t			hctl;
	uint32_t			con;
	int					status;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  signal voltage %d", __FUNCTION__, signal_voltage );
#endif

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	status	= EOK;
	base	= mmchs->mmc_base;
	cfg		= &hc->cfg;
	con		= in32( base + MMCHS_CON );
	hctl	= in32( base + MMCHS_HCTL ) & ~HCTL_SDVS_MSK;

	if( signal_voltage == SIGNAL_VOLTAGE_1_8 ) {

		// Assumption:  SD VOLTAGE SWITCH (CMD 11) has been issued to device
		// OMAP5432 spec mentions CMD11 for signal voltage switch, but according to
		// JEDEC MMC 4.51, CMD 11 is obsolete and response undefined.

		delay( 10 );		// some cards need a bit of time (Patriot)

			// force ADPIDLE mode to active state
		out32( base + MMCHS_CON, con | CON_PADEN );

			// wait for CLEV = 0x0, DLEV = 0x0
			// This check always times out on OMAP5432 ES2.0, even with very long timeout values.
			// needs to be verified on J6
		if( ( cfg->did == OMAP_DID_54XX ) || ( cfg->did == OMAP_DID_54XX_ES2 ) ||
			( cfg->did == OMAP_DID_DRA7XX ) || ( cfg->did == OMAP_DID_DRA7XX_ES2 ) ||
			( status = omap_waitmask( hc, MMCHS_PSTATE, PSTATE_CLEV | PSTATE_DLEV_MSK, 0, MMCHS_PWR_SWITCH_TIMEOUT ) ) == EOK ) {
			if( ( status = omap_set_ldo( hc, SDIO_LDO_VCC_IO, 1800 ) ) == EOK ) {
				out32( base + MMCHS_HCTL, hctl | HCTL_SDVS1V8 );

				if( hc->version >= REV_SREV_V3 ) {
					out32( base + MMCHS_HCTL2, in32( base + MMCHS_HCTL2 ) | HCTL2_SIG_1_8V );
				}

				out32( base + MMCHS_CON, in32( base + MMCHS_CON ) | CON_CLKEXTFREE );

						// wait for CLEV = 0x1, DLEV = 0xf
				if( ( status = omap_waitmask( hc, MMCHS_PSTATE, PSTATE_CLEV | PSTATE_DLEV_MSK, PSTATE_CLEV | PSTATE_DLEV_MSK, MMCHS_PWR_SWITCH_TIMEOUT ) ) == EOK ) {
				}
			}
		}

			// cut off external card clock, remove forcing from ADPIDLE pin
		con &= ~( CON_CLKEXTFREE | CON_PADEN );
		out32( base + MMCHS_CON, con );
	}
	else if( signal_voltage == SIGNAL_VOLTAGE_3_3 ) {
		if( hc->vdd > OCR_VDD_17_195 ) {
			if( ( status = omap_set_ldo( hc, SDIO_LDO_VCC_IO, 3000 ) ) == EOK ) {
				out32( base + MMCHS_HCTL, hctl | HCTL_SDVS3V0 );
				if( hc->version >= REV_SREV_V3 ) {
					out32( base + MMCHS_HCTL2, in32( base + MMCHS_HCTL2 ) & ~HCTL2_SIG_1_8V );
				}
			}
		}
	}
	else {
		return( EINVAL );
	}

	hc->signal_voltage = signal_voltage;

	return( status );
}

static int omap_drv_type( sdio_hc_t *hc, int type )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;
	uint32_t			hctl2;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: type %d", __FUNCTION__, type );
#endif

	if( hc->version < REV_SREV_V3 ) {
		return( EOK );
	}

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;
	hctl2	= in32( base + MMCHS_HCTL2 ) & ~HCTL2_DRV_TYPE_MSK;

	if( !( hctl2 & HCTL2_PRESET_VAL ) ) {
		switch( type ) {
			case DRV_TYPE_A:
				hctl2 |= HCTL2_DRV_TYPE_A; break;
			case DRV_TYPE_C:
				hctl2 |= HCTL2_DRV_TYPE_C; break;
			case DRV_TYPE_D:
				hctl2 |= HCTL2_DRV_TYPE_D; break;
			case DRV_TYPE_B:
			default:
				hctl2 |= HCTL2_DRV_TYPE_B; break;
				break;
		}

		out32( base + MMCHS_HCTL2, hctl2 );
	}

	hc->drv_type = type;

	return( EOK );
}

// Set DLL phase delay for HS tuning
static int omap_set_dll( sdio_hc_t *hc, int delay)
{
	omap_hc_mmchs_t		*mmchs;
	uint32_t			dll;
	uintptr_t			base;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;

	dll = ( in32( base + MMCHS_DLL ) & ~( DLL_SLAVE_RATIO_MSK | DLL_CALIB ) );
	dll |= ( delay << DLL_SLAVE_RATIO_SHIFT );
	out32( base + MMCHS_DLL, dll);
	dll |= DLL_CALIB;
	out32( base + MMCHS_DLL, dll);
	// Flush last write
	dll = in32( base + MMCHS_DLL );

	return ( EOK );
}

static int omap_tune( sdio_hc_t *hc, int op )
{
	omap_hc_mmchs_t		*mmchs;
	struct sdio_cmd		*cmd;
	sdio_sge_t			sge;
	uint32_t			*td;
	uintptr_t			base;
	uint32_t			hctl2;
	uint32_t			pdelay;
	int					tlc;
	int					tlen;
	int					status;
	int					index = 0xFF;
	int					imax = 0;
	int					wlen = 0;
	int					wmax = 0;
	int					pm = 0;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: ", __FUNCTION__ );
#endif

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;
	tlen	= hc->bus_width == BUS_WIDTH_8 ? 128 : 64;

	if( hc->version < REV_SREV_V3 ) {
		return( EOK );
	}

	hctl2 = in32( base + MMCHS_HCTL2 );

		// return if not HS200 or SDR104, and not SDR50 that requires tuning
	if( ( HCTL2_MODE( hctl2 ) != HCTL2_MODE_SDR104 ) &&
			( HCTL2_MODE( hctl2 ) != HCTL2_MODE_HS200 ) &&
		    ( ( HCTL2_MODE( hctl2 ) == HCTL2_MODE_SDR50 ) &&
		    !( mmchs->flags & OF_TUNE_SDR50 ) ) ) {
		return( EOK );
	}

	if( ( cmd = sdio_alloc_cmd( ) ) == NULL ) {
		return( ENOMEM );
	}

		// on OMAP5 the driver has to read the tuning data and compare it with the reference data
		// therefore we need a buffer here for the tuning data
	if( ( td = (uint32_t*) sdio_alloc( tlen ) ) == NULL ) {
		sdio_free_cmd( cmd );
		return( ENOMEM );
	}

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: start tuning", __FUNCTION__ );
#endif
		// start the tuning procedure by setting MMCHS_AC12[22] ET to 0x1
	hctl2 |= HCTL2_EXEC_TUNING;
	out32( base + MMCHS_HCTL2, hctl2 );

		// it is important to increment tlc to be able to calculate correct phase delay
	for( tlc = 0; ( tlc < MMCHS_TUNING_RETRIES ); tlc++) {

			// clear tuning data buffer to avoid comparing old data after unsuccessful transfer
		memset(td, 0, tlen);
			// check whether the DLL is locked
		if ( ( status = omap_waitmask( hc, MMCHS_DLL, DLL_LOCK, DLL_LOCK, MMCHS_TUNING_TIMEOUT ) ) != EOK ) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: DLL_LOCK TIMEOUT status: %#x", __FUNCTION__, status);
			break;
		}
			// setup tuning command
		sdio_setup_cmd( cmd, SCF_CTYPE_ADTC | SCF_RSP_R1, op, 0 );
		sge.sg_count = tlen; sge.sg_address = (paddr_t)td;
		sdio_setup_cmd_io( cmd, SCF_DIR_IN, 1, tlen, &sge, 1, NULL );

		if( ( status = sdio_issue_cmd( &hc->device, cmd, MMCHS_TUNING_TIMEOUT ) ) ) {
#ifdef OMAP_DEBUG
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: MMCHS_TUNING_TIMEOUT status: %#x", __FUNCTION__, status);
#endif
			break;
		}

			// determine largest timing window where data transfer is working
		if ( ( cmd->status != CS_CMD_CMP ) || ( memcmp( td, ( hc->bus_width == BUS_WIDTH_8 ) ? sdio_tbp_8bit : sdio_tbp_4bit, tlen ) ) ) {
			pm = 0;
		}
		else {
			if ( pm == 0 ) {
					// new window with successful transmission
				index = tlc;
				wlen = 1;
			}
			else {
				wlen++;
			}
			pm = 1;
			if ( wlen > wmax ) {
				imax = index;
				wmax = wlen;
			}
		}

			// check if tuning is finished
		if( !( ( hctl2 = in32( base + MMCHS_HCTL2 ) ) & HCTL2_EXEC_TUNING ) ) {
			break;
		}
	}

	if( !status && !( hctl2 & HCTL2_EXEC_TUNING ) ) {
			// tuning successful, set phase delay to the middle of the window
		pdelay = 2 * (imax + (wmax >> 1));
			// reset data and command line before setting phase delay,
			// according to OMAP5432 ES2.0 spec.
		omap_reset( hc, SYSCTL_SRD );
		omap_reset( hc, SYSCTL_SRC );
		omap_set_dll(hc, pdelay);
		hc->tuning_count = pdelay;
#ifdef OMAP_DEBUG
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: tuning successful. Set dll 0x%x", __FUNCTION__, pdelay);
#endif
	}
	else {
			// tuning failed
		hctl2 &= ~( HCTL2_TUNED_CLK | HCTL2_EXEC_TUNING );
		out32( base + MMCHS_HCTL2, hctl2 );
		status = EIO;
#ifdef OMAP_DEBUG
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: tuning failed. status %d", __FUNCTION__, status);
#endif
	}

	sdio_free( td, tlen );
	sdio_free_cmd( cmd );

	return( status );
}

static int omap_preset( sdio_hc_t *hc, int enable )
{
	omap_hc_mmchs_t		*mmchs;
	uintptr_t			base;
	uint32_t			hctl2;

#ifdef OMAP_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: ", __FUNCTION__ );
#endif

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;
	base	= mmchs->mmc_base;

	if( hc->version < REV_SREV_V3 ) {
		return( EOK );
	}

	hctl2	= in32( base + MMCHS_HCTL2 ) & ~HCTL2_PRESET_VAL;
	hctl2	|= ( ( enable == SDIO_TRUE ) ? HCTL2_PRESET_VAL : 0 );
	out32( base + MMCHS_HCTL2, hctl2 );

	return( EOK );
}

static int omap_cd( sdio_hc_t *hc )
{
	return( SDIO_TRUE );		// external logic required
}

static void omap_sdma_channel_map( sdio_hc_t *hc )
{
	omap_hc_mmchs_t *mmchs = ( omap_hc_mmchs_t * )hc->cs_hdl;
	sdio_hc_cfg_t *cfg = &hc->cfg;

	switch( mmchs->mmc_pbase ) {
		case MMCHS1_BASE:
			mmchs->sdma_rreq	= cfg->dma_chnl[0] ? cfg->dma_chnl[0] : OMAP_MMC1_RX;
			mmchs->sdma_treq	= cfg->dma_chnl[1] ? cfg->dma_chnl[1] : OMAP_MMC1_TX;
			break;
		case MMCHS2_BASE:
			mmchs->sdma_rreq	= cfg->dma_chnl[0] ? cfg->dma_chnl[0] : OMAP_MMC2_RX;
			mmchs->sdma_treq	= cfg->dma_chnl[1] ? cfg->dma_chnl[1] : OMAP_MMC2_TX;
			break;
		case MMCHS3_BASE:
			mmchs->sdma_rreq	= cfg->dma_chnl[0] ? cfg->dma_chnl[0] : OMAP_MMC3_RX;
			mmchs->sdma_treq	= cfg->dma_chnl[1] ? cfg->dma_chnl[1] : OMAP_MMC3_TX;
			break;
		case MMCHS4_BASE:
			mmchs->sdma_rreq	= cfg->dma_chnl[0] ? cfg->dma_chnl[0] : OMAP_MMC4_RX;
			mmchs->sdma_treq	= cfg->dma_chnl[1] ? cfg->dma_chnl[1] : OMAP_MMC4_TX;
			break;
		case MMCHS5_BASE:
		default:
			mmchs->sdma_rreq	= cfg->dma_chnl[0] ? cfg->dma_chnl[0] : OMAP_MMC5_RX;
			mmchs->sdma_treq	= cfg->dma_chnl[1] ? cfg->dma_chnl[1] : OMAP_MMC5_TX;
			break;
	}
}

int omap_sdma_init( sdio_hc_t *hc )
{
	omap_hc_mmchs_t		*mmchs;
    rsrc_request_t		req;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;

		// set some defaults
	mmchs->sdma_iid		= -1;
	mmchs->sdma_chnl	= -1;
	omap_sdma_channel_map( hc );

	if( ( mmchs->sdma_base = mmap_device_io( OMAP_DMA4_SIZE, OMAP_DMA4_BASE ) ) != (uintptr_t)MAP_FAILED ) {
		memset( &req, 0, sizeof( rsrc_request_t ) );
		req.length	= 1;
		req.flags	= RSRCDBMGR_DMA_CHANNEL;
		if( rsrcdbmgr_attach( &req, 1 ) != -1 ) {
			mmchs->sdma_chnl	= req.start;
			mmchs->sdma_irq		= OMAP_BASE_IRQ + mmchs->sdma_chnl;
			mmchs->sdma_cbase	= mmchs->sdma_base + DMA4_CH_BASE( mmchs->sdma_chnl );
			SIGEV_PULSE_INIT( &mmchs->sdma_ev, hc->hc_coid, SDIO_PRIORITY, HC_EV_DMA, NULL );
			if( ( mmchs->sdma_iid = InterruptAttach( mmchs->sdma_irq, omap_sdma_intr, hc, sizeof( *hc ), _NTO_INTR_FLAGS_TRK_MSK ) ) != -1 ) {
				return( EOK );
			}
			else {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  InterruptAttach SDMA:  (%s)", __FUNCTION__, strerror( errno ) );
			}
		}
		else {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  rsrcdbmgr_attach:  (%s)", __FUNCTION__, strerror( errno ) );
		}
	}
	else {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: mmap_device_io SDMA:  (%s)", __FUNCTION__, strerror( errno ) );
	}

	return( errno );
}

int omap_sdma_dinit( sdio_hc_t *hc )
{
	omap_hc_mmchs_t		*mmchs;
    rsrc_request_t		req;

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;

	if( mmchs->sdma_chnl != -1 ) {
		memset( &req, 0, sizeof( rsrc_request_t ) );
		req.length	= 1;
		req.start	= req.end = mmchs->sdma_chnl;
		req.flags	= RSRCDBMGR_DMA_CHANNEL;
		rsrcdbmgr_detach( &req, 1 );
	}

	if( mmchs->sdma_iid != -1 ) {
		InterruptDetach( mmchs->sdma_iid );
	}

	if( mmchs->sdma_base != (uintptr_t)MAP_FAILED) {
		munmap_device_io( mmchs->sdma_base, OMAP_DMA4_SIZE );
	}

	return( EOK );
}


//	Common entry to DMA initialization. Uses either SDMA or EDMA.
int omap_dma_init(sdio_hc_t *hc)
{
	omap_hc_mmchs_t *mmchs = (omap_hc_mmchs_t *)hc->cs_hdl;
	if (mmchs->flags & OF_USE_EDMA)
		return edma3_init(hc);
	else
		return omap_sdma_init(hc);
}


int omap_dinit( sdio_hc_t *hc )
{
	omap_hc_mmchs_t		*mmchs;

	if( !hc || !hc->cs_hdl)
		return( EOK );

	mmchs	= (omap_hc_mmchs_t *)hc->cs_hdl;

	if( mmchs->mmc_base ) {
		omap_pwr( hc, 0 );
		if( hc->hc_iid != -1 ) {
			InterruptDetach( hc->hc_iid );
		}
		if( mmchs->ctrl_base ) {
			munmap_device_io( mmchs->ctrl_base, CONTROL_PBIAS_SIZE );
		}
		if( mmchs->clkctrl_reg ) {
			munmap_device_io( mmchs->clkctrl_reg, MMC_REGISTER_SIZE );
		}
		if( mmchs->context_reg ) {
			munmap_device_io( mmchs->context_reg, MMC_REGISTER_SIZE );
		}
		munmap_device_io( mmchs->mmc_base, MMCHS_SIZE );
	}

	if( mmchs->adma ) {
		munmap( mmchs->adma, sizeof( omap_adma32_t ) * DMA_DESC_MAX );
#ifdef SDIO_OMAP_BUS_SYNC
		omap_so_dinit(hc);
#endif
	}

	if (mmchs->flags & OF_USE_EDMA)
		edma3_dinit(&mmchs->edma3);
	else
		omap_sdma_dinit( hc );

	free( mmchs );
	hc->cs_hdl = NULL;

	return( EOK );
}

static int omap_register_map( omap_hc_mmchs_t *mmchs, int did )
{
	pm_info_t*  pm_info_tbl;

	if( did >= OMAP_DID_DRA7XX ) {
		pm_info_tbl = dra7xx_info;
	}
	else if( did >= OMAP_DID_54XX_ES2 ) {
		pm_info_tbl = omap5_es2_info;
	}
	else if( did >= OMAP_DID_54XX ) {
		pm_info_tbl = omap5_es1_info;
	}
	else if( did >= OMAP_DID_44XX ) {
		pm_info_tbl = omap4_info;
	}
	else {
		return( EOK );
	}

	for( ; pm_info_tbl->physbase != -1; pm_info_tbl++ ) {
		if( pm_info_tbl->physbase == mmchs->mmc_pbase ) {
			if( ( mmchs->clkctrl_reg = mmap_device_io( MMC_REGISTER_SIZE, pm_info_tbl->clkctrl_phys ) ) == (uintptr_t)MAP_FAILED ) {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: MMCHS L3INIT_CM2 mmap_device_io %s", __FUNCTION__, strerror( errno ) );
				return( errno );
			}

			if( ( mmchs->context_reg = mmap_device_io( MMC_REGISTER_SIZE, pm_info_tbl->context_phys ) ) == (uintptr_t)MAP_FAILED ) {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: MMCHS L3INIT_PRM mmap_device_io %s", __FUNCTION__, strerror( errno ) );
				return( errno );
			}

			return( EOK );
		}
	}

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: MMCHS can not locate CLKCTRL or CONTEXT register for SoC (DID 0x%x)", __FUNCTION__, did );
	errno = EFAULT;
	return( errno );
}

int omap_init( sdio_hc_t *hc )
{
	sdio_hc_cfg_t		*cfg;
	omap_hc_mmchs_t		*mmchs;
	uint32_t			cur;
	uint32_t			cap;
	uint32_t			cap2;
	uint32_t			mccap;
	uint32_t			hwinfo;
	uint32_t			status;
	uintptr_t			base;
	struct sigevent		event;
	char				cbuf[128];

	if( ( hc->cs_hdl = calloc( 1, sizeof( omap_hc_mmchs_t ) ) ) == NULL ) {
		return( ENOMEM );
	}

	cfg					= &hc->cfg;
	mmchs				= (omap_hc_mmchs_t *)hc->cs_hdl;
	mmchs->mmc_pbase	= cfg->base_addr[0];
	mmchs->clk_ref		= MMCHS_CLK_REF_96MHZ;
	hc->hc_iid			= -1;
	cfg->did			= OMAP_DID_44XX;

	if( confstr( _CS_MACHINE, cbuf, sizeof( cbuf ) ) ) {
		if( strstr( cbuf, "OMAP3" ) ) {
			cfg->did		= OMAP_DID_34XX;
		}
		else if( strstr( cbuf, "OMAP4" ) ) {
			cfg->did		= OMAP_DID_44XX;
		}
		else if( strstr( cbuf, "OMAP5" ) ) {
			cfg->did		= OMAP_DID_54XX;
			mmchs->clk_ref	= MMCHS_CLK_REF_192MHZ; // OMAP543X has a maximum clock source of 192MHz

			if( ( base = mmap_device_io( 0x04, OMAP54XX_CONTROL_ID_CODE ) ) == (uintptr_t)MAP_FAILED ) {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: OMAP5 code ID mmap_device_io (0x%x) %s", __FUNCTION__, OMAP54XX_CONTROL_ID_CODE, strerror( errno ) );
				omap_dinit( hc );
				return( errno );
			}

			if( (( in32( base ) & OMAP54XX_CONTROL_ID_CODE_REV_MASK) >> OMAP54XX_CONTROL_ID_CODE_REV_SHIFT) == OMAP54XX_CONTROL_ID_CODE_REV_ES2_0 )
				cfg->did = OMAP_DID_54XX_ES2;
			else
				cfg->did = OMAP_DID_54XX;

			munmap_device_io( base, 4 );
		} else if( strstr( cbuf, "DRA7" ) || strstr( cbuf, "J6" ) ) {
			if( ( base = mmap_device_io( 0x04, DRA74X_CTRL_WKUP_ID_CODE ) ) == (uintptr_t)MAP_FAILED ) {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: DRA7XX code ID mmap_device_io (0x%x) %s", __FUNCTION__, DRA74X_CTRL_WKUP_ID_CODE, strerror( errno ) );
				omap_dinit( hc );
				return( errno );
			}

			if( ( in32( base ) == DRA7xx_CTRL_WKUP_ID_CODE_ES1_0 ) ||
			    ( in32( base ) == DRA7xx_CTRL_WKUP_ID_CODE_ES1_1) )
				cfg->did = OMAP_DID_DRA7XX;
			else
				cfg->did = OMAP_DID_DRA7XX_ES2;

			munmap_device_io( base, 4 );
			mmchs->clk_ref = MMCHS_CLK_REF_192MHZ;
		}
		else if(strstr(cbuf, "J5") || strstr(cbuf, "DM814X") || strstr(cbuf, "AM335X") || strstr(cbuf, "AM437X"))
		{
			//	The above test might detect the J5 target.
			//
			//	Temporarily use hard coded clock speed.
			//
			mmchs->clk_ref = MMCHS_CLK_REF_192MHZ;
			//
			//	I'm now using OMAP_DID_34XX "did" for this J5 target.
			//
			//	Using this did might skip some important initializations
			//	and have who knows what unwanted side-effects.
			//	Should a new "did" be assigned for the device instead,
			//	or some other already defined did be selected?
			//
			//	Figure out the correct did and the correct clock speed
			//	and decide how to set them.
			//
			cfg->did = OMAP_DID_34XX; // Pretend to be OMAP34XX
			//
			//	It seems logical to give the real documented base address
			//	on the command line. To be able to do this for the J5 target,
			//	a small base address adjustment is needed in here to
			//	counteract the subtraction of 0x100 that'll be done for
			//	this device type a few lines down.
			//
			mmchs->mmc_pbase += 0x100;
		}
	}

	//	Use EDMA if "edma" options are present in the command line.
	//
	//	Actually there aren't any specific "edma" options any more because
	//	it's recommended not to add any new option strings to the base.c
	//
	//	Instead, EDMA use is detected by the presence of more than two
	//	dma options. The first two are dma channels as usual, and the rest
	//	is used as PaRAM info. See get_edma_options() for more details.
	//
	if (cfg->dma_chnls > 2) {
		mmchs->flags |= OF_USE_EDMA;
	}

	memcpy( &hc->entry, &omap_hc_entry, sizeof( sdio_hc_entry_t ) );

	if( cfg->did == OMAP_DID_34XX ) {
			// OMAP3 doesn't have the MMCHS_HL_xxx register
		mmchs->mmc_pbase -= 0x100;
	}

	if( ( base = mmchs->mmc_base = mmap_device_io( MMCHS_SIZE, mmchs->mmc_pbase ) ) == (uintptr_t)MAP_FAILED ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: MMCHS base mmap_device_io (0x%x) %s", __FUNCTION__, mmchs->mmc_pbase, strerror( errno ) );
		omap_dinit( hc );
		return( errno );
	}

	if( cfg->did >= OMAP_DID_44XX ) {
		if( mmchs->mmc_pbase == MMCHS1_BASE ) {
			if( ( mmchs->ctrl_base = mmap_device_io( CONTROL_PBIAS_SIZE, CONTROL_PBIAS_BASE( cfg->did ) ) ) == (uintptr_t)MAP_FAILED ) {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: MMCHS CONTROL_PBIAS_BASE mmap_device_io %s", __FUNCTION__, strerror( errno ) );
				omap_dinit( hc );
				return( errno );
			}
		}

		if( omap_register_map( mmchs, cfg->did ) ) {
			omap_dinit( hc );
			return( errno );
		}

		out32( mmchs->context_reg, CONTEXT_LOSTCONTEXT_RFF );
	}

	omap_pm( hc, PM_ACTIVE );

	hc->ocr		= OCR_VDD_30_31 | OCR_VDD_29_30;

	if( cfg->clk )
		hc->clk_max	= cfg->clk;
	else
		hc->clk_max	= MMCHS_CLK_MAX( cfg->did );

	hc->clk_min	= hc->clk_max / CLK_MAX_DIV;

	hc->version	= REV_SREV( in32( base + MMCHS_REV ) );
	cap			= in32( base + MMCHS_CAPA );
	cap2		= hc->version >= REV_SREV_V3 ? in32( base + MMCHS_CAPA2 ) :
					( hc->version >= REV_SREV_V2 ? ( CAP_DDR50 | CAP_SDR50 ) : 0 );
	mccap		= in32( base + MMCHS_MCCAP );
	hwinfo		= in32( base + MMCHS_HL_HWINFO );

	omap_pm( hc, PM_IDLE );

	if( hc->version >= REV_SREV_V3 ) {
		mmchs->clk_mul = CAP_CLK_MULT( cap2 );
		if( mmchs->clk_mul ) {
			hc->clk_min	= ( hc->clk_max * mmchs->clk_mul ) / 1024;
			hc->clk_max	= hc->clk_max * mmchs->clk_mul;
		}
	}

	hc->caps	|= HC_CAP_BW4 | HC_CAP_BW8;
	hc->caps	|= HC_CAP_PIO | HC_CAP_ACMD12;
	hc->caps	|= HC_CAP_200MA | HC_CAP_DRV_TYPE_B;

	// OMAP54XX ES2 and J6 DRA7XX don't have a register indicating HS200 capability
	// Errata i843 on DRA7XX ES1.0 and ES1.1 prevents using 192MHz for MMC1/2/3
	if ( ( cfg->did == OMAP_DID_54XX_ES2 ) || ( cfg->did == OMAP_DID_DRA7XX_ES2 ) )
		hc->caps    |= HC_CAP_HS200;

	if( ( cap & CAP_HS ) )
		hc->caps |= HC_CAP_HS;

	if( ( cap & CAP_DS ) )
		hc->caps |= HC_CAP_DMA;

	if( ( cap2 & CAP_SDR104 ) )
		hc->caps	|= HC_CAP_SDR104;

	if( ( cap2 & CAP_SDR50 ) )
		hc->caps	|= HC_CAP_SDR50 | HC_CAP_SDR25 | HC_CAP_SDR12;

	if( ( cap2 & CAP_DDR50 ) )
		hc->caps	|= HC_CAP_DDR50;

	if( ( cap2 & CAP_TUNE_SDR50 ) )
		mmchs->flags |= OF_TUNE_SDR50;

	if( ( cap2 & CAP_DRIVE_TYPE_A ) )
		hc->caps	|= HC_CAP_DRV_TYPE_A;

	if( ( cap2 & CAP_DRIVE_TYPE_C ) )
		hc->caps	|= HC_CAP_DRV_TYPE_C;

	if( ( cap2 & CAP_DRIVE_TYPE_D ) )
		hc->caps	|= HC_CAP_DRV_TYPE_D;

	if( ( cap & CAP_VS1V8 ) ) {
		hc->ocr			|= OCR_VDD_17_195;
		cur				= MCCAP_180( mccap ) * MCCAP_MULT;

		if( cur >= 150 )
			hc->caps	|= HC_CAP_XPC_1_8V;

		if( cur >= 800 )
			hc->caps 	|= HC_CAP_800MA;
		else if( cur >= 600 )
			hc->caps 	|= HC_CAP_600MA;
		else if( cur >= 400 )
			hc->caps 	|= HC_CAP_400MA;
		else
			hc->caps 	|= HC_CAP_200MA;
	}

	if( ( cap & CAP_VS3V0 ) ) {
		hc->ocr			|= OCR_VDD_30_31 | OCR_VDD_29_30;
		if( ( MCCAP_300( mccap ) * MCCAP_MULT ) > 150 ) {
			hc->caps	|= HC_CAP_XPC_3_0V;
		}
	}

	if( ( cap & CAP_VS3V3 ) ) {
		hc->ocr			|= OCR_VDD_32_33 | OCR_VDD_33_34;
		if( ( MCCAP_330( mccap ) * MCCAP_MULT ) > 150 ) {
			hc->caps	|= HC_CAP_XPC_3_3V;
		}
	}

	if( CAP_TIMER_COUNT( cap2 ) ) {
		if( CAP_TIMER_COUNT( cap2 ) != CAP_TIMER_OTHER ) {
			hc->tuning_count = 1 << ( CAP_TIMER_COUNT( cap2 ) - 1 );
		}
		else {
			// get information from other source
		}
	}

	mmchs->tuning_mode = CAP_TUNING_MODE( cap2 );

	hc->caps		|= HC_CAP_SV_3_3V;
	if( hc->version >= REV_SREV_V3 ) {
		hc->caps	|= HC_CAP_SV_1_8V;
	}

	if( ( hc->caps & HC_CAP_DMA ) ) {
		if( hc->version > REV_SREV_V1 && ( cap & CAP_AD2S ) &&
					( hwinfo & HWINFO_MADMA_EN ) ) {
			if( ( mmchs->adma = mmap( NULL, sizeof( omap_adma32_t ) * DMA_DESC_MAX,
					PROT_READ | PROT_WRITE | PROT_NOCACHE, 
					MAP_SHARED | MAP_ANON | MAP_PHYS, NOFD, 0 ) ) == MAP_FAILED ) {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: ADMA mmap %s", __FUNCTION__, strerror( errno ) );
				omap_dinit( hc );
				return( errno );
			}
#ifdef SDIO_OMAP_BUS_SYNC
			if(omap_so_init(hc) != EOK){
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:omap_so_init failed %s", __FUNCTION__, strerror( errno ) );
				omap_dinit( hc );
				return( errno );
			}
#endif
			mmchs->flags	|= OF_USE_ADMA;
			mmchs->admap	= sdio_vtop( mmchs->adma );
			if( hc->version >= REV_SREV_V3 ) {
				hc->caps	|= HC_CAP_ACMD23;
			}
			hc->cfg.sg_max	= DMA_DESC_MAX;
		}
		else {
			if( ( status = omap_dma_init( hc ) ) != EOK ) {
				omap_dinit( hc );
				return( status );
			}
			hc->cfg.sg_max	= DMA_DESC_MAX;
		}
	}

	hc->caps	&= cfg->caps;		// reconcile command line options

	SIGEV_PULSE_INIT( &event, hc->hc_coid, SDIO_PRIORITY, HC_EV_INTR, NULL );
	if( ( hc->hc_iid = InterruptAttachEvent( cfg->irq[0], &event, _NTO_INTR_FLAGS_TRK_MSK ) ) == -1 ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: InterrruptAttachEvent (irq 0x%x) - %s", __FUNCTION__, cfg->irq[0], strerror( errno ) );
		omap_dinit( hc );
		return( errno );
	}

	return( EOK );
}


//	Simple way to log errors
static void emit_message(const char *fmt, ... )
{
	va_list arglist;
	va_start(arglist, fmt);
	vslogf(_SLOGC_SDIODI, _SLOG_ERROR, fmt, arglist);
	va_end(arglist);
}


//
//	EDMA3 PaRAM set - Parameters for a transfer.
//
//	Naming of the fields follows the terminology used in
//	the TI (Texas Instruments) document SPRUHF4C April 2012.
//
//	You'll probably need the above mentioned document to understand
//	the details, so I won't try to reproduce all the info in here.
//
//	The TI document calls these structures PaRAM sets.
//	I'll try to use the same terminology in my comments.
typedef struct {
	uint32_t opt; // Options
	uint32_t src; // Source address
	uint32_t bcnt_acnt; // Counts for 2nd(B) and 1st(A) dimensions
	uint32_t dst; // Destination
	uint32_t dstbidx_srcbidx; // Destination and source B increment
	uint32_t bcntrld_link; // BCNT reload value and link address
	uint32_t dstcidx_srccidx; // Destination and source C increment
	uint32_t ccnt; // Low 16 bits = CCNT Count for 3rd dimension
} edma3_param_t;


#define EDMA3_WINDOW_BASE 	0x49000000	// Default base address (for Jacinto5)
#define EDMA3_WINDOW_SIZE 	0x8000		// Size of the register window
#define EDMA3_PARAM_AREA_OFFSET	0x4000	// Offset to the PARAM area
#define EDMA3_PARAM_SET_SIZE	0x20	// Size of one PARAM set

//	Some registers are accessible both globally and via the shadow regions.
//
//	The shadow region 0 is used as much as possible, to minimize potential
//	conflicts with other EDMA users.
//
//	Global access to the relevant registers is within the address offset
//	range 0x1000 to 0x1094 from the base of the EDMA register window.
//
//	The shadow range 0 starts from offset 0x2000.
//
//	For ease of reference with TI documentation, the global base offset
//	of 0x1000 is included in the register definitions. This way the
//	numbers in the code exactly match the numbers in the TI documents.
//
//	This also means that I now have to subtract the extra 0x1000 from the
//	shadow region 0 start address. So here it is, 0x1000 less than the
//	actual documented value 0x2000.
#define EDMA3_SHADOW0_OFFSET	0x1000	// Adjusted offset to the shadow area 0

//	Register naming follows the terminology used in TI documentation.

//	First some global registers
#define EDMA3_DCHMAP0	0x0100	// Channel map 0 (first of the 64 registers)
//
#define EDMA3_EMR	0x0300	// Event missed
#define EDMA3_EMCR	0x0308	// Event missed clear

//	Then some shadowed registers (with the global offset included)
#define EDMA3_ER	0x1000	// Event register
#define EDMA3_ECR	0x1008	// Event clear
#define EDMA3_ESR	0x1010	// Event set
#define EDMA3_EECR	0x1028	// Event enable clear
#define EDMA3_EESR	0x1030	// Event enable set
#define EDMA3_SECR	0x1040	// Secondary event clear

//	A NULL link for PaRAM entries
#define EDMA3_NULL_LINK	0xffff


//
//	Clear the event enable bit and the event bit of the active channel.
//
static void disable_active_channel(const edma3_data_t* edma)
{
	uintptr_t ptr = edma->active_base;
	uint32_t mask = edma->active_mask;;

	out32(ptr + EDMA3_SHADOW0_OFFSET + EDMA3_EECR, mask); // Clear event enable
	out32(ptr + EDMA3_SHADOW0_OFFSET + EDMA3_ECR, mask);  // Clear the event
}


//
//	Set the event enable bit of the active channel.
//
static void enable_active_channel(const edma3_data_t* edma)
{
	uintptr_t ptr = edma->active_base;
	uint32_t mask = edma->active_mask;;

	out32(ptr + EDMA3_SHADOW0_OFFSET + EDMA3_EESR, mask); // Set event enable
}


//
//	Clear the event bit and some error bits of the active channel.
//
static void clear_active_channel(const edma3_data_t* edma)
{
	uintptr_t ptr = edma->active_base;
	uint32_t mask = edma->active_mask;;

	out32(ptr + EDMA3_SHADOW0_OFFSET + EDMA3_ECR, mask);  // Clear the event

	//	Clear some error conditions (possibly never needed)
	out32(ptr + EDMA3_SHADOW0_OFFSET + EDMA3_SECR, mask); // Clear secondary event
	out32(ptr + EDMA3_EMCR, mask); // Clear event missed
}


//
//	Get address of the PaRAM set of the active channel.
//
static edma3_param_t* get_active_param_address(edma3_data_t* edma)
{
	uintptr_t ptr = edma->base;
	unsigned channel = edma->active_channel;

	//	Use the actual hardware mapping from the channel mapping register
	unsigned channel_mapping = in32(ptr + EDMA3_DCHMAP0 + 4*channel);

	//	The PaRAM index is in bits 13-5, a 9 bit unsigned value
	unsigned param_index = (channel_mapping >> 5) & 0x1ff;
	unsigned offset = EDMA3_PARAM_AREA_OFFSET + EDMA3_PARAM_SET_SIZE*param_index;
	return (edma3_param_t*)(ptr + offset);
}


//
//	Setup a scatter receive (transfer from a device I/O port to memory buffers),
//	or a gather transmit (transfer from memory buffers to a device I/O port).
//	Works fine also with just one buffer, so there's no need to have
//	separate code for single buffer I/O.
//
//	Returns 0 if no errors were detected, nonzero otherwise.
//
//	Currently the only possible error is EINVAL - Invalid argument.
//
static int edma3_setup_transfer(
	edma3_data_t* edma,
	unsigned port,
	int receiving,
	unsigned block_size,
	const sdio_sge_t* sgp,
	unsigned fragment_count)
{
	//	The fragment count should not be more than the number of PaRAM sets
	//	available.
	//	We have one dedicated PaRAM set per channel, plus a configurable
	//	number of sequential extras.
	if (fragment_count > edma->extra_param_count + 1)
	{
		return EINVAL;
	}

	//	The block size should be a multiple of four, because the EDMA
	//	transfer is going to be done using a given number 4 byte words.
	if ((block_size & 3) != 0)
	{
		return EINVAL;
	}

	//	Channel and state depend on the transfer direction
	int channel;
	if (receiving)
	{
		channel = edma->receive_channel;
		edma->state = EDMA3_RECEIVING;
	}
	else {
		channel = edma->transmit_channel;
		edma->state = EDMA3_TRANSMITTING;
	}

	//	Channel specific fields.
	edma->active_channel = channel;	// The channel number
	edma->active_base = edma->base + 4*(channel >> 5); // Corresponding base address
	edma->active_mask = 1 << (channel & 0x1f); // Corresponding bit mask

	//	Clear some channel bits to get a clean initial state
	clear_active_channel(edma);

	//	Set up the fixed parts of the PaRAM set
	uint32_t opt = (channel << 12) | (1 << 2); // TCC = channel number, SYNCDIM = AB-Synchronized
	uint32_t bcnt_acnt = ((block_size/4) << 16) | 4; // Block size in words, Word size 4 bytes

	//	These too are fixed, but depend on the transfer direction.
	uint32_t dstbidx_srcbidx;
	uint32_t dstcidx_srccidx;
	if (receiving)
	{
		//	Destination addresses increment by 4 and by the block size.
		//	The source is fixed, so both source increments are set to zero.
		dstbidx_srcbidx = (4 << 16);
		dstcidx_srccidx = (block_size << 16);
	}
	else {
		//	Source addresses increment by 4 and by the block size.
		//	The destination is fixed, so both destination increments are set to zero.
		dstbidx_srcbidx = 4;
		dstcidx_srccidx = block_size;
	}

	//	Last index is one less than the count of scatter gather fragments.
	unsigned last_index = fragment_count-1;

	//	Link to the next PaRAM set.
	//	Links are byte offsets from the beginning of the PaRAM area.
	//	The first link is to the first extra PaRAM set.
	unsigned link = EDMA3_PARAM_SET_SIZE*(edma->first_extra_param_index);

	//	Loop through the scatter gather fragments setting up the PaRAM sets.
	//
	//	The PaRAM sets are used in sequence, starting with the dedicated PaRAM
	//	set of the channel, and then continuing with the extra sets
	//	sequentially one by one.
	//
	edma3_param_t* param = get_active_param_address(edma);	// Dedicated set
	unsigned ix;
	for (ix = 0; ix < fragment_count; ix++)
	{
		//	Get the fragment address and length in bytes
		unsigned address = sgp[ix].sg_address;
		unsigned length = sgp[ix].sg_count;

		//	Fragment length in blocks
		unsigned block_count = length / block_size;

		//	The length should be a multiple of block size.
		if (length != block_count*block_size)
		{
			return EINVAL;
		}

		//	Copy the "fixed" parts
		param->opt = opt;
		param->bcnt_acnt = bcnt_acnt;
		param->dstbidx_srcbidx = dstbidx_srcbidx;
		param->dstcidx_srccidx = dstcidx_srccidx;

		//	Set the source and the destination
		if (receiving)
		{
			param->src = port;
			param->dst = address;
		}
		else {
			param->src = address;
			param->dst = port;
		}

		//	The link, or for the last set, the terminating "null"
		if (ix == last_index)
			param->bcntrld_link = EDMA3_NULL_LINK;
		else
			param->bcntrld_link = link;

		//	The transfer length as a number of blocks
		param->ccnt = block_count;

		//	One set is now ready and done.
		//	Let's move on to the next.

		//	The next PaRAM set must be where the link points to.
		param = (edma3_param_t*)(edma->base + EDMA3_PARAM_AREA_OFFSET + link);

		//	The next "link" will be to the next sequential PaRAM set.
		//	There's no need to check for the link going too high, as the
		//	number of scatter gather fragments has already been checked.
		link += EDMA3_PARAM_SET_SIZE;
	}

	//	Enable the channel
	enable_active_channel(edma);

	return EOK;
}


//
//	Verify that the transfer has completed
//
static void edma3_verify_completion(edma3_data_t* edma)
{
	if (edma->state == EDMA3_IDLE)
		return;

	//
	//	According to some, it is possible that the SD/MMC controller
	//	signals transfer completion before the transfer has in fact
	//	completed.
	//
	//	The mmcsd driver has the following piece of code to handle
	//	this exceptional situation:
	//
	//	if (in32(base + DRA446_EDMA_ER) & in32(base + DRA446_EDMA_EER)) {
	//		int i=100;
	//		while (param->ccnt != 0 && i--){
	//			printf("%s(%d): %d %x \n", __func__, __LINE__, channel, param->ccnt);
	//			delay(1);
	//		}
	//	}
	//
	//	I slightly changed the if statement. The original checks
	//	if any of the edma channels from 0 to 31 or from 32 to 64
	//	has signaled an event that is also enabled.
	//	The checked range depends on the currently used channel.
	//
	//	I just check if our own channel has signaled an event.
	//	If a transfer was going on, we know that the event is enabled.
	//	Events on other channels should have no effect on our transfer.
	//
	uintptr_t ptr = edma->active_base;
	uint32_t mask = edma->active_mask;
	if (in32(ptr + EDMA3_SHADOW0_OFFSET + EDMA3_ER) & mask)
	{
		//	Check that the remaining transfer count is zero, and if not,
		//	wait a short while to let the transfer complete.
		unsigned wait_count = 0;
		volatile edma3_param_t* param = get_active_param_address(edma);
		while (param->ccnt != 0 && wait_count < 100)
		{
			delay(1);
			wait_count++;
		}

		//	I haven't seen this log message in my tests so far, but if
		//	it starts flooding the logs, some throttling should be added.
		if (wait_count)
		{
			emit_message("%s: Premature completion.", __FUNCTION__);
		}
	}
}


//
//	Cleanup at the end of a transfer (either receive or transmit)
//
static void edma3_transfer_done(edma3_data_t* edma)
{
	if (edma->state == EDMA3_IDLE)
		return;

	disable_active_channel(edma);

	edma->state = EDMA3_IDLE;
}


//
//	Cleanup a failed transfer (either receive or transmit)
//
static void edma3_transfer_failed(edma3_data_t* edma)
{
	//	Nothing much can be done, nor need to be done.
	//	Just call the normal "transfer done" for now.
	edma3_transfer_done(edma);
}


//
//	Get and superficially check the edma related command line options.
//	Returns 0 if no obvious errors were detected, nonzero otherwise.
//
static int get_edma_options(edma3_data_t* edma, const struct _sdio_hc_cfg* cfg)
{
	int first = 0;
	int count = 0;

	//	At least three dma options are required.
	//	This test is redundant as we know that it'll always pass.
	//	Edma handling is triggered by the number of dma "channels"
	if (cfg->dma_chnls < 3)
	{
		emit_message("%s: Missing extra dma options");
		return EINVAL;
	}

	//	dma channel 2 is the first PaRAM set index
	first = cfg->dma_chnl[2];

	//	If present, dma channel 3 is the PaRAM set count.
	//	If not given, the count is taken to be zero.
	if (cfg->dma_chnls >= 4)
	{
		count = cfg->dma_chnl[3];
	}

	//	Negative numbers aren't accepted
	if (first < 0 || count < 0)
	{
		emit_message("%s: Negative extra dma option");
		return EINVAL;
	}

	//	Add more checks in here if you like.
	//	Currently I'm happy to have two non negative integers.

	edma->first_extra_param_index = first;
	edma->extra_param_count = count;

	return 0;
}


//
//	Initialize the EDMA3 setup
//
static int edma3_init(const sdio_hc_t *hc)
{
	static const char* fn = __FUNCTION__;	// Function name for messages

	int err = 0;
	unsigned base_address = EDMA3_WINDOW_BASE; // Default base address
	omap_hc_mmchs_t* mmchs = (omap_hc_mmchs_t *)hc->cs_hdl;
	const sdio_hc_cfg_t* cfg = &hc->cfg;
	edma3_data_t* edma = &mmchs->edma3;

	edma->base = MAP_DEVICE_FAILED;
	edma->state = EDMA3_IDLE;

	//	There are no defaults for the DMA channels, so they must be given
	//	in the command line. Report an error if they are missing.
	if (cfg->dma_chnls < 2)
	{
		emit_message("%s: Two DMA channels are required.", fn);
		return EINVAL;
	}

	edma->receive_channel = cfg->dma_chnl[0];
	edma->transmit_channel = cfg->dma_chnl[1];

	emit_message(
		"%s: Channel for receive: %d, for transmit: %d",
		fn, edma->receive_channel, edma->transmit_channel
	);

	//	Need to implement centralized handling for the system wide EDMA resources.
	//	With the current fixed setup it's all too easy to create situations
	//	where multiple processes are using the EDMA in conflicting ways.
	//
	//	"hardware/devb/mmcsd/arm/jacinto5teb.le.v7/sim_bs.c"
	// 	has the following explanation about the current approach:
	//
	//	/*There are 512 PaRam set, 0-63 are used for each EDMA channel, and 64-127
	//	* can be used for ping-pong buffer such as Audio driver, it will depends on how
	//	* QDMA channel is used, the following assume that we are going to use
	//	* 128 scatter gather list (each list need one PaRam set), and we start from PaRam
	//	* set 128. Then 256 - 511 are free.
	//	*/
	//
	//	The block size is usually 512 bytes. Max single I/O is probably 64 KiB.
	//	The maximum number of PaRAM sets would then be one per block, or
	//	128 in total. The first PaRAM set is always the channel's own dedicated
	//	set, so not more than 127 extra sets should ever be needed.
	//

	//	We need one or two extra dma options from the command line.
	if (get_edma_options(edma, cfg))
	{
		return EINVAL;
	}

	emit_message(
		"%s: First extra PaRAM index: %d, count: %d",
		fn, edma->first_extra_param_index, edma->extra_param_count
	);

	//	Handle the register window base address at the very end.
	//	This way the memory mapping won't need to be undone in case of errors.

	//	Use a default address if the command line didn't specify one.
	if (cfg->base_addrs > 1)
		base_address = cfg->base_addr[1];

	emit_message("%s: Using base address: 0x%08x", fn, base_address);

	//	Always use the default size
	edma->base = mmap_device_io(EDMA3_WINDOW_SIZE, base_address);
	if (edma->base == MAP_DEVICE_FAILED)
	{
		err = errno;
		emit_message("%s: mmap_device_io() errno: %d %s", fn, err, strerror(err));
		return err;
	}

	emit_message("%s: Base mapped to: 0x%08x", fn, edma->base);

	return err;
}


static int edma3_dinit(edma3_data_t* edma)
{
	//	If a channel is active, disable it, and hope for the best.
	//
	//	Actually there really isn't any documented way to stop an EDMA
	//	transfer. Just sleeping a short time might be the best option.
	//	It would simply allow the transfer to finish on it's own.
	//
	//
	if (edma->state != EDMA3_IDLE)
	{
		disable_active_channel(edma);
	}

	if (edma->base && edma->base != MAP_DEVICE_FAILED)
	{
		munmap_device_io(edma->base, EDMA3_WINDOW_SIZE);
	}

	return 0;
}

#endif


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/sdiodi/hc/omap.c $ $Rev: 805416 $")
#endif

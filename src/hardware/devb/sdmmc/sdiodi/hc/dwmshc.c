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


// Samsung Questions:
//  Describe UHS/HS200 tuning
//  Describe voltage switching
//	Describe AUTO_STOP failure 
//	Can we assume IDMAC xfer has completed when DTO is set?


#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <hw/inout.h>
#include <sys/mman.h>
#include <sys/rsrcdbmgr.h>

#include <internal.h>

#ifdef SDIO_HC_SYNOPSYS

//#define DW_DEBUG

#include <dwmshc.h>

static int dw_pwr( sdio_hc_t *hc, int vdd );
static int dw_drv_type( sdio_hc_t *hc, int type );
static int dw_bus_mode( sdio_hc_t *hc, int bus_mode );
static int dw_bus_width( sdio_hc_t *hc, int width );
static int dw_clk( sdio_hc_t *hc, int clk );
static int dw_timing( sdio_hc_t *hc, int timing );
static int dw_signal_voltage( sdio_hc_t *hc, int signal_voltage );
static int dw_cd( sdio_hc_t *hc );
static int dw_pm( sdio_hc_t *hc, int op );
static int dw_cmd( sdio_hc_t *hc, sdio_cmd_t *cmd );
static int dw_abort( sdio_hc_t *hc, sdio_cmd_t *cmd );
static int dw_event( sdio_hc_t *hc, sdio_event_t *ev );
static int dw_tune( sdio_hc_t *hc, int op );
static int dw_preset( sdio_hc_t *hc, int enable );

static sdio_hc_entry_t dw_hc_entry =	{ 16,
										dw_dinit, NULL /* dw_pm */,
										dw_cmd, dw_abort,
										dw_event, dw_cd, dw_pwr,
										dw_clk, dw_bus_mode,
										dw_bus_width, dw_timing,
										dw_signal_voltage, dw_drv_type,
										NULL, dw_tune, dw_preset,
										};

#ifdef DW_DEBUG
static int dw_idma_dump( sdio_hc_t *hc, const char *func )
{
	dw_hc_msh_t			*msh;
	dw_idmac_desc_t		*idma;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	idma	= (dw_idmac_desc_t *)msh->idma;

	do {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s (%s): des0 0x%x, des1 0x%x, des2 0x%x, des3 0x%x", __FUNCTION__, func, idma->des0, idma->des1, idma->des2, idma->des3 );
		idma++;
	} while( idma->des0 );

	return( EOK );
}
#endif

static int dw_reg_dump( sdio_hc_t *hc, const char *func, int line )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: line %d", func, line );
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: CTRL %x, PWREN %x, CLKDIV %x, CLKSRC %x, CLKENA %x, TMOUT %x, CTYPE %x",
		__FUNCTION__, 
		in32( base + DW_CTRL ),
		in32( base + DW_PWREN ),
		in32( base + DW_CLKDIV ),
		in32( base + DW_CLKSRC ),
		in32( base + DW_CLKENA ),
		in32( base + DW_TMOUT ),
		in32( base + DW_CTYPE ) );

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: BLKSIZ %x, BYTCNT %x, INTMASK %x, CMDARG %x, CMD %x, MINTSTS %x, RINTSTS %x",
		__FUNCTION__, 
		in32( base + DW_BLKSIZ ),
		in32( base + DW_BYTCNT ),
		in32( base + DW_INTMASK ),
		in32( base + DW_CMDARG ),
		in32( base + DW_CMD ),
		in32( base + DW_MINTSTS ),
		in32( base + DW_RINTSTS ) );

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: RESP0 %x, RESP1 %x, RESP2 %x, RESP3 %x, STATUS %x, FIFOTH %x, CDETECT %x",
		__FUNCTION__, 
		in32( base + DW_RESP0 ),
		in32( base + DW_RESP1 ),
		in32( base + DW_RESP2 ),
		in32( base + DW_RESP3 ),
		in32( base + DW_STATUS ),
		in32( base + DW_FIFOTH ),
		in32( base + DW_CDETECT ) );

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: WRTPRT %x, GPIO %x, TCBCNT %x, TBBCNT %x, DEBNCE %x, USRID %x, VERID %x",
		__FUNCTION__, 
		in32( base + DW_WRTPRT ),
		in32( base + DW_GPIO ),
		in32( base + DW_TCBCNT ),
		in32( base + DW_TBBCNT ),
		in32( base + DW_DEBNCE ),
		in32( base + DW_USRID ),
		in32( base + DW_VERID ) );

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: HCON %x, UHS_REG %x, BMOD %x, PLDMND %x, DBADDR %x, IDSTS %x, IDINTEN %x, DSCADDR %x, BUFADDR %x",
		__FUNCTION__, 
		in32( base + DW_HCON ),
		in32( base + DW_UHS_REG ),
		in32( base + DW_BMOD ),
		in32( base + DW_PLDMND ),
		in32( base + DW_DBADDR ),
		in32( base + DW_IDSTS ),
		in32( base + DW_IDINTEN ),
		in32( base + DW_DSCADDR ),
		in32( base + DW_BUFADDR ) );

	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: CLKSEL %x, CARDTHRCTL %x, BACKEND PWR %x, EMMC DDR REG %x, DDR200 RDDQS EN %x, DDR200 ASYNC FIFO CTRL %x",
		__FUNCTION__, 
		in32( base + DW_CLKSEL ),
		in32( base + DW_CARDTHRCTL ),
		in32( base + DW_BACK_END_POWER ),
		in32( base + DW_EMMC_DDR_REG ),
		in32( base + DW_DDR200_RDDQS_EN ),
		in32( base + DW_DDR200_ASYNC_FIFO_CTRL ) );

	return( EOK );
}

static int dw_waitmask( sdio_hc_t *hc, uint32_t reg, uint32_t mask, uint32_t val, uint32_t usec )
{
	dw_hc_msh_t			*msh;
	uintptr_t			base;
	uint32_t			cnt;
	int					stat;
	int					rval;
	uint32_t			iter;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;
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

#ifdef DW_DEBUG
	if( !cnt ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: reg %x, mask %x, val %x, rval %x", __FUNCTION__, reg, mask, val, in32( base + reg ) ); 
	}
#endif

	return( stat );
}

static int dw_set_ldo( sdio_hc_t *hc, int ldo, uint32_t voltage )
{
	int				status;

#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  ldo %s, voltage %d", __FUNCTION__, ( ldo == SDIO_LDO_VCC ) ? "VCC" : "VCC_IO", voltage );
#endif

	if( ( status = bs_set_ldo( hc, ldo, voltage ) ) != EOK ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: bs_set_ldo (status %d)", __FUNCTION__, status );
	}

	return( status );
}

static int dw_pm( sdio_hc_t *hc, int pm )
{
#ifdef DW_DEBUG	
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:   pm %d", __FUNCTION__, pm );
#endif
	
	switch( pm ) {
		case PM_IDLE:
			bs_clock_gate( hc, SDIO_TRUE );
			break;

		case PM_ACTIVE:
			bs_clock_gate( hc, SDIO_FALSE );
			if( hc->pm_state == PM_SLEEP ) {
				dw_set_ldo( hc, SDIO_LDO_VCC, ( hc->vdd <= OCR_VDD_17_195 ) ? 1800 : 3000 );
			}
			break;

		case PM_SLEEP:
			dw_set_ldo( hc, SDIO_LDO_VCC, 0 );		// pwr off device
#ifdef BS_PAD_CONF
			bs_pad_conf( hc, SDIO_FALSE );
#endif
			bs_clock_gate( hc, SDIO_TRUE );
			break;

		default:
			break;
	}

	return( EOK );
}

int dw_cmd_wait( sdio_hc_t *hc, uint32_t cmd, uint32_t arg, uint32_t tms )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;

	msh			= (dw_hc_msh_t *)hc->cs_hdl;
	base		= msh->base;

	out32( base + DW_CMDARG, arg );
	out32( base + DW_CMD, cmd | DW_CMD_START );

	return( dw_waitmask( hc, DW_CMD, DW_CMD_START, 0, tms ) );
}

static int dw_reset( sdio_hc_t *hc, uint32_t rst, uint32_t flgs )
{
	dw_hc_msh_t			*msh;
	uintptr_t			base;
	int					stat;
	uint32_t			ctrl;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;

	out32( base + DW_RINTSTS, ~0 );

#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  rst %x", __FUNCTION__, rst );
#endif

	ctrl	= in32( base + DW_CTRL );

	out32( base + DW_CTRL, ( ctrl & ~DW_CTRL_INT_ENABLE ) | rst );
	if( ( stat = dw_waitmask( hc, DW_CTRL, rst, 0, 100000 ) ) ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  timeout", __FUNCTION__ );
	}

	out32( base + DW_RINTSTS, ~0 );		// clear pending interrupts
	out32( base + DW_CTRL, ctrl );

	if( flgs ) {					// update clk after CIU reset
		if( dw_cmd_wait( hc, DW_CMD_UPD_CLK | DW_CMD_PRV_DAT_WAIT, 0, 1000 ) ) {
		}
	}

	return( stat );
}

static inline int dw_pio_write( sdio_hc_t *hc, int fcnt, uint8_t *src, int slen )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;
	uint8_t			*sptr;
	int				olen;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;
	sptr	= src;

	while( slen && fcnt ) {
		olen = min( slen, fcnt );
		sptr = msh->douts( sptr, olen >> msh->dshft, base + DW_DATA );
		slen -= olen;
	}

	out32( base + DW_RINTSTS, DW_INT_TXDR );

	return( sptr - src );
}

static inline int dw_pio_read( sdio_hc_t *hc, int fcnt, uint8_t *dst, int dlen )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;
	void			*dptr;
	int				ilen;
	
	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;
	dptr	= (uint64_t *)dst;

	while( dlen && fcnt && !( in32( base + DW_STATUS ) & DW_STATUS_FIFO_EMPTY ) ) {
		ilen = min( dlen, fcnt );
		dptr = msh->dins( dptr, ilen >> msh->dshft, base + DW_DATA );
		dlen -= ilen; fcnt -= ilen;
	}

	return( (uint8_t *)dptr - dst );
}

static int dw_xfer_pio( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	dw_hc_msh_t			*msh;
	sdio_wspc_t			*wspc;
	uintptr_t			base;
	int					len;
	uint32_t			fcnt;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;
	wspc	= &hc->wspc;

	do {

#ifdef DW_DEBUG
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  STATUS 0x%x, fcnt %d, xlen %d", __FUNCTION__, in32( base + DW_STATUS ), DW_STATUS_FIFO_COUNT( in32( base + DW_STATUS ) ) << msh->dshft, msh->xlen );
#endif

		if( !msh->xlen ) {
			break;
		}

		fcnt = DW_STATUS_FIFO_COUNT( in32( base + DW_STATUS ) ) << msh->dshft;
		if( ( cmd->flags & SCF_DIR_IN ) ) {
			len = dw_pio_read( hc, fcnt, wspc->sga, wspc->sgc );
		}
		else {
			len = dw_pio_write( hc, msh->fifo_size - fcnt, wspc->sga, wspc->sgc );
		}

		msh->xlen	-= len;
		wspc->sga	+= len;
		wspc->sgc	-= len;

		if( !wspc->sgc && msh->xlen ) {	// advance to next sg entry
			if( --wspc->nsg ) {
				wspc->sge++;
				wspc->sgc	= wspc->sge->sg_count;
				wspc->sga	= SDIO_DATA_PTR_V( wspc->sge->sg_address );
			}
		}		
		out32( base + DW_RINTSTS, DW_INT_TXDR | DW_INT_RXDR );
	} while( ( in32( base + DW_MINTSTS ) & ( DW_INT_TXDR | DW_INT_RXDR ) ) );

	return( EOK );
}

static int dw_idma_stop( sdio_hc_t *hc )
{
	dw_hc_msh_t			*msh;
	uintptr_t			base;
	uint32_t			ctrl;
	uint32_t			bmod;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;

	ctrl	= in32( base + DW_CTRL ) & ~DW_CTRL_USE_IDMAC;
	out32( base + DW_CTRL, ctrl | DW_CTRL_DMA_RESET );

	bmod = in32( base + DW_BMOD ) & ~( DW_IDMAC_ENABLE | DW_IDMAC_FB );
	out32( base + DW_BMOD, bmod );

	return( EOK );
}

static int dw_intr_event( sdio_hc_t *hc )
{
	dw_hc_msh_t			*msh;
	sdio_cmd_t			*cmd;
	uint32_t			cs;
	uint32_t			sts;
	uint32_t			idsts;
	uintptr_t			base;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;

	cmd		= hc->wspc.cmd;
	cs		= CS_CMD_INPROG;

	sts		= in32( base + DW_MINTSTS );
	idsts	= in32( base + DW_IDSTS );

#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  MINTSTS 0x%x, IDSTS 0x%x", __FUNCTION__, sts, idsts );
#endif

	if( ( sts & DW_INT_CDET ) ) {
		sdio_hc_event( hc, HC_EV_CD );
	}

	out32( base + DW_RINTSTS, sts );

	if( cmd == NULL ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  cmd == NULL MINTSTS 0x%x, IDSTS 0x%x", __FUNCTION__, sts, idsts );
		dw_reg_dump( hc, __FUNCTION__, __LINE__ );
		return( EOK );
	}

	if( cmd->opcode == SD_VOLTAGE_SWITCH && ( sts & DW_INT_VOLT_SWITCH ) ) {
		sts &= ~DW_INT_VOLT_SWITCH;
		dw_reset( hc, DW_CTRL_RESET_ALL, SDIO_TRUE );		// reset CIU, FIFO, DMA
		cs = CS_CMD_CMP;
	}

		// Check of errors
	if( ( sts & DW_INT_ERRORS ) || ( idsts & DW_IDMAC_INT_AI ) ) {
		dw_reg_dump( hc, __FUNCTION__, __LINE__ );

		if( ( sts & DW_INT_DRTO ) )	cs = CS_DATA_TO_ERR;
		if( ( sts & DW_INT_DCRC ) )	cs = CS_DATA_CRC_ERR;
		if( ( sts & DW_INT_EBE ) )	cs = CS_DATA_END_ERR;
		if( !cs )					cs = CS_CMD_CMP_ERR;

		if( ( msh->flags & MF_XFER_DMA ) ) {
			if( ( idsts & DW_IDMAC_INT_AI ) ) {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  IDSTS 0x%x", __FUNCTION__, idsts );
			}
			out32( base + DW_IDSTS, idsts );

#ifdef DW_DEBUG
			dw_idma_dump( hc, __FUNCTION__ );
#endif
			dw_idma_stop( hc );
		}
		dw_reset( hc, DW_CTRL_RESET_ALL, SDIO_TRUE );	// reset CIU, FIFO, DMA
	}
	else {
		if( ( sts & DW_INT_CD ) && !( cmd->flags & SCF_DATA_MSK ) ) {
			if( ( cmd->flags & SCF_RSP_136 ) ) {
				cmd->rsp[0] = in32( base + DW_RESP3 );
				cmd->rsp[1] = in32( base + DW_RESP2 );
				cmd->rsp[2] = in32( base + DW_RESP1 );
				cmd->rsp[3] = in32( base + DW_RESP0 );
			}
			else if( ( cmd->flags & SCF_RSP_PRESENT ) ) {
				cmd->rsp[0] = in32( base + DW_RESP0 );
			}
			cs = CS_CMD_CMP;
		}

		if( ( sts & ( DW_INT_RXDR | DW_INT_TXDR ) ) ) {
			cs = dw_xfer_pio( hc, cmd );
		}

		if( ( sts & DW_INT_DTO ) ) {			// Data Transfer Over
			cs = CS_CMD_CMP;
			cmd->rsp[0] = in32( base + DW_RESP0 );

			if( ( msh->flags & MF_XFER_DMA ) ) {
					// wait for IDMAC to complete/idle
				if( dw_waitmask( hc, DW_IDSTS, DW_IDSTS_FSM_MSK, DMAC_FSM_DMA_IDLE, 10000 ) ) {
					cs = CS_DATA_TO_ERR;
				}
				dw_idma_stop( hc );
			}
			else {
				// While reading in PIO mode DTO can be set while
				// the FIFO still has data.  This can happen when the
				// remaining data is < the FIFO threshold watermark.
				if( ( cmd->flags & SCF_DIR_IN ) && msh->xlen ) {
					dw_xfer_pio( hc, cmd );
				}
			}
		}

		if( ( sts & DW_INT_ACD ) ) {		// check for Auto Command Done
			cmd->rsp[0] = in32( base + DW_RESP1 );	// AUTO_STOP command resp
			cs = CS_CMD_CMP;
		}
	}

	if( cs != CS_CMD_INPROG ) {
#ifdef DW_DEBUG
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  cmd cmplt", __FUNCTION__ );
#endif
		msh->flags &= ~MF_XFER_DMA;

		if( cs == CS_CMD_CMP && ( in32( base + DW_INTMASK ) & DW_INT_ACD ) ) {
			if( !( sts & DW_INT_ACD ) ) {	// wait until Auto Command Done
				return( EOK );
			}
		}

		sdio_cmd_cmplt( hc, cmd, cs );
	}

	return( EOK );
}

static int dw_event( sdio_hc_t *hc, sdio_event_t *ev )
{
	int		status;

	switch( ev->code ) {
		case HC_EV_INTR:
			status = dw_intr_event( hc );
			InterruptUnmask( hc->cfg.irq[0], hc->hc_iid );
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

static int dw_idmac_setup( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	dw_hc_msh_t			*msh;
	dw_idmac_desc_t		*idma;
	sdio_sge_t			*sgp;
	int					sgc;
	int					sgi;
	int					acnt;
	int					alen;
	uintptr_t			base;
	int					sg_count;
	paddr_t				sg_paddr;
	paddr_t				idmap;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	idma	= (dw_idmac_desc_t *)msh->idma;
	base	= msh->base;
	idmap	= msh->idmap;

	sgc = cmd->sgc;
	sgp = cmd->sgl;

	if( !( cmd->flags & SCF_DATA_PHYS ) ) {
		sdio_vtop_sg( sgp, msh->sgl, sgc, cmd->mhdl );
		sgp = msh->sgl;
	}

	for( sgi = 0, acnt = 0; sgi < sgc; sgi++, sgp++ ) {
		sg_count	= sgp->sg_count;
		sg_paddr	= sgp->sg_address;
		while( sg_count ) {
			alen		= min( sg_count, IDMA_MAX_XFER );
			idmap		+= sizeof( dw_idmac_desc_t );
			idma->des0	= IDMAC_DES0_OWN | IDMAC_DES0_DIC | IDMAC_DES0_CH;
			idma->des1	= alen;
			idma->des2	= sg_paddr;
			idma->des3	= idmap;

			sg_count	-= alen; 
			sg_paddr	+= alen;
			idma++;

			if( ++acnt > DW_DMA_DESC_MAX ) {
				return( ENOTSUP );
			}
		}
	}

	idma->des0 = 0;				// temp for debug

	idma--;
	idma->des0	= IDMAC_DES0_OWN | IDMAC_DES0_DIC | IDMAC_DES0_LD;
	idma->des3	= 0;

		// set first descriptor
	msh->idma->des0	|= IDMAC_DES0_FD;

	out32( base + DW_DBADDR, msh->idmap );
	out32( base + DW_BMOD, in32( base + DW_BMOD ) | DW_IDMAC_ENABLE | DW_IDMAC_FB );
	out32( base + DW_PLDMND, DW_PLDMND_PD );

	return( EOK );
}

static int dw_xfer_setup( sdio_hc_t *hc, sdio_cmd_t *cmd, uint32_t *command, uint32_t *imask )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;
	uint32_t		ctrl;
	int				status;

#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  %s, blksz %d, nblks %d", __FUNCTION__, ( cmd->flags & SCF_DIR_IN ) ? "in" : "out", cmd->blksz, cmd->blks );
#endif

	msh			= (dw_hc_msh_t *)hc->cs_hdl;
	base		= msh->base;
	status		= EOK;
	*imask		|= DW_INT_DTO;			// data transfer over interrupt
	*command	|= ( cmd->flags & SCF_DIR_IN ) ? DW_CMD_DAT_EXP : ( DW_CMD_DAT_EXP | DW_CMD_DAT_WR );
	ctrl		= in32( base + DW_CTRL ) & ~( DW_CTRL_USE_IDMAC | DW_CTRL_DMA_ENABLE );


	sdio_sg_start( hc, cmd->sgl, cmd->sgc );

	if( ( hc->caps & HC_CAP_DMA ) ) {
		if( ( status = dw_idmac_setup( hc, cmd ) ) == EOK ) {
			ctrl |= ( DW_CTRL_USE_IDMAC | DW_CTRL_DMA_ENABLE );
			msh->flags |= MF_XFER_DMA;
		}
	}

	if( status || !( hc->caps & HC_CAP_DMA ) ) {	// use PIO
		if( !( hc->caps & HC_CAP_PIO ) || ( cmd->flags & SCF_DATA_PHYS ) ) {
			return( ENOTSUP );
		}
		msh->xlen	= cmd->blksz * cmd->blks;
		*imask		|= ( cmd->flags & SCF_DIR_IN ) ? DW_INT_RXDR : DW_INT_TXDR;
	}

	if( cmd->blks > 1 ) {
		if( ( hc->caps & HC_CAP_ACMD12 ) ) {
			*imask		|= DW_INT_ACD;
			*command	|= DW_CMD_SEND_STOP;
		}
	}

	out32( base + DW_CTRL, ctrl );
	out32( base + DW_BYTCNT, cmd->blksz * cmd->blks );
	out32( base + DW_BLKSIZ, cmd->blksz );

	return( EOK );
}

static int dw_cmd( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;
	uint32_t		imask;
	int				status;
	uint32_t		command;

	msh			= (dw_hc_msh_t *)hc->cs_hdl;
	base		= msh->base;
	imask		= DW_INT_DFLTS;
	cmd->status	= CS_CMD_INPROG;

#ifdef BS_POWMAN
	bs_powman( hc, POWMAN_ALIVE );
#endif

	out32( base + DW_RINTSTS, ~0 );			// Clear Status

	command	= cmd->opcode | DW_CMD_USE_HOLD_REG | DW_CMD_START;

	if( ( msh->flags & MF_SEND_INIT_STREAM ) ) {
		msh->flags &= ~MF_SEND_INIT_STREAM;
		command |= DW_CMD_INIT;
	}

	if( cmd->opcode == SD_VOLTAGE_SWITCH ) {
		command |= DW_CMD_VOLT_SWITCH;
	}

	if( cmd->opcode == MMC_STOP_TRANSMISSION ) {
		command |= DW_CMD_STOP;
	}
	else {
		command |= DW_CMD_PRV_DAT_WAIT;
	}

	if( ( cmd->flags & SCF_DATA_MSK ) ) {
		if( ( status = dw_xfer_setup( hc, cmd, &command, &imask ) ) != EOK ) {
			return( status );
		}
	}
	else {
		imask |= DW_INT_CD;				// Enable command done int
	}

	if( ( cmd->flags & SCF_RSP_PRESENT ) ) {
		command |= DW_CMD_RESP_EXP;

		if( ( cmd->flags & SCF_RSP_136 ) ) {
			command |= DW_CMD_RESP_LONG;
		}

		if( ( cmd->flags & SCF_RSP_CRC ) ) {		// CRC check
			command |= DW_CMD_RESP_CRC;
		}
	}

	out32( base + DW_INTMASK, imask );
	out32( base + DW_CMDARG, cmd->arg );
	out32( base + DW_CMD, command );

	return( EOK );
}

static int dw_abort( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	dw_idma_stop( hc );
	dw_reset( hc, DW_CTRL_RESET_ALL, SDIO_TRUE );	// reset CIU, FIFO, DMA
	delay( 2 );							// allow pending events to complete

	return( EOK );
}

static int dw_pwr( sdio_hc_t *hc, int vdd )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;

#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  vdd 0x%x", __FUNCTION__, vdd );
#endif

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;

	dw_pm( hc, PM_ACTIVE );

	out32( base + DW_IDINTEN, 0 );				// disable intrs

	if( !vdd ) {
		dw_reset( hc, DW_CTRL_RESET_ALL, SDIO_FALSE );	// reset CIU, FIFO, DMA

		if( msh->pbase != DW_MSH0_BASE ) {			// don't turn off eMMC
			dw_set_ldo( hc, SDIO_LDO_VCC, 0 );		// pwr off device
			dw_set_ldo( hc, SDIO_LDO_VCC_IO, 0 );
#ifdef BS_PAD_CONF
			bs_pad_conf( hc, SDIO_FALSE );
#endif
		}
		dw_pm( hc, PM_IDLE );
	}
	else {
		msh->flags |= MF_SEND_INIT_STREAM;

#ifdef BS_PAD_CONF
		bs_pad_conf( hc, SDIO_TRUE );
#endif

		dw_set_ldo( hc, SDIO_LDO_VCC, ( vdd <= OCR_VDD_17_195 ) ? 1800 : 3000 );
		dw_set_ldo( hc, SDIO_LDO_VCC_IO, 3000 );

		out32( base + DW_CTRL, DW_CTRL_INT_ENABLE );	// enable intrs
	}

	hc->vdd = vdd;

	return( EOK );
}

static int dw_bus_mode( sdio_hc_t *hc, int bus_mode )
{
#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  bus_mode %d", __FUNCTION__, bus_mode );
#endif

	if( bus_mode == BUS_MODE_OPEN_DRAIN ) {
	}

	hc->bus_mode = bus_mode;

	return( EOK );
}

static int dw_bus_width( sdio_hc_t *hc, int width )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;
	uint32_t		ctype;

#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  width %d", __FUNCTION__, width );
#endif

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;
	ctype	= 0;

	switch( width ) {
		case BUS_WIDTH_8:
			ctype |= DW_CTYPE_8BIT; break;
		case BUS_WIDTH_4:
			ctype |= DW_CTYPE_4BIT; break;
		case BUS_WIDTH_1:
		default:
			ctype |= DW_CTYPE_1BIT;
			break;
	}

	out32( base + DW_CTYPE, ctype );

	hc->bus_width = width;

	return( EOK );
}

static int dw_clk( sdio_hc_t *hc, int clk )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;
	uint32_t		div;
	uint32_t		clkd;
	uint32_t		clkena;

#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  clk %d", __FUNCTION__, clk );
#endif

	msh			= (dw_hc_msh_t *)hc->cs_hdl;
	base		= msh->base;

	for( clkd = 0, div = 1; clkd <= 0xFF; ) {
		if( ( ( msh->sclk / msh->divratio ) / div ) <= clk ) {
			break;
		}
		div = ++clkd * 2;
	}

		// disable card clock generation
	out32( base + DW_CLKENA, 0 );
	out32( base + DW_CLKSRC, 0 );
	if( dw_cmd_wait( hc, DW_CMD_UPD_CLK | DW_CMD_PRV_DAT_WAIT, 0, 1000 ) ) {
	}

	out32( base + DW_CLKDIV, clkd );
	if( dw_cmd_wait( hc, DW_CMD_UPD_CLK | DW_CMD_PRV_DAT_WAIT, 0, 1000 ) ) {
	}

	clkena = ( DW_CLKEN_ENABLE << msh->cardno ) | ( DW_CLKEN_LOW_PWR << msh->cardno );
	out32( base + DW_CLKENA, clkena );
	if( dw_cmd_wait( hc, DW_CMD_UPD_CLK | DW_CMD_PRV_DAT_WAIT, 0, 1000 ) ) {
	}

	hc->clk = ( msh->sclk / msh->divratio ) / div;

	return( EOK );
}

static int dw_timing( sdio_hc_t *hc, int timing )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;
	uint32_t		uhs;
	uint32_t		clksel;

#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: timing %d", __FUNCTION__, timing );
#endif

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;

	clksel	= msh->sdr;
	uhs		= in32( base + DW_UHS_REG ) & ~DW_DDR( msh->cardno );

	switch( timing ) {
		case TIMING_HS200:
			clksel	= msh->sdr;
			break;

		case TIMING_DDR50:
			clksel	= msh->ddr;
			uhs		|= DW_DDR( msh->cardno );
			break;

		case TIMING_SDR104:
		case TIMING_SDR50:
		case TIMING_SDR25:
		case TIMING_SDR12:
			clksel	= msh->sdr;
			uhs		|= DW_VOLT_1_8( msh->cardno );
			break;

		case TIMING_HS:
		case TIMING_LS:
			clksel	= msh->sdr;
			break;

		default:
			break;
	}

	out32( base + DW_UHS_REG, uhs );

	if( DW_VERID_CREV( hc->version ) >= DW_VERID_CREV_240A ) {
		out32( base + DW_CLKSEL, clksel );
	}

	hc->timing = timing;

	return( EOK );
}

static int dw_signal_voltage( sdio_hc_t *hc, int signal_voltage )
{
	dw_hc_msh_t			*msh;
	uintptr_t			base;
	uint32_t			uhs;
	uint32_t			clkena;
	int					status;

#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s:  signal voltage %d", __FUNCTION__, signal_voltage );
#endif

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	status	= EOK;
	base	= msh->base;
	uhs		= in32( base + DW_UHS_REG );

	if( signal_voltage == SIGNAL_VOLTAGE_1_8 ) {
		clkena = in32( base + DW_CLKENA ) & ~( ( DW_CLKEN_ENABLE | DW_CLKEN_LOW_PWR ) << msh->cardno );
		out32( base + DW_CLKENA, clkena );
		if( dw_cmd_wait( hc, DW_CMD_UPD_CLK | DW_CMD_PRV_DAT_WAIT, 0, 1000 ) ) {
		}

		if( ( status = dw_set_ldo( hc, SDIO_LDO_VCC_IO, 1800 ) ) == EOK ) {
			out32( base + DW_UHS_REG, uhs | DW_VOLT_1_8( msh->cardno ) );
		}

		delay( 5 );

		clkena = in32( base + DW_CLKENA ) | ( DW_CLKEN_ENABLE << msh->cardno ) | ( DW_CLKEN_LOW_PWR << msh->cardno );
		out32( base + DW_CLKENA, clkena );
		if( dw_cmd_wait( hc, DW_CMD_UPD_CLK | DW_CMD_PRV_DAT_WAIT, 0, 1000 ) ) {
		}
	}
	else if( signal_voltage == SIGNAL_VOLTAGE_3_3 ) {
		if( hc->vdd > OCR_VDD_17_195 ) {
			uhs &= ~DW_VOLT_1_8( msh->cardno );
			if( ( status = dw_set_ldo( hc, SDIO_LDO_VCC_IO, 3000 ) ) == EOK ) {
				out32( base + DW_UHS_REG, uhs );
			}
			delay( 5 );
		}
	}
	else {
		return( EINVAL );
	}

	hc->signal_voltage = signal_voltage;

	return( status );
}

static int dw_drv_type( sdio_hc_t *hc, int type )
{
#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: type %d", __FUNCTION__, type );
#endif

	hc->drv_type = type;

	return( EOK );
}

static int dw_median_sample( uint8_t map, uint8_t bit )
{
	uint8_t			_map;
	uint8_t			pattern;
	int				sample;
	const uint8_t	patterns[9] = { 0, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };

	pattern = DW_ROR8( patterns[bit], bit / 2 );

	for( sample = 0; sample < DW_MS_ITER; sample++ ) {
		_map = DW_ROR8( map, sample );
		if( ( _map & pattern ) == pattern ) {
			return( sample );
		}
	}

	return( -1 );
}

static int dw_tune( sdio_hc_t *hc, int op )
{
	dw_hc_msh_t		*msh;
	sdio_cmd_t		*cmd;
	sdio_sge_t		sge;
	int				status;
	const uint8_t	*tbp;
	uint8_t			*tbd;
	uint8_t			map;
	uint32_t		tbl;
	uintptr_t		base;
	uint32_t		bits;
	uint32_t		clksel;
	uint32_t		divratio;
	int				sample;

#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: ", __FUNCTION__ );
#endif

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;
	status	= EOK;
	map		= 0;

	switch( hc->timing ) {
		case TIMING_HS200:		// only HS200/SDR104/SDR50 require tuning
		case TIMING_SDR104:
		case TIMING_SDR50:
			break;

		default:
			return( EOK );
	}

	if( op == MMC_SEND_TUNING_BLOCK && hc->bus_width == BUS_WIDTH_8 ) {
		tbl = 128; tbp = sdio_tbp_8bit;
	}
	else {
		tbl = 64; tbp = sdio_tbp_4bit;
	}

	divratio = ( msh->sdr >> DW_DIVRATIO_SHFT ) & DW_DIVRATIO_MSK;
	if( divratio != 1 && divratio != 3 ) {
		return( EIO );
	}
	
	bits = divratio + 2;

	if( ( cmd = sdio_alloc_cmd( ) ) == NULL ) {
		return( ENOMEM );
	}

	if( ( tbd = sdio_alloc( tbl ) ) == NULL ) {
		sdio_free_cmd( cmd );
		return( ENOMEM );
	}

	clksel = in32( base + DW_CLKSEL ) & ~DW_CLK_SMPL_MSK;
	out32( base + DW_CARDTHRCTL, DW_CARDTHRCTL_THRES( 512 ) | DW_CARDTHRCTL_EN );

	for( sample = 0; sample <= DW_CLK_SMPL_MAX; sample++ ) {
		out32( base + DW_CLKSEL, clksel | sample );
		sdio_setup_cmd( cmd, SCF_CTYPE_ADTC | SCF_RSP_R1, op, 0 );
		sge.sg_count = tbl; sge.sg_address = (paddr_t)tbd;
		sdio_setup_cmd_io( cmd, SCF_DIR_IN, 1, tbl, &sge, 1, NULL );
		if( ( status = sdio_issue_cmd( &hc->device, cmd, DW_TUNING_TIMEOUT ) ) == EOK ) {
			if( !memcmp( tbp, tbd, tbl ) ) {
				map |= ( 1 << sample );
			}
		}
	}

	if( ( sample = dw_median_sample( map, bits ) ) < 0 ) {
		out32( base + DW_CARDTHRCTL, 0 );
		out32( base + DW_CLKSEL, clksel );
		status = ESTALE;
	}
	else {
		out32( base + DW_CLKSEL, clksel | sample );
	}

	sdio_free( tbd, tbl );
	sdio_free_cmd( cmd );

	return( status );
}

static int dw_preset( sdio_hc_t *hc, int enable )
{
#ifdef DW_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: ", __FUNCTION__ );
#endif

	return( EOK );
}

static int dw_cd( sdio_hc_t *hc )
{
	dw_hc_msh_t		*msh;
	uintptr_t		base;
	int				cstate;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;

	cstate	= CD_RMV;

	if( ( hc->caps & HC_CAP_SLOT_TYPE_EMBEDDED ) ) {
		cstate  |= CD_INS;
	}
	else if( !( in32( base + DW_CDETECT ) & ( 1 << msh->cardno ) ) ) {
		cstate  |= CD_INS;
		if( ( in32( base + DW_WRTPRT ) & ( 1 << msh->cardno ) ) ) {
			cstate  |= CD_WP;
		}
	}

	return( cstate );
}

int dw_dinit( sdio_hc_t *hc )
{
	dw_hc_msh_t		*msh;

	if( ( msh = (dw_hc_msh_t *)hc->cs_hdl ) == NULL ) {
		return( EOK );
	}

	if( msh->base ) {
		dw_pwr( hc, 0 );
		if( hc->hc_iid != -1 ) {
			InterruptDetach( hc->hc_iid );
		}
		munmap_device_io( msh->base, DW_MSH_SIZE );
	}

	if( msh->idma ) {
		munmap( msh->idma, sizeof( dw_idmac_desc_t ) * DW_DMA_DESC_MAX );
	}

	free( msh );
	hc->cs_hdl = NULL;

	return( EOK );
}

int dw_idmac_init( sdio_hc_t *hc )
{	
	dw_hc_msh_t			*msh;
	int					n_idma;
	paddr_t				paddr;
	dw_idmac_desc_t		*idesc;

	msh	= (dw_hc_msh_t *)hc->cs_hdl;

	if( ( idesc = msh->idma = mmap( NULL, sizeof( dw_idmac_desc_t ) * DW_DMA_DESC_MAX,
			PROT_READ | PROT_WRITE | PROT_NOCACHE, 
			MAP_SHARED | MAP_ANON | MAP_PHYS, NOFD, 0 ) ) == MAP_FAILED ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: IDMA descriptor mmap %s", __FUNCTION__, strerror( errno ) );
		return( errno );
	}

	paddr = msh->idmap = sdio_vtop( msh->idma );

		// link descriptors
	for( n_idma = DW_DMA_DESC_MAX - 1; n_idma; n_idma--, idesc++ ) {
		paddr 		+= sizeof( dw_idmac_desc_t );
		idesc->des3	= paddr;
	}

	idesc->des0	= IDMAC_DES0_ER;

	out32( msh->base + DW_DBADDR, msh->idmap );

	return( EOK );
}

int dw_msh_init( sdio_hc_t *hc )
{
	dw_hc_msh_t			*msh;
	uintptr_t			base;
	uint32_t			fsize;

	msh		= (dw_hc_msh_t *)hc->cs_hdl;
	base	= msh->base;
	fsize	= msh->fifo_size >> msh->dshft;

//		do we need a card hardware reset
//	out32( base + DW_RST, in32( base + DW_RST ) & ~( DW_RST_ACTIVE << msh->cardno ) );
//	out32( base + DW_RST, in32( base + DW_RST ) | ( DW_RST_ACTIVE << msh->cardno ) );
//	out32( base + DW_PWREN, 0 );

	out32( base + DW_PWREN, DW_PWREN_POWER_ENABLE );
//	delay( 10 );								// Wait for the power ramp-up time
	dw_reset( hc, DW_CTRL_RESET_ALL, SDIO_FALSE );	// reset CIU, FIFO, DMA
	out32( base + DW_BMOD, DW_IDMAC_SWRESET );	// reset IDMAC
	out32( base + DW_INTMASK, 0 );
	out32( base + DW_RINTSTS, ~0 );
	out32( base + DW_IDINTEN, 0 );
	out32( base + DW_IDSTS, DW_IDMAC_INT_MSK );
	out32( base + DW_CTRL, DW_CTRL_INT_ENABLE );

		// set multiple transaction size, rx watermark,  tx watermark
	out32( base + DW_FIFOTH, DW_FIFOTH_MSIZE_32 | ( ( ( fsize / 2 ) - 1 ) << DW_FIFOTH_RX_WMARK_SHFT ) | ( fsize / 2 ) );

	out32( base + DW_CTYPE, DW_CTYPE_1BIT );
	out32( base + DW_DEBNCE, DW_DEBNCE_25MS );
	out32( base + DW_TMOUT, DW_TMOUT_DATA_MAX | DW_TMOUT_RESP_MAX );
	out32( base + DW_CARDTHRCTL, DW_CARDTHRCTL_EN | DW_CARDTHRCTL_THRES( 0x200 ) );

	return( EOK );
}

int dw_init( sdio_hc_t *hc )
{
	sdio_hc_cfg_t		*cfg;
	dw_hc_msh_t			*msh;
	uint32_t			hcon;
	uint32_t			verid;
	uint32_t			status;
	uintptr_t			base;
	struct sigevent		event;

	if( ( hc->cs_hdl = calloc( 1, sizeof( dw_hc_msh_t ) ) ) == NULL ) {
		return( ENOMEM );
	}

	cfg				= &hc->cfg;
	msh				= (dw_hc_msh_t *)hc->cs_hdl;
	msh->pbase		= cfg->base_addr[0];
	msh->sclk		= DW_SCLK;
	hc->hc_iid		= -1;

	memcpy( &hc->entry, &dw_hc_entry, sizeof( sdio_hc_entry_t ) );

	if( ( base = msh->base = mmap_device_io( DW_MSH_SIZE, msh->pbase ) ) == (uintptr_t)MAP_FAILED ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: msh base mmap_device_io (0x%x) %s", __FUNCTION__, msh->pbase, strerror( errno ) );
		dw_dinit( hc );
		return( errno );
	}

	hc->ocr		= OCR_VDD_32_33 | OCR_VDD_33_34;
	hc->clk_max	= hc->clk_max ? hc->clk_max : DW_CLK_MAX;
	hc->clk_min	= DW_CLK_MIN;

	hc->caps |= HC_CAP_BW4 | HC_CAP_BW8;
	hc->caps |= HC_CAP_PIO | HC_CAP_DMA | HC_CAP_ACMD12;
	hc->caps |= HC_CAP_200MA | HC_CAP_DRV_TYPE_B;
#if 0
	hc->caps |= HC_CAP_HS200 | HC_CAP_HS | HC_CAP_DDR50 | HC_CAP_SDR104 |
				HC_CAP_SDR50 | HC_CAP_SDR25 | HC_CAP_SDR12;
#else
	hc->caps |= HC_CAP_HS | HC_CAP_DDR50;
#endif

	hc->caps |= HC_CAP_SV_3_3V;

	hc->version		= verid = in32( base + DW_VERID );
	msh->divratio	= DW_DIVRATIO_4;
	hcon			= in32( base + DW_HCON );
	switch( DW_HCON_DATA_WIDTH( hcon ) ) {
		case DW_HCON_DATA_WIDTH16:
			msh->dshft	= 0;
			msh->dins	= in16s;
			msh->douts	= out16s;
			break;
		case DW_HCON_DATA_WIDTH32:
			msh->dshft	= 2;
			msh->dins	= in32s;
			msh->douts	= out32s;
			break;
		case DW_HCON_DATA_WIDTH64:
			msh->dshft	= 3;
			msh->dins	= dw_in64s;
			msh->douts	= dw_out64s;
			break;
		default:
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: msh invalid HCON DATA WIDTH (0x%x)", __FUNCTION__, DW_HCON_DATA_WIDTH( hcon ) );
			dw_dinit( hc );
			return( EINVAL );
	}

	if( !msh->ncards ) {
		msh->ncards		= DW_HCON_NUM_CARD( hcon ) + 1;
	}

	if( !msh->fifo_size ) {
			// we could use power on value, but it has
			// probably been changed by loader
//		msh->fifo_size	= ( in32( base + DW_FIFOTH ) >> DW_FIFOTH_RX_WMARK ) + 1;
		msh->fifo_size = ( DW_VERID_CREV( verid ) >= DW_VERID_CREV_240A ) ? DW_FIFO_SZ_128 : DW_FIFO_SZ_32;
	}

	msh->fifo_size <<= msh->dshft;

	if( DW_VERID_CREV( verid ) >= DW_VERID_CREV_240A ) {

	}

	dw_msh_init( hc );

	if( ( status = dw_idmac_init( hc ) ) != EOK ) {
		dw_dinit( hc );
		return( status );
	}

	hc->cfg.sg_max	= DW_DMA_DESC_MAX;

	hc->caps	&= cfg->caps;		// reconcile command line options

	SIGEV_PULSE_INIT( &event, hc->hc_coid, SDIO_PRIORITY, HC_EV_INTR, NULL );
	if( ( hc->hc_iid = InterruptAttachEvent( cfg->irq[0], &event, _NTO_INTR_FLAGS_TRK_MSK ) ) == -1 ) {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, 1, 1, "%s: InterrruptAttachEvent (irq 0x%x) - %s", __FUNCTION__, cfg->irq[0], strerror( errno ) );
		dw_dinit( hc );
		return( errno );
	}
	
	return( EOK );
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/sdiodi/hc/dwmshc.c $ $Rev: 805416 $")
#endif

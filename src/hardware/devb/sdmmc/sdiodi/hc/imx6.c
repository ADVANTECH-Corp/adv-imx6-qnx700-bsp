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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <hw/inout.h>
#include <sys/mman.h>
#include <internal.h>
#include <sys/syspage.h>
#include <inttypes.h>

#ifdef SDIO_HC_IMX6
#include <imx6.h>

//#define IMX6_SDHCX_DEBUG

#ifdef IMX6_SDHCX_DEBUG
static int imx6_sdhcx_reg_dump( sdio_hc_t *hc, const char *func, int line )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;

	sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1, "%s: line %d", func, line );
	sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
                "%s: HCTL_VER %x, CAP %x",
                __FUNCTION__, 
                imx6_sdhcx_in16( base + IMX6_SDHCX_HCTL_VERSION ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_CAP ));
	
	sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1, "%s: SDMA_ARG2 %x, ADMA %x, BLK %x, ARG %x, CMD %x, RSP10 %x, RSP32 %x, RSP54 %x,  RSP76 %x",
                __FUNCTION__,
                imx6_sdhcx_in32( base + IMX6_SDHCX_DS_ADDR ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_ADMA_ADDRL ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_BLK_ATT ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_ARG ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_CMD ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_RESP0 ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_RESP1 ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_RESP2 ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_RESP3 ) );

	sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1, "%s: PSTATE %x, PCTL %x, SYSCTL %x, IS %x, IE %x, ISE %x, ADMA_ES %x ADMA_ADDRL %x",
                __FUNCTION__,
                imx6_sdhcx_in32( base + IMX6_SDHCX_PSTATE ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_PCTL ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_SYSCTL ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_IS ), 
                imx6_sdhcx_in32( base + IMX6_SDHCX_IE ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_ISE ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_ADMA_ES ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_ADMA_ADDRL ));

	return( EOK );
}
#endif

static int imx6_sdhcx_waitmask( sdio_hc_t *hc, uint32_t reg, uint32_t mask,
                                uint32_t val, uint32_t usec )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;
	uint32_t		cnt;
	int				stat;
	int				rval;
	uint32_t		iter;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;
	stat	= ETIMEDOUT;
	rval	= 0;

    /* fast poll for 1ms - 1us intervals */
	for( cnt = min( usec, 1000 ); cnt; cnt-- ) {
		if( ( ( rval = imx6_sdhcx_in32( base + reg ) ) & mask ) == val ) {
			stat = EOK;
			break;
		}
		nanospin_ns( 1000L );
	}

	if( usec > 1000 && stat ) {
		iter	= usec / 1000L;
		for( cnt = iter; cnt; cnt-- ) {
			if( ( ( rval = imx6_sdhcx_in32( base + reg ) ) & mask ) == val ) {
				stat = EOK;
				break;
			}
			delay( 1 );
		}
	}

	return( stat );
}

static int imx6_sdhcx_reset( sdio_hc_t *hc, uint32_t rst )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;
	uint32_t		sctl;
	int				status;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;

	sctl	= imx6_sdhcx_in32( base + IMX6_SDHCX_SYSCTL );

    /* wait up to 100 ms for reset to complete */
	imx6_sdhcx_out32( base + IMX6_SDHCX_SYSCTL, sctl | rst );
	status = imx6_sdhcx_waitmask( hc, IMX6_SDHCX_SYSCTL, rst, 0, 100000 );

	return( status );
}

static int imx6_sdhcx_pio_xfer( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;
	int				len;
	int				msk;
	int				blksz;
	uint8_t			*addr;
	int 			blks = cmd->blks;

#ifdef IMX6_SDHCX_DEBUG
  	uint64_t    cps, cycle1, cycle2=0;
    cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
	cycle1 = ClockCycles();
#endif

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;
	msk		= ( cmd->flags & SCF_DIR_IN ) ? IMX6_SDHCX_PSTATE_BRE : IMX6_SDHCX_PSTATE_BWE;

	/* Multiple blocks */
	while( blks-- ) {
		blksz	= cmd->blksz;
		while( blksz ) {
			if( sdio_sg_nxt( hc, &addr, &len, blksz ) ) {
				break;
			}
			blksz	-= len;
			len		/= 4;

			/* BRE or BWE is asserted when the available buffer is more than the watermark level */
			while( len ) {
				int xfer_len = min( len, IMX6_SDHCX_WATML_WR );
				len -= xfer_len;

				/* Wait until the read or write buffer is ready */
				if( imx6_sdhcx_waitmask( hc, IMX6_SDHCX_PSTATE, msk, msk, IMX6_SDHCX_TRANSFER_TIMEOUT ) ) {
						 sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 1,
						 	" pio read: timedout for BREN in present state register");
						 return ETIMEDOUT;
				}

				if( ( cmd->flags & SCF_DIR_IN ) ) 
					imx6_sdhcx_in32s( addr, xfer_len, base + IMX6_SDHCX_DATA );
				else  
					imx6_sdhcx_out32s( addr, xfer_len, base + IMX6_SDHCX_DATA );

				addr += xfer_len * 4;
			}
		}
	}

#ifdef IMX6_SDHCX_DEBUG
	cycle2 = ClockCycles();
	sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1, "%s: CMD %d  flags:%x, cmd->blks: %d, cmd->blksz: %d, it took %d us", __FUNCTION__, cmd->opcode, cmd->flags, cmd->blks, cmd->blksz, (int)((cycle2 - cycle1) * 1000 / (cps/1000)));
#endif

	return( EOK );
}

static int imx6_sdhcx_intr_event( sdio_hc_t *hc )
{
	imx6_sdhcx_hc_t		*sdhc;
	sdio_cmd_t		*cmd;
	uint32_t		sts;
	int				cs;
	int				idx;
	uint32_t		rsp;
	uint8_t			rbyte;
	uintptr_t		base;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;
	cs	= CS_CMD_INPROG;
    sts = imx6_sdhcx_in32( base + IMX6_SDHCX_IS );

#ifdef IMX6_SDHCX_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
                "%s: is:%x ie:%x ise:%x\n", __FUNCTION__,
                imx6_sdhcx_in32( base + IMX6_SDHCX_IS ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_IE ),
                imx6_sdhcx_in32( base + IMX6_SDHCX_ISE ));
#endif

    /*
	 * Errata TKT070753, Rev 1.0, IMX6DQCE
	 * on mx6q, there is low possibility that DATA END interrupt comes earlier than DMA
	 * END interrupt which is conflict with standard host controller spec. In this case, read the
	 * status register again will workaround this issue.
	 */
	if( ( sts & IMX6_SDHCX_INTR_TC ) && !( sts & IMX6_SDHCX_INTR_DMA ) )
        sts = imx6_sdhcx_in32( base + IMX6_SDHCX_IS );

	/* Clear interrupt events */
	imx6_sdhcx_out32(base + IMX6_SDHCX_IS, sts);

	/* Card insertion and card removal events */
	if( ( sts & ( IMX6_SDHCX_INTR_CINS | IMX6_SDHCX_INTR_CREM ) ) ) {
		sdio_hc_event( hc, HC_EV_CD );
	}
	
	if( ( cmd = hc->wspc.cmd ) == NULL ) {
		return( EOK );
	}

	/* Tuning commands */
	if( cmd->opcode == SD_SEND_TUNING_BLOCK || cmd->opcode == MMC_SEND_TUNING_BLOCK ) {
		/* Though we don't need the data, we need to clear the buffer */
		if( sts & IMX6_SDHCX_INTR_BRR ) {
			imx6_sdhcx_pio_xfer( hc, cmd );
		}

		if( sts & IMX6_SDHCX_IS_ERRI )
			cs = CS_CMD_CMP_ERR;
		else
			cs = CS_CMD_CMP;

		sdio_cmd_cmplt( hc, cmd, cs);
		return EOK;
	}

	/* Check of errors */
	if( sts & IMX6_SDHCX_IS_ERRI) {
        sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 1,
                        "%s, ERROR in HC, CMD%d, sts: 0x%x:  is 0x%x, ac12 0x%x, IMX6_SDHCX_IS_ERRI: 0x%x",
                        __FUNCTION__, cmd->opcode, sts, imx6_sdhcx_in32( base + IMX6_SDHCX_IS ),
                        imx6_sdhcx_in32( base + IMX6_SDHCX_AC12 ), IMX6_SDHCX_IS_ERRI );

		if( sts & IMX6_SDHCX_INTR_DTO )		cs = CS_DATA_TO_ERR;
		if( sts & IMX6_SDHCX_INTR_DCRC )	cs = CS_DATA_CRC_ERR;
		if( sts & IMX6_SDHCX_INTR_DEB )		cs = CS_DATA_END_ERR;
		if( sts & IMX6_SDHCX_INTR_CTO )		cs = CS_CMD_TO_ERR;
		if( sts & IMX6_SDHCX_INTR_CCRC )	cs = CS_CMD_CRC_ERR;
		if( sts & IMX6_SDHCX_INTR_CEB )		cs = CS_CMD_END_ERR;
		if( sts & IMX6_SDHCX_INTR_CIE )		cs = CS_CMD_IDX_ERR;
		if( sts & IMX6_SDHCX_INTR_DMAE )	cs = CS_DATA_TO_ERR;
		if( sts & IMX6_SDHCX_INTR_ACE )     cs = CS_DATA_TO_ERR;
		if( !cs )							cs = CS_CMD_CMP_ERR;
		imx6_sdhcx_reset( hc, IMX6_SDHCX_SYSCTL_SRC | IMX6_SDHCX_SYSCTL_SRD );
	} else {
		/* End of command */
		if( ( sts & IMX6_SDHCX_INTR_CC ) ) {

			cs = CS_CMD_CMP;

 			// Errata ENGcm03648
            if( cmd->flags & SCF_RSP_BUSY ) {
                int timeout = 16 * 1024 * 1024;

                while (!(imx6_sdhcx_in32(base + IMX6_SDHCX_PSTATE) & IMX6_SDHCX_PSTATE_DL0SL) && timeout--)
                    nanospin_ns(100);

                if (timeout <= 0) {
                    cs = CS_CMD_TO_ERR;
					imx6_sdhcx_reset( hc, IMX6_SDHCX_SYSCTL_SRC | IMX6_SDHCX_SYSCTL_SRD );
                    sdio_slogf(_SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 1, 
						"%s: busy done wait timeout for cmd: %d", __func__, cmd->opcode);
                }
            }

			if( ( cmd->flags & SCF_RSP_136 ) ) {
                /*
				 *  CRC is not included in the response reg, left
                 *  shift 8 bits to match the 128 CID/CSD structure
				 */
				for( idx = 0, rbyte = 0; idx < 4; idx++ ) {
					rsp				= imx6_sdhcx_in32( base + IMX6_SDHCX_RESP0
                                                       + idx * 4 );
					cmd->rsp[3-idx] = (rsp << 8) + rbyte;
					rbyte			= rsp >> 24;
				}
			} else if( ( cmd->flags & SCF_RSP_PRESENT ) ) {
				cmd->rsp[0] = imx6_sdhcx_in32( base + IMX6_SDHCX_RESP0 );
			}
		}

		/* End of data transfer */
		if ( sts & IMX6_SDHCX_INTR_TC) {
			cs = CS_CMD_CMP;
			cmd->rsp[0] = imx6_sdhcx_in32( base + IMX6_SDHCX_RESP0 );
		} 
		
		/* Doesn't need to do anything for DMA interrupt */
		if( ( sts & IMX6_SDHCX_INTR_DMA ) ) {
			cs = CS_CMD_CMP;
		}

		if( ( sts & ( IMX6_SDHCX_INTR_BWR | IMX6_SDHCX_INTR_BRR ) ) ) {
			if (EOK == imx6_sdhcx_pio_xfer( hc, cmd ))
				cs = CS_CMD_CMP;
			else
				cs = CS_DATA_TO_ERR;
		}
	}

	if ( cs != CS_CMD_INPROG ) {
		if ( (cs == CS_DATA_TO_ERR) || (cs == CS_CMD_TO_ERR) ) {
			// Timeout error case, check if card removed
			if(!(hc->entry.cd(hc) & CD_INS))
				cs = CS_CARD_REMOVED;
		}
		sdio_cmd_cmplt( hc, cmd, cs );
	}

	return( EOK );
}

static int imx6_sdhcx_event( sdio_hc_t *hc, sdio_event_t *ev )
{
    int					status;

	switch( ev->code ) {
		case HC_EV_INTR:
			status = imx6_sdhcx_intr_event( hc );
			InterruptUnmask( hc->cfg.irq[0], hc->hc_iid );
			break;

		default:
			status = bs_event( hc, ev );
			break;
	}

	return( status );
}

static int imx6_sdhcx_adma_setup( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	imx6_sdhcx_hc_t			*sdhc;
	imx6_sdhcx_adma32_t		*adma;
	sdio_sge_t			*sgp;
	int					sgc;
	int					sgi;
	int					acnt;
	int					alen;
	int					sg_count;
	paddr_t				paddr;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	adma	= (imx6_sdhcx_adma32_t *)sdhc->adma;

	sgc = cmd->sgc;
	sgp = cmd->sgl;

	if( !( cmd->flags & SCF_DATA_PHYS ) ) {
		sdio_vtop_sg( sgp, sdhc->sgl, sgc, cmd->mhdl );
		sgp = sdhc->sgl;
	}

	for( sgi = 0, acnt = 0; sgi < sgc; sgi++, sgp++ ) {
		paddr		= sgp->sg_address;
		sg_count	= sgp->sg_count;
		while( sg_count ) {
			alen		= min( sg_count, IMX6_SDHCX_ADMA2_MAX_XFER );
			adma->attr	= IMX6_SDHCX_ADMA2_VALID | IMX6_SDHCX_ADMA2_TRAN;
			adma->addr	= paddr;
			adma->len	= alen;
			sg_count	-= alen; 
			paddr		+= alen;
			adma++;
			if( ++acnt > ADMA_DESC_MAX ) {
				return( ENOTSUP );
			}
		}
	}

	adma--;
	adma->attr |= IMX6_SDHCX_ADMA2_END;

	imx6_sdhcx_out32( sdhc->base + IMX6_SDHCX_ADMA_ADDRL, sdhc->admap );

	return( EOK );
}

static int imx6_sdhcx_sdma_setup( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	imx6_sdhcx_hc_t		*sdhc;
	sdio_sge_t		*sgp;
	int				sgc;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
    
	sgc = cmd->sgc;
	sgp = cmd->sgl;

	if( !( cmd->flags & SCF_DATA_PHYS ) ) {
		sdio_vtop_sg( sgp, sdhc->sgl, sgc, cmd->mhdl );
		sgp = sdhc->sgl;
	}

	imx6_sdhcx_out32( sdhc->base + IMX6_SDHCX_DS_ADDR, sgp->sg_address );

	return( EOK );
}

static int imx6_sdhcx_xfer_setup( sdio_hc_t *hc, sdio_cmd_t *cmd,
                                  uint32_t *command, uint32_t *imask )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;
	uint32_t		pctl, mix_ctrl;
	int				status = EOK;

	sdhc		= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base		= sdhc->base;
	pctl		= imx6_sdhcx_in32( base + IMX6_SDHCX_PCTL ) & ~IMX6_SDHCX_PCTL_DMA_MSK;
    mix_ctrl    = 0;

	/* Data present */
	*command	|= IMX6_SDHCX_CMD_DP;

	if (cmd->flags & SCF_DIR_IN)
		mix_ctrl    |=  IMX6_SDHCX_MIX_CTRL_DDIR;

	*imask		|= IMX6_SDHCX_INTR_DTO | IMX6_SDHCX_INTR_DCRC | IMX6_SDHCX_INTR_DEB | IMX6_SDHCX_INTR_TC;

	status = sdio_sg_start( hc, cmd->sgl, cmd->sgc );

	if( cmd->sgc && ( hc->caps & HC_CAP_DMA ) ) {
		if( ( sdhc->flags & SF_USE_ADMA ) ) {
			if( ( status = imx6_sdhcx_adma_setup( hc, cmd ) ) == EOK )
				pctl		|= IMX6_SDHCX_PCTL_ADMA2;
		} else {
			if( ( status = imx6_sdhcx_sdma_setup( hc, cmd ) ) == EOK )
				pctl		|= IMX6_SDHCX_PCTL_SDMA;
		}

		if (status == EOK) {
			*imask |= IMX6_SDHCX_INTR_DMAE;
			mix_ctrl |= IMX6_SDHCX_MIX_CTRL_DE;
		}
	}

	/* Use PIO */
	if( status || !( hc->caps & HC_CAP_DMA ) ) {
		if( ( cmd->flags & SCF_DATA_PHYS ) ) {
			return( status );
		}
		status = EOK;

		*imask |= ( cmd->flags & SCF_DIR_IN ) ?
              IMX6_SDHCX_INTR_BRR : IMX6_SDHCX_INTR_BWR;
	}

	if( cmd->blks > 1 ) {
		mix_ctrl |= IMX6_SDHCX_MIX_CTRL_MBS | IMX6_SDHCX_MIX_CTRL_BCE;
		if( ( hc->caps & HC_CAP_ACMD23 ) && ( cmd->flags & SCF_SBC ) ) {
			mix_ctrl |= IMX6_SDHCX_MIX_CTRL_ACMD23;
		}
		else if( ( hc->caps & HC_CAP_ACMD12 ) ) {
			mix_ctrl |= IMX6_SDHCX_MIX_CTRL_ACMD12;
		}
	}

	sdhc->mix_ctrl = mix_ctrl;

    imx6_sdhcx_out32( base + IMX6_SDHCX_PCTL, pctl );
	imx6_sdhcx_out32( base + IMX6_SDHCX_BLK_ATT, cmd->blksz |
                      ( cmd->blks << IMX6_SDHCX_BLK_BLKCNT_SHIFT ) );

	return( status );
}

static int imx6_sdhcx_cmd( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;
	uint32_t		pmask, pval;
	uint32_t		imask;
	int				status;
	uint32_t		command, reg;

#ifdef IMX6_SDHCX_DEBUG
  	uint64_t    cps, cycle1, cycle2=0;
    cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
	cycle1 = ClockCycles();
#endif

	sdhc			= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base			= sdhc->base;

	/* Command inhibit (CMD) and CMD line signal level state */
	pmask	= IMX6_SDHCX_PSTATE_CMDI | IMX6_SDHCX_PSTATE_CLSL;

	command	= cmd->opcode << 24;

	if( ( cmd->opcode == MMC_STOP_TRANSMISSION ) || (cmd->opcode == SD_STOP_TRANSMISSION))
		command |= IMX6_SDHCX_CMD_TYPE_CMD12;

    imask = IMX6_SDHCX_IE_DFLTS;

	if( ( cmd->flags & SCF_DATA_MSK ) ) {
		pmask |= IMX6_SDHCX_PSTATE_DATI | IMX6_SDHCX_PSTATE_DL0SL;

		if( ( cmd->blks && (status = imx6_sdhcx_xfer_setup( hc, cmd, &command, &imask ) )) != EOK )
			return( status );
    } else {
		/* Enable command complete intr */
		imask |= IMX6_SDHCX_INTR_CC;					
	}

	if( ( cmd->flags & SCF_RSP_PRESENT ) ) {
		if (cmd->flags & SCF_RSP_136)
			command |= IMX6_SDHCX_CMD_RSP_TYPE_136;
		else if (cmd->flags & SCF_RSP_BUSY ) {
			command |= IMX6_SDHCX_CMD_RSP_TYPE_48b;

			/* command 12 can be asserted even if data lines are busy */
			if( ( cmd->opcode != MMC_STOP_TRANSMISSION) && ( cmd->opcode != SD_STOP_TRANSMISSION ) )
				pmask |= IMX6_SDHCX_PSTATE_DATI | IMX6_SDHCX_PSTATE_DL0SL;
		} else {
			command |= IMX6_SDHCX_CMD_RSP_TYPE_48;
		}

		if (cmd->flags & SCF_RSP_OPCODE)		// Index check
			command |= IMX6_SDHCX_CMD_CICE;

		if (cmd->flags & SCF_RSP_CRC) 			// CRC check 
			command |= IMX6_SDHCX_CMD_CCCE;
	}

    reg = imx6_sdhcx_in32(base + IMX6_SDHCX_PSTATE);

	/* If host requires tunning */
	if( reg & ( IMX6_SDHCX_PSTATE_RTR)) {
		sdio_hc_event( hc, HC_EV_TUNE );
	}

    /* wait till card is ready to handle next command */
	pval = pmask & (IMX6_SDHCX_PSTATE_CLSL | IMX6_SDHCX_PSTATE_DL0SL);
    if ((( reg & pmask) != pval) &&
		(status = imx6_sdhcx_waitmask(hc, IMX6_SDHCX_PSTATE, 
		pmask, pval, IMX6_SDHCX_COMMAND_TIMEOUT))) {
        sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 1,
                    "%s: Timeout CMD_INHIBIT at CMD%d", __FUNCTION__, cmd->opcode);
        return( status );
    }

	/* IDLE state, need to initialize clock */
    if (cmd->opcode == 0) {
        imx6_sdhcx_out32( base + IMX6_SDHCX_SYSCTL,
                          (imx6_sdhcx_in32( base + IMX6_SDHCX_SYSCTL) | IMX6_SDHCX_SYSCTL_INITA));
    	if( ( status = imx6_sdhcx_waitmask( hc, IMX6_SDHCX_SYSCTL, IMX6_SDHCX_SYSCTL_INITA, 0,
                                            IMX6_SDHCX_COMMAND_TIMEOUT) ) ) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 1,
						"%s: Timeout IMX6_SDHCX_SYSCTL at CMD%d", __FUNCTION__, cmd->opcode);
            return( status );
        }
    }

    sdhc->mix_ctrl |= (imx6_sdhcx_in32( base + IMX6_SDHCX_MIX_CTRL) & \
					(IMX6_SDHCX_MIX_CTRL_EXE_TUNE | \
					IMX6_SDHCX_MIX_CTRL_SMP_CLK_SEL | \
					IMX6_SDHCX_MIX_CTRL_AUTOTUNE_EN | \
					IMX6_SDHCX_MIX_CTRL_FBCLK_SEL |
					IMX6_SDHCX_MIX_CTRL_DDR_EN));

    imx6_sdhcx_out32( base + IMX6_SDHCX_MIX_CTRL, sdhc->mix_ctrl);
    sdhc->mix_ctrl = 0;
    
    /* Rev 1.0 i.MX6x chips require the watermark register to be set prior to
     * every SD command being sent. If the watermark is not set only SD interface
     * 3 works.
     */
    imx6_sdhcx_out32(base + IMX6_SDHCX_WATML,
                     ((0x8 << IMX6_SDHCX_WATML_WRBRSTLENSHIFT) |
					  (IMX6_SDHCX_WATML_WR << IMX6_SDHCX_WATML_WRWMLSHIFT) |
                      (0x8 << IMX6_SDHCX_WATML_RDBRSTLENSHIFT) |
					  (IMX6_SDHCX_WATML_RD << IMX6_SDHCX_WATML_RDWMLSHIFT)));

    /* enable interrupts */
    imx6_sdhcx_out32( base + IMX6_SDHCX_IS,  IMX6_SDHCX_INTR_CLR_ALL);
    imx6_sdhcx_out32( base + IMX6_SDHCX_IE,  imask );
	imx6_sdhcx_out32( base + IMX6_SDHCX_ARG, cmd->arg );
	imx6_sdhcx_out32( base + IMX6_SDHCX_CMD, command );

#ifdef IMX6_SDHCX_DEBUG
	cycle2 = ClockCycles();
	sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
               "%s(), Issuing CMD%d,  cmd_arg:0x%x command:0x%x blks: %d, blksz: %d, mix_ctrl: 0x%x, it took %d us\n", __FUNCTION__, hc->wspc.cmd->opcode, cmd->arg, command, cmd->blks, cmd->blksz, imx6_sdhcx_in32( base + IMX6_SDHCX_MIX_CTRL), (int)((cycle2 - cycle1) * 1000 / (cps/1000)));
#endif

	return( EOK );
}

static int imx6_sdhcx_abort( sdio_hc_t *hc, sdio_cmd_t *cmd )
{
	return( EOK );
}

static int imx6_sdhcx_pwr( sdio_hc_t *hc, int vdd )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;
	uint32_t		reg;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;

	if( !hc->vdd || !vdd ) {			/* reset core */
		imx6_sdhcx_reset( hc, IMX6_SDHCX_SYSCTL_SRA );

		/* make a clean environment */
		imx6_sdhcx_out32( base + IMX6_SDHCX_MIX_CTRL, 0);
		imx6_sdhcx_out32( base + IMX6_SDHCX_TUNE_CTRL_STATUS, 0);
	}

	if( vdd ) {
		reg = imx6_sdhcx_in32( base + IMX6_SDHCX_VEND_SPEC ) &
              ~IMX6_SDHCX_VEND_SPEC_SDVS_MSK;
		switch( vdd ) {
			case OCR_VDD_17_195:
				reg |= IMX6_SDHCX_VEND_SPEC_SDVS1V8;
                sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
                            "%s(): setting power to 1.8v\n", __FUNCTION__);
                break;

			case OCR_VDD_29_30:
			case OCR_VDD_30_31:
			case OCR_VDD_32_33:
            case OCR_VDD_33_34:
				reg |= IMX6_SDHCX_VEND_SPEC_SDVS3V0;
                sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
                            "%s(): setting power to 3.3v\n", __FUNCTION__);
                break;

			default:
                sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
                            "%s(): unrecognized voltage level. vdd: 0x%x\n", __FUNCTION__, vdd);
				return( EINVAL );
		}

		imx6_sdhcx_out32( base + IMX6_SDHCX_VEND_SPEC, reg);
		imx6_sdhcx_out32( base + IMX6_SDHCX_ISE, IMX6_SDHCX_ISE_DFLTS );
	}

	hc->vdd = vdd;
	return( EOK );
}

static int imx6_sdhcx_bus_mode( sdio_hc_t *hc, int bus_mode )
{
	hc->bus_mode = bus_mode;
	return( EOK );
}

static int imx6_sdhcx_bus_width( sdio_hc_t *hc, int width )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;
	uint32_t		hctl;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;
	hctl	= imx6_sdhcx_in32( base + IMX6_SDHCX_PCTL ) &
          ~( IMX6_SDHCX_PCTL_DTW4 | IMX6_SDHCX_PCTL_DTW8);

	switch( width ) {
		case BUS_WIDTH_8:
			hctl |=  IMX6_SDHCX_PCTL_DTW8;
			break;

		case BUS_WIDTH_4:
			hctl |= IMX6_SDHCX_PCTL_DTW4;
			break;

		case BUS_WIDTH_1:
		default:
			break;
	}

	imx6_sdhcx_out32( base + IMX6_SDHCX_PCTL, hctl );
	hc->bus_width = width;
	return( EOK );
}

static int imx6_sdhcx_clk( sdio_hc_t *hc, int clk )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;
	uint32_t		sctl;
	int			    pre_div = 1, div = 1;
	int 			ddr_mode = 0;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;
	sctl	= imx6_sdhcx_in32( base + IMX6_SDHCX_SYSCTL );

    /* stop clock */
	sctl &= ~(IMX6_SDHCX_SYSCTL_DTO_MSK | IMX6_SDHCX_SYSCTL_SDCLKFS_MSK);
	sctl |= IMX6_SDHCX_SYSCTL_DTO_MAX | IMX6_SDHCX_SYSCTL_SRC |
          IMX6_SDHCX_SYSCTL_SRD;

	if (hc->timing == TIMING_DDR50) {
		ddr_mode = 1;
		pre_div = 2;
	}

	if (clk > hc->clk_max)
		clk = hc->clk_max;

	while ((hc->clk_max / (pre_div * 16) > clk) && (pre_div < 256))
        pre_div *= 2;

	while ((hc->clk_max / (pre_div * div) > clk) && (div < 16))
        div++;

#ifdef IMX6_SDHCX_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1, "desired SD clock: %d, actual: %d\n",
        clk, hc->clk_max / pre_div / div);
#endif

    pre_div >>= (1 + ddr_mode);
    div--;

	sctl = ((0xE << IMX6_SDHCX_SYSCTL_DTO_SHIFT) |
		(pre_div << IMX6_SDHCX_SYSCTL_SDCLKFS_SHIFT) |
		(div << IMX6_SDHCX_SYSCTL_DVS_SHIFT) | 0xF);

	/* enable clock to the card */
	imx6_sdhcx_out32( base + IMX6_SDHCX_SYSCTL, sctl );

	/* wait for clock to stabilize */
	imx6_sdhcx_waitmask( hc, IMX6_SDHCX_PSTATE, IMX6_SDHCX_PSTATE_SDSTB,
		IMX6_SDHCX_PSTATE_SDSTB, IMX6_SDHCX_CLOCK_TIMEOUT);

	nanospin_ns( 1000L );

	hc->clk = clk;

	return( EOK );
}

static int imx6_sdhcx_timing( sdio_hc_t *hc, int timing )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t			base;
	uint32_t			mix_ctl;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base 	= sdhc->base;
    
	hc->timing = timing;
	mix_ctl = imx6_sdhcx_in32( base + IMX6_SDHCX_MIX_CTRL);
	
	if (timing == TIMING_DDR50) 
		mix_ctl |= IMX6_SDHCX_MIX_CTRL_DDR_EN;
	else
		mix_ctl &= ~IMX6_SDHCX_MIX_CTRL_DDR_EN;

	imx6_sdhcx_out32( base + IMX6_SDHCX_MIX_CTRL, mix_ctl);
	imx6_sdhcx_clk(hc, hc->clk);

	return( EOK );
}

static int imx6_sdhcx_signal_voltage( sdio_hc_t *hc, int signal_voltage )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;
	uint32_t		reg, dlsl;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;

	if(( hc->version < IMX6_SDHCX_SPEC_VER_3 ) || ( hc->signal_voltage == signal_voltage )) {
		return( EOK );
	}

    if( (signal_voltage == SIGNAL_VOLTAGE_3_3 ) || ( signal_voltage == SIGNAL_VOLTAGE_3_0 )) {
		reg = imx6_sdhcx_in32( base + IMX6_SDHCX_VEND_SPEC );
		reg &= ~IMX6_SDHCX_VEND_SPEC_SDVS_MSK;
		reg |= IMX6_SDHCX_VEND_SPEC_SDVS3V0; 
        imx6_sdhcx_out32( base + IMX6_SDHCX_VEND_SPEC, reg);

        sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
                        "%s: switched to  3.3V ", __FUNCTION__ );
    } else if( signal_voltage == SIGNAL_VOLTAGE_1_8 ) {
		reg = imx6_sdhcx_in32( base + IMX6_SDHCX_VEND_SPEC );
		reg &= ~IMX6_SDHCX_VEND_SPEC_FRC_SDCLK_ON;

		/* stop sd clock */
		imx6_sdhcx_out32( base + IMX6_SDHCX_VEND_SPEC, reg);

		dlsl = IMX6_SDHCX_PSTATE_DL0SL | IMX6_SDHCX_PSTATE_DL1SL 
			| IMX6_SDHCX_PSTATE_DL2SL | IMX6_SDHCX_PSTATE_DL3SL;
		if (imx6_sdhcx_waitmask( hc, IMX6_SDHCX_PSTATE, dlsl, 0, IMX6_SDHCX_CLOCK_TIMEOUT)) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
							"%s() : Failed to switch to 1.8V, can't stop SD Clock", __FUNCTION__);
			return (EIO);
		}
		reg |= IMX6_SDHCX_VEND_SPEC_SDVS1V8; 
        imx6_sdhcx_out32( base + IMX6_SDHCX_VEND_SPEC, reg);

		/* sleep at least 5ms */
		delay(5);

		/* restore sd clock status */
		imx6_sdhcx_out32( base + IMX6_SDHCX_VEND_SPEC, reg | IMX6_SDHCX_VEND_SPEC_FRC_SDCLK_ON);
		delay(1);

		reg = imx6_sdhcx_in32( base + IMX6_SDHCX_VEND_SPEC );

		/* Data lines should be high now */
		if (!(reg & IMX6_SDHCX_VEND_SPEC_SDVS1V8) || 
			!(imx6_sdhcx_in32(base + IMX6_SDHCX_PSTATE) & dlsl)) {
			sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
							"%s(): Failed to switch to 1.8V, DATA lines remain in low", __FUNCTION__);
			return (EIO);
		}
        sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1,
                        "%s: switched to 1.8V ", __FUNCTION__ );
    } else {
		return ( EINVAL );
	}

    hc->signal_voltage = signal_voltage;

	/* The board specific code may need to do something  */
#ifdef BS_PAD_CONF
	bs_pad_conf( hc, SDIO_TRUE );
#endif

    return ( EOK );
}

static void imx6_sdhcx_pre_tuning( sdio_hc_t *hc, int tuning)
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t			base;
	uint32_t			mix_ctl;

#ifdef IMX6_SDHCX_DEBUG
	sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1, "%s: tuning at %d\n", __FUNCTION__, tuning );
#endif

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;
	
	imx6_sdhcx_reset( hc, IMX6_SDHCX_SYSCTL_SRA );
	delay(1);

	imx6_sdhcx_clk( hc, hc->clk );

	mix_ctl = imx6_sdhcx_in32( base + IMX6_SDHCX_MIX_CTRL);
	mix_ctl |= (IMX6_SDHCX_MIX_CTRL_EXE_TUNE | \
		   IMX6_SDHCX_MIX_CTRL_SMP_CLK_SEL | \
		   IMX6_SDHCX_MIX_CTRL_FBCLK_SEL);

	imx6_sdhcx_out32( base + IMX6_SDHCX_MIX_CTRL, mix_ctl);
	imx6_sdhcx_out32( base + IMX6_SDHCX_TUNE_CTRL_STATUS, 
			tuning << IMX6_SDHCI_TUNE_DLY_CELL_SET_PRE_OFF);
}

static int imx6_sdhcx_send_tune_cmd( sdio_hc_t *hc, int op )
{
	struct sdio_cmd		cmd;
	int					tlen;
	int					status;
	sdio_sge_t          sge;
	uint8_t				buf[128];

	tlen	= hc->bus_width == BUS_WIDTH_8 ? 128 : 64;

	sge.sg_count = tlen; 
	sge.sg_address = (paddr_t)&buf;

	memset((void *)&cmd,0,sizeof(struct sdio_cmd));
	sdio_setup_cmd( &cmd, SCF_CTYPE_ADTC | SCF_RSP_R1, op, 0 );

	/* Seems if not read the data out of the buffer, it will be some problems */
	sdio_setup_cmd_io( &cmd, SCF_DIR_IN, 1, tlen, &sge, 1, NULL );
	status = sdio_issue_cmd( &hc->device, &cmd, SDIO_TIME_DEFAULT);

	/* Command error */
	if (cmd.status > CS_CMD_CMP)
		status = EIO;

	return( status );
}

static int imx6_sdhcx_tune( sdio_hc_t *hc, int op )
{
	imx6_sdhcx_hc_t			*sdhc;
	uintptr_t			base;
	uint32_t			mix_ctl;
	int					status = EIO;
	int					min, max;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base	= sdhc->base;

	if( hc->version < IMX6_SDHCX_SPEC_VER_3 ) {
		return( EOK );
	}

    /* return if not HS200 or SDR104, and not SDR50 that requires tuning */
	if((hc->timing != TIMING_SDR104 ) &&
		( hc->timing != TIMING_HS200 ) &&
		(( hc->timing == TIMING_SDR50 ) &&
		!( sdhc->flags & SF_TUNE_SDR50))) {
		return( EOK );
	}

	min = IMX6_SDHCI_TUNE_CTRL_MIN;
	while (min < IMX6_SDHCI_TUNE_CTRL_MAX) {
		imx6_sdhcx_pre_tuning( hc, min);
		if ((status = imx6_sdhcx_send_tune_cmd(hc, op)) == EOK) {
#ifdef IMX6_SDHCX_DEBUG
			sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1, "%s() Found the mximum not-good value: %d",
				__func__, min);
#endif
			break;
		}

		min += IMX6_SDHCI_TUNE_STEP;
	}

	max = min + IMX6_SDHCI_TUNE_STEP;
	while (max < IMX6_SDHCI_TUNE_CTRL_MAX) {
		imx6_sdhcx_pre_tuning( hc, max);
		if ((status = imx6_sdhcx_send_tune_cmd(hc, op)) != EOK) {
#ifdef IMX6_SDHCX_DEBUG
			sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1, "%s() Found the mximum good value: %d",
				__func__, max);
#endif
			max += IMX6_SDHCI_TUNE_STEP;
			break;
		}
		max += IMX6_SDHCI_TUNE_STEP;
	}

	imx6_sdhcx_pre_tuning( hc, (min + max)/2);
	if ((status = imx6_sdhcx_send_tune_cmd(hc, op) != EOK)) {
		status = EIO;
#ifdef IMX6_SDHCX_DEBUG
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 1, "%s(), failed tunning", __func__);
#endif
	}

	mix_ctl = imx6_sdhcx_in32( base + IMX6_SDHCX_MIX_CTRL);
	mix_ctl &= ~IMX6_SDHCX_MIX_CTRL_EXE_TUNE;
	
	/* Use the fixed clock if failed */
	if (status) {
		status = EOK;
		mix_ctl &= ~IMX6_SDHCX_MIX_CTRL_SMP_CLK_SEL;
		sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1, "%s(), failed tuning, using the fixed clock", __func__);
	} else {
#ifdef IMX6_SDHCX_DEBUG
		sdio_slogf( _SLOGC_SDIODI, _SLOG_INFO, hc->cfg.verbosity, 1, "%s(), tuned DLY_CELL_SET_PRE at: %d\n",
			__func__, (min + max)/2);
#endif
	}

	imx6_sdhcx_out32( base + IMX6_SDHCX_MIX_CTRL, mix_ctl);
	return( status );
}

static int imx6_sdhcx_cd( sdio_hc_t *hc )
{
	imx6_sdhcx_hc_t		*sdhc;
	uintptr_t		base;
	int				stat;

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;
	base  	= sdhc->base;

	stat = imx6_sdhcx_waitmask( hc, IMX6_SDHCX_PSTATE, IMX6_SDHCX_CARD_STABLE,
                                IMX6_SDHCX_CARD_STABLE, 100000 );

	if( !( imx6_sdhcx_in32( base + IMX6_SDHCX_PSTATE ) & IMX6_SDHCX_PSTATE_CI )) 
		return( SDIO_FALSE );

	return( stat == EOK ? SDIO_TRUE : SDIO_FALSE );
}

int imx6_sdhcx_dinit( sdio_hc_t *hc )
{
	imx6_sdhcx_hc_t	*sdhc;

	if ( !hc || !hc->cs_hdl )
		return (EOK);

	sdhc	= (imx6_sdhcx_hc_t *)hc->cs_hdl;

	if( sdhc->base ) {
		imx6_sdhcx_pwr( hc, 0 );
		imx6_sdhcx_reset( hc, IMX6_SDHCX_SYSCTL_SRA );
		imx6_sdhcx_out32( sdhc->base + IMX6_SDHCX_ISE, 0 );
		imx6_sdhcx_out32( sdhc->base + IMX6_SDHCX_IE, 0);

		if( hc->hc_iid != -1 ) {
			InterruptDetach( hc->hc_iid );
		}
		munmap_device_memory( (void *)sdhc->base, hc->cfg.base_addr_size[0] );
	}

	if( sdhc->adma )
		munmap( sdhc->adma, sizeof( imx6_sdhcx_adma32_t ) * ADMA_DESC_MAX );

	free( sdhc );
	hc->cs_hdl = NULL;

	return( EOK );
}

static sdio_hc_entry_t imx6_sdhcx_hc_entry ={ 16,
			   imx6_sdhcx_dinit, NULL,
			   imx6_sdhcx_cmd, imx6_sdhcx_abort,
			   imx6_sdhcx_event, imx6_sdhcx_cd, imx6_sdhcx_pwr,
			   imx6_sdhcx_clk, imx6_sdhcx_bus_mode,
			   imx6_sdhcx_bus_width, imx6_sdhcx_timing,
			   imx6_sdhcx_signal_voltage, NULL,
			   NULL, imx6_sdhcx_tune,
			   NULL
};

int imx6_sdhcx_init( sdio_hc_t *hc )
{
	sdio_hc_cfg_t		*cfg;
	imx6_sdhcx_hc_t		*sdhc;
	uint32_t			cap;
	uintptr_t			base;
	struct sigevent		event;

	hc->hc_iid			= -1;
	cfg					= &hc->cfg;

	memcpy( &hc->entry, &imx6_sdhcx_hc_entry, sizeof( sdio_hc_entry_t ) );

	if( !cfg->base_addr[0] )
		return( ENODEV );

	if( !cfg->base_addr_size[0] )
		cfg->base_addr_size[0] = IMX6_SDHCX_SIZE;

	if( ( sdhc = hc->cs_hdl = calloc( 1, sizeof( imx6_sdhcx_hc_t ) ) ) == NULL )
		return( ENOMEM );

	if( ( base = sdhc->base = (uintptr_t)mmap_device_io(
              cfg->base_addr_size[0],
              cfg->base_addr[0] ) ) == (uintptr_t)MAP_FAILED )
    {
		sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 1,
                    "%s: SDHCI base mmap_device_io (%s)",
                    __FUNCTION__, strerror( errno ) );
		imx6_sdhcx_dinit( hc );
		return( errno );
	}

	sdhc->usdhc_addr = cfg->base_addr[0];

	if( !hc->version )
		hc->version = imx6_sdhcx_in16( base + IMX6_SDHCX_HCTL_VERSION ) &
              IMX6_SDHCX_SPEC_VER_MSK;

	cap			= imx6_sdhcx_in32( base + IMX6_SDHCX_CAP );

	if (cfg->clk)
		hc->clk_max = cfg->clk;
	else
		hc->clk_max = IMX6_CLOCK_DEFAULT;

	hc->caps	|= HC_CAP_BSY | HC_CAP_BW4 | HC_CAP_CD_INTR;
	hc->caps	|= HC_CAP_ACMD12 | HC_CAP_200MA | HC_CAP_DRV_TYPE_B;

	if( cap & IMX6_SDHCX_CAP_HS ) 
		hc->caps |= HC_CAP_HS;
    
	if(  cap & IMX6_SDHCX_CAP_DMA )
		hc->caps |= HC_CAP_DMA;
	
    hc->caps	|= HC_CAP_SDR104;
    hc->caps	|= HC_CAP_SDR50 | HC_CAP_SDR25 | HC_CAP_SDR12;
    hc->caps	|= HC_CAP_DDR50 | HC_CAP_HS200;
    sdhc->flags |= SF_TUNE_SDR50;
	if( cap & IMX6_SDHCX_CAP_S18 ) {
        hc->ocr		= OCR_VDD_17_195;
        hc->caps	|= HC_CAP_SV_1_8V;
	}

	if( ( cap & IMX6_SDHCX_CAP_S30 ) ) {
		hc->ocr		= OCR_VDD_30_31 | OCR_VDD_29_30;
        hc->caps	|= HC_CAP_SV_3_0V;
	}
    
	if( ( cap & IMX6_SDHCX_CAP_S33 ) ) {
		hc->ocr		= OCR_VDD_32_33 | OCR_VDD_33_34;
        hc->caps	|= HC_CAP_SV_3_3V;
	}
#ifdef ADMA_SUPPORTED    
    if( ( cap & IMX6_SDHCX_CAP_DMA ) ) {
        if( hc->version >= IMX6_SDHCX_SPEC_VER_3 ) {
			hc->cfg.sg_max	= ADMA_DESC_MAX;
			if( ( sdhc->adma = mmap( NULL, sizeof( imx6_sdhcx_adma32_t ) *
                                     ADMA_DESC_MAX, PROT_READ | PROT_WRITE | PROT_NOCACHE, 
                                     MAP_PRIVATE | MAP_ANON | MAP_PHYS, NOFD, 0 ) )
                == MAP_FAILED )
            {
				sdio_slogf( _SLOGC_SDIODI, _SLOG_ERROR, hc->cfg.verbosity, 1,
                            "%s: ADMA mmap %s", __FUNCTION__,
                            strerror( errno ) );
				imx6_sdhcx_dinit( hc );
				return( errno );
			}
			sdhc->flags		|= SF_USE_ADMA;
			sdhc->admap		= sdio_vtop( sdhc->adma );
			if( hc->version >= IMX6_SDHCX_SPEC_VER_3 ) {
				hc->caps	|= HC_CAP_ACMD23;
			}
		}
		else {
			hc->cfg.sg_max	= 1;
			sdhc->flags		|= SF_USE_SDMA;
		}
	}
#endif /* ADMA_SUPPORTED */	
	hc->caps	&= cfg->caps;		/* reconcile command line options */

	SIGEV_PULSE_INIT( &event, hc->hc_coid, SDIO_PRIORITY, HC_EV_INTR, NULL );
	if( ( hc->hc_iid = InterruptAttachEvent( cfg->irq[0], &event,
                                             _NTO_INTR_FLAGS_TRK_MSK ) ) == -1 ) {
		imx6_sdhcx_dinit( hc );
		return( errno );
	}

	/* Only enable the card insertion and removal interrupts */
    imx6_sdhcx_out32( base + IMX6_SDHCX_ISE, IMX6_SDHCX_ISE_DFLTS );
	imx6_sdhcx_out32( base + IMX6_SDHCX_IE, IMX6_SDHCX_INTR_CINS | IMX6_SDHCX_INTR_CREM );

	return( EOK );
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devb/sdmmc/sdiodi/hc/imx6.c $ $Rev: 809510 $")
#endif

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

#include "ipl.h"
#include "omap_sdhc.h"
#include <hw/inout.h>


/*--------------------------------------*/
/* OMAP SDMMC generci driver code		*/
/*--------------------------------------*/

/*
 * SDMMC command table
 */
static const struct cmd_str
{
	int			cmd;
	unsigned	sdmmc_cmd;
} cmdtab[] =
{
	// MMC_GO_IDLE_STATE
	{ 0,			(0 << 24) },
	// MMC_SEND_OP_COND (R3)
	{ 1,			(1 << 24) | CMD_RSP_TYPE_48 },
	// MMC_ALL_SEND_CID (R2)
	{ 2,			(2 << 24) | CMD_CCCE | CMD_RSP_TYPE_136 },
	// MMC_SET_RELATIVE_ADDR (R6)
	{ 3,			(3 << 24) | CMD_CICE | CMD_CCCE | CMD_RSP_TYPE_48 },
	// MMC_SWITCH (R1b)
	{ 6,			(6 << 24) | CMD_CICE | CMD_CCCE | CMD_RSP_TYPE_48b },
	// MMC_SEL_DES_CARD (R1b)
	{ 7,			(7 << 24) | CMD_CICE | CMD_CCCE | CMD_RSP_TYPE_48b },
	// MMC_IF_COND (R7)
	{ 8,			(8 << 24) | CMD_CICE | CMD_CCCE | CMD_RSP_TYPE_48 },
	// MMC_SEND_CSD (R2)
	{ 9,			(9 << 24) | CMD_CCCE | CMD_RSP_TYPE_136 },
	// MMC_SEND_STATUS (R1)
	{ 13,			(13 << 24) | CMD_CICE | CMD_CCCE | CMD_RSP_TYPE_48 },
	// MMC_SET_BLOCKLEN (R1)
	{ 16,			(16 << 24) | CMD_CICE | CMD_CCCE | CMD_RSP_TYPE_48 },
	// MMC_READ_SINGLE_BLOCK (R1)
	{ 17,			(17 << 24) | CMD_CICE | CMD_CCCE | CMD_RSP_TYPE_48 | CMD_DP | CMD_DE | CMD_DDIR },
	// MMC_READ_MULTIPLE_BLOCK (R1)
	{ 18,			(18 << 24) | CMD_CICE | CMD_CCCE | CMD_RSP_TYPE_48 | CMD_DP | CMD_DE | CMD_DDIR | CMD_MBS | CMD_BCE | CMD_ACEN },
	// MMC_APP_CMD (R1)
	{ 55,			(55 << 24) | CMD_CICE | CMD_CCCE | CMD_RSP_TYPE_48 },
	// SD_SET_BUS_WIDTH (R1)
	{ (55 << 8) | 6,	(6 << 24) | CMD_CICE | CMD_CCCE | CMD_RSP_TYPE_48 },
	// SD_SEND_OP_COND (R3)
	{ (55 << 8) | 41,	(41 << 24) | CMD_RSP_TYPE_48 },
	// end of command list
	{ -1 },
};

/* searches for a command in the command table */
static struct cmd_str *get_cmd(int cmd)
{
	struct cmd_str *command = (struct cmd_str *)&cmdtab[0];

	while (command->cmd != -1) {
		if (command->cmd == cmd) {
			return command;
		}
		command++;
	}

	return 0;
}

static int omap_wait_bits(sdmmc_t *sdmmc, int offset, int mask, int value)
{
/*
 * Even in the slowest OMAP SoC, this represents the time in the order of milli seconds,
 * which is more than enough for the SD/MMC controller
 */
#define LDELAY		100000

	unsigned base = sdmmc->hc->sdmmc_pbase;
	unsigned tmp;
	int count = LDELAY;

	do {
		tmp = in32(base + offset);

		/* Wait to be cleared */
		if (0 == value) {
			if (!(tmp & mask)) {
				return SDMMC_OK;
			}
		} else {
			/* Any bit is set */
			if (tmp & mask & value) {
				return SDMMC_OK;
			}
		}
	} while (count--);

	return SDMMC_ERROR;
}

/* sets the sdmmc clock frequency */
static void omap_set_frq(sdmmc_t *sdmmc, unsigned frq)
{
	sdmmc_hc_t	*hc = sdmmc->hc;
	unsigned	base = hc->sdmmc_pbase;
	unsigned	sysctl = in32(base + OMAP_MMCHS_SYSCTL);
	int clkd = 1;

	/* Stop clock */
	sysctl &= ~(SYSCTL_ICE | SYSCTL_CEN | SYSCTL_DTO_MASK);
	sysctl |= SYSCTL_DTO_MAX;
	out32(base + OMAP_MMCHS_SYSCTL, sysctl);

	/* Enable internal clock */
	sysctl &= ~(0x3FF << 6);
	sysctl |= SYSCTL_ICE;

	/* Find out the correct clock dividor */
	while (frq * clkd < hc->clock) {
		clkd += 1;
	}
	sysctl |= (clkd << 6);

	out32(base + OMAP_MMCHS_SYSCTL, sysctl);

	if (sdmmc->verbose > SDMMC_VERBOSE_LVL_0) {
		ser_putstr("Set clock to ");
		ser_putdec(frq);
		ser_putstr("KHz ref clock ");
		ser_putdec(hc->clock);
		ser_putstr("KHz sysctl 0x");
		ser_puthex(sysctl);
		ser_putstr("\n");
	}

	/* Wait for the clock to be stable */
	omap_wait_bits(sdmmc, OMAP_MMCHS_SYSCTL, SYSCTL_ICS, SYSCTL_ICS);

	/*
	 * Changing CLKD automatically causes SRC and SRD reset
	 * wait until data and clock lines back to normal
	 */
	omap_wait_bits(sdmmc, OMAP_MMCHS_SYSCTL, (SYSCTL_SRC | SYSCTL_SRD), 0);

	/* Enable clock to the card */
	out32(base + OMAP_MMCHS_SYSCTL, sysctl | SYSCTL_CEN);
}

/* sets the data bus width */
static void omap_set_bus_width(sdmmc_t *sdmmc, int width)
{
	unsigned	base = sdmmc->hc->sdmmc_pbase;
	unsigned	tmp = in32(base + OMAP_MMCHS_CON);

	if (sdmmc->verbose > SDMMC_VERBOSE_LVL_0) {
		ser_putstr("Bus width ");
		ser_putdec(width);
		ser_putstr("\n");
	}

	if (width == 8) {
		out32(base + OMAP_MMCHS_CON, tmp | CON_DW8);
	} else {
		out32(base + OMAP_MMCHS_CON, tmp & ~CON_DW8);
		tmp = in32(base + OMAP_MMCHS_HCTL);
		if (width == 4) {
			tmp |= HCTL_DTW4;
		} else {
			tmp &= ~HCTL_DTW4;
		}
		out32(base + OMAP_MMCHS_HCTL, tmp);
	}
}

/* MMC: Open Drain; SD: Push Pull */
static void omap_set_bus_mode(sdmmc_t *sdmmc, int mode)
{
	unsigned	base = sdmmc->hc->sdmmc_pbase;
	unsigned	tmp = in32(base + OMAP_MMCHS_CON) & ~CON_OD;

	if (mode == BUS_MODE_OPEN_DRAIN) {
		tmp |= CON_OD;
	}

	out32(base + OMAP_MMCHS_CON, tmp);
}

static void omap_set_timing(sdmmc_t *sdmmc, int timing)
{
	static const char *name[9] = { "INVALID", "LS", "HS", "HS DDR50", "SDR12", "SDR25", "SDR50", "SDR104", "HS200" };
	unsigned base = sdmmc->hc->sdmmc_pbase;
	unsigned tmp = in32(base + OMAP_MMCHS_CON);

	if (sdmmc->verbose > SDMMC_VERBOSE_LVL_0) {
		ser_putstr("Timing ");
		ser_putstr(name[timing]);
		ser_putstr("\n");
	}

	if (timing == TIMING_DDR50) {
		tmp |= CON_DDR;
		out32(base + OMAP_MMCHS_CON, tmp | CON_DW8);
		out32(base + OMAP_MMCHS_AC12, 0x00040000);
	}
	sdmmc->timing = timing;
}

/* initializes the SDMMC controller */
static int omap_init_ctrl(sdmmc_t *sdmmc)
{
	unsigned base = sdmmc->hc->sdmmc_pbase;

	sdmmc->icr = SD_SIC_VHS_27_36V;
	sdmmc->ocr = OCR_VDD_32_33 | OCR_VDD_33_34;

	/* Disable All interrupts */
	out32(base + OMAP_MMCHS_IE, 0);

	/* Software reset */
	out32(base + OMAP_MMCHS_SYSCONFIG, SYSCONFIG_SOFTRESET);
	omap_wait_bits(sdmmc, OMAP_MMCHS_SYSSTATUS, SYSSTATUS_RESETDONE, SYSSTATUS_RESETDONE);

	out32(base + OMAP_MMCHS_SYSCTL, SYSCTL_SRA);
	omap_wait_bits(sdmmc, OMAP_MMCHS_SYSCTL, SYSCTL_SRA, 0);

	out32(base + OMAP_MMCHS_HCTL, HCTL_SDVS3V0);
	out32(base + OMAP_MMCHS_CAPA, in32(base + OMAP_MMCHS_CAPA) | CAPA_VS3V3 | CAPA_VS3V0 | CAPA_VS1V8);
	out32(base + OMAP_MMCHS_CON, (3 << 9));

	omap_set_frq(sdmmc, 400);
	omap_set_bus_mode(sdmmc, BUS_MODE_OPEN_DRAIN);

	out32(base + OMAP_MMCHS_HCTL, HCTL_SDVS3V0 | HCTL_SDBP);
	out32(base + OMAP_MMCHS_ISE, 0x117F8033);
	out32(base + OMAP_MMCHS_IE, 0x117F8033);

	return SDMMC_OK;
}

/* clean up the SDMMC controller */
int omap_sdmmc_fini(sdmmc_t *sdmmc)
{
	unsigned base = sdmmc->hc->sdmmc_pbase;

	out32(base + OMAP_MMCHS_ISE, 0);
	out32(base + OMAP_MMCHS_IE, 0);
	out32(base + OMAP_MMCHS_STAT, 0x117F8033);
	out32(base + OMAP_MMCHS_SYSCTL, 0);

	return 0;
}

static void omap_reset_cmd(sdmmc_t *sdmmc)
{
	unsigned base = sdmmc->hc->sdmmc_pbase;
	sdmmc_cmd_t	*cmd = &sdmmc->cmd;

	cmd->erintsts = in32(base + OMAP_MMCHS_STAT);
	out32(base + OMAP_MMCHS_SYSCTL, in32(base + OMAP_MMCHS_SYSCTL) | SYSCTL_SRC);

	/*
	 * Wait until the SRC bit is set.
	 * This could be in no time or could take up to few micro seconds
	 */
	omap_wait_bits(sdmmc, OMAP_MMCHS_SYSCTL, SYSCTL_SRC, SYSCTL_SRC);

	/*
	 * It takes few micro seconds to clear the SRC bit.
	 * On J5/OMAP5/J6, it always takes more than 100 loops when SRC is cleared
	 */
	omap_wait_bits(sdmmc, OMAP_MMCHS_SYSCTL, SYSCTL_SRC, 0);
}

/* issues a command on the SDMMC bus */
static int omap_send_cmd(sdmmc_t *sdmmc)
{
	struct cmd_str *command;
	sdmmc_cmd_t	*cmd = &sdmmc->cmd;
	unsigned	base = sdmmc->hc->sdmmc_pbase;
	int			data_txf;

	if (0 == (command = get_cmd(cmd->cmd))) {
		ser_putstr("Failed to get SDMMC command\n");
		return SDMMC_ERROR;
	}

	if (sdmmc->verbose > SDMMC_VERBOSE_LVL_2) {
		ser_putstr("omap_send_cmd(), CMD");
		ser_putdec(command->cmd);
		ser_putstr(" ARG: 0x");
		ser_puthex(cmd->arg);
		ser_putstr("\n");
	}

	/* check if need data transfer */
	data_txf = command->sdmmc_cmd & CMD_DP;
	omap_wait_bits(sdmmc, OMAP_MMCHS_PSTATE, PSTATE_CMDI, 0);

	/* clear SDMMC status */
	out32(base + OMAP_MMCHS_STAT, 0x117F8033);

	if (data_txf) {
		/* For data trsnfer, need to wait for data line idle */
		omap_wait_bits(sdmmc, OMAP_MMCHS_PSTATE, (PSTATE_DLA | PSTATE_DATI), 0);

		/* setup Rx DMA channel for data read */
		sdmmc->hc->setup_dma(sdmmc);
	}

	/* setup the argument register and send command */
	out32(base + OMAP_MMCHS_ARG, cmd->arg);
	out32(base + OMAP_MMCHS_CMD, command->sdmmc_cmd);

	/* wait for command finish */
	omap_wait_bits(sdmmc, OMAP_MMCHS_STAT, (INTR_CC | INTR_ERRI), (INTR_CC | INTR_ERRI));

	/* check error status */
	if (in32(base + OMAP_MMCHS_STAT) & INTR_ERRI) {

		/* We know some expected errors in IDLE state */
		if (sdmmc->card.state != MMC_IDLE) {
			ser_putstr("omap_send_cmd() failed at CMD");
			ser_putdec(command->cmd);
			ser_putstr(" MMCHS_STAT: 0x");
			ser_puthex(in32(base + OMAP_MMCHS_STAT));
			ser_putstr("\n");
		}

		omap_reset_cmd(sdmmc);
		return SDMMC_ERROR;
	}

	/* get command responce */
	if (cmd->rsp != 0) {
		cmd->rsp[0] = in32(base + OMAP_MMCHS_RSP10);
		if ((command->sdmmc_cmd & CMD_RSP_TYPE_MASK) == CMD_RSP_TYPE_136) {
			cmd->rsp[1] = in32(base + OMAP_MMCHS_RSP32);
			cmd->rsp[2] = in32(base + OMAP_MMCHS_RSP54);
			cmd->rsp[3] = in32(base + OMAP_MMCHS_RSP76);
		}
	}

	if (data_txf) {
		sdmmc->hc->dma_done(sdmmc);
	}

	return SDMMC_OK;
}

static int omap_pio_read(sdmmc_t *sdmmc, void *buf, unsigned len)
{
	unsigned	base = sdmmc->hc->sdmmc_pbase;
	sdmmc_cmd_t	*cmd = &sdmmc->cmd;
	unsigned	*pbuf = (unsigned *)buf;

	if (omap_wait_bits(sdmmc, OMAP_MMCHS_PSTATE, (PSTATE_CMDI | PSTATE_DATI), 0)) {
		return SDMMC_ERROR;
	}

	/* setup PIO read */
	out32(base + OMAP_MMCHS_BLK, (1 << 16) | len);

	/* clear SDMMC status */
	out32(base + OMAP_MMCHS_STAT, 0x117F8033);

	/* setup the argument register and send command */
	out32(base + OMAP_MMCHS_ARG, cmd->arg);
	out32(base + OMAP_MMCHS_CMD, (cmd->cmd << 24) | CMD_CICE | CMD_DP | CMD_DDIR | CMD_CCCE | CMD_RSP_TYPE_48);

	/* wait for command finish */
	while (!(in32(base + OMAP_MMCHS_STAT) & (INTR_BRR | INTR_ERRI)))
		;

	/* check error status */
	if (in32(base + OMAP_MMCHS_STAT) & INTR_ERRI) {
		if (sdmmc->card.state != MMC_IDLE) {
			ser_putstr("omap_pio_read() failed. MMCHS_STAT: 0x");
			ser_puthex(in32(base + OMAP_MMCHS_STAT));
			ser_putstr("\n");
		}
		omap_reset_cmd(sdmmc);
		return SDMMC_ERROR;
	}

	/* get command responce */
	if (cmd->rsp != 0) {
		cmd->rsp[0] = in32(base + OMAP_MMCHS_RSP10);
	}

	/* now read from FIFO */
	for (; len > 0; len -= 4) {
		*pbuf++ = in32(base + OMAP_MMCHS_DATA);
	}

	if (len < MMCHS_FIFO_SIZE) {
		int	cnt;
		for (cnt = 2000; cnt; cnt--) {
			if (!(in32(base + OMAP_MMCHS_PSTATE) & PSTATE_RTA)) {
				break;
			}
		}
	}

	return SDMMC_OK;
}

/*--------------------------------------*/
/* EDMA engine driver					*/
/*--------------------------------------*/
static int omap_edma_done(sdmmc_t *sdmmc)
{
	omap_edma_ext_t *dma_ext = (omap_edma_ext_t *)sdmmc->hc->dma_ext;
	int chnl = dma_ext->dma_chnl;
	unsigned stat;

	/* Wait for the end of the transfer or error happens */
	do {
		stat = in32(sdmmc->hc->sdmmc_pbase + OMAP_MMCHS_STAT);
		if (stat & INTR_ERRI) {
			ser_putstr("SD/MMC transmission error. MMCHS_STAT: 0x\n");
			ser_puthex(stat);
			ser_putstr("\n");

			omap_reset_cmd(sdmmc);
			return SDMMC_ERROR;
		}
	} while (!omap3_edma_read_bit(dma_ext->dma_pbase, OMAP3_EDMA_IPR, chnl));

	/* Clear IPR bit */
	omap3_edma_set_bit(dma_ext->dma_pbase, OMAP3_EDMA_ICR, chnl);

	/* Disable this EDMA event */
	omap3_edma_set_bit(dma_ext->dma_pbase, OMAP3_EDMA_EECR, chnl);

	return SDMMC_OK;
}

static int omap_setup_edma_read(sdmmc_t *sdmmc)
{
	sdmmc_hc_t		*hc = sdmmc->hc;
	omap_edma_ext_t *dma_ext = (omap_edma_ext_t *)hc->dma_ext;
	int chnl = dma_ext->dma_chnl;
	edma_param *param;

	param = (edma_param *)(dma_ext->dma_pbase + EDMA_PARAM(dma_ext->dma_chnl));

	out32(hc->sdmmc_pbase + OMAP_MMCHS_BLK, (sdmmc->cmd.bcnt << 16) | sdmmc->cmd.bsize);

	/*
	 * In case there is a pending event
	 */
	omap3_edma_set_bit(dma_ext->dma_pbase, OMAP3_EDMA_ECR, chnl);

	/*
	 * Initialize Rx DMA channel
	 */
	param->opt = (0 << 23)		/* ITCCHEN = 0 */
				| (0 << 22)		/* TCCHEN = 0 */
				| (0 << 21)		/* ITCINTEN = 0*/
				| (1 << 20)		/* TCINTEN = 0*/
				| (chnl << 12)	/* TCC */
				| (0 << 11)		/* Normal completion */
				| (0 << 8)		/* 8 bits width */
				| (0 << 3)		/* PaRAM set is static */
				| (1 << 2)		/* AB-synchronizad */
				| (0 << 1)		/* Destination address increment mode */
				| (0 << 0);		/* Source address increment mode */

	param->src			= hc->sdmmc_pbase + OMAP_MMCHS_DATA;
	param->abcnt		= (128 << 16) | 4;
	param->dst			= (unsigned)sdmmc->cmd.dbuf;
	param->srcdstbidx	= (4 << 16) | 0;
	param->linkbcntrld	= 0xFFFF;
	param->srcdstcidx	= (512<< 16) | 0;
	param->ccnt			= sdmmc->cmd.bcnt;

	/*
	 * Enable event
	 */
	omap3_edma_set_bit(dma_ext->dma_pbase, OMAP3_EDMA_EESR, chnl);

	return SDMMC_OK;
}

static int omap_edma_maxblks(sdmmc_t *sdmmc)
{
	/* No limit */
	return (~0);
}

static void omap_sdmmc_edma(sdmmc_t *sdmmc, void *dma_ext)
{
	sdmmc->hc->dma_ext = dma_ext;
	sdmmc->hc->max_blks = omap_edma_maxblks;
	sdmmc->hc->setup_dma = omap_setup_edma_read;
	sdmmc->hc->dma_done = omap_edma_done;
}

/*--------------------------------------*/
/* ADMA engine driver					*/
/*--------------------------------------*/

/* setup ADMA for data read */
static int omap_setup_adma_read(sdmmc_t *sdmmc)
{
	unsigned		addr, len, chunk;
	unsigned		base = sdmmc->hc->sdmmc_pbase;
	omap_adma_ext_t *dma_ext = (omap_adma_ext_t *)sdmmc->hc->dma_ext;
	omap_adma32_t	*adma = dma_ext->adma_des;

	out32(base + OMAP_MMCHS_BLK, (sdmmc->cmd.bcnt << 16) | sdmmc->cmd.bsize);

	// use ADMA2
	out32(base + OMAP_MMCHS_CON, in32(base + OMAP_MMCHS_CON) | CON_MNS);
	out32(base + OMAP_MMCHS_HCTL, in32(base + OMAP_MMCHS_HCTL) | HCTL_DMAS_ADMA2);

	addr = (unsigned)sdmmc->cmd.dbuf;
	len = sdmmc->cmd.bcnt * sdmmc->cmd.bsize;

	if (sdmmc->verbose > SDMMC_VERBOSE_LVL_2) {
		ser_putstr("ADMA DST 0x");
		ser_puthex((int)sdmmc->cmd.dbuf);
		ser_putstr("\n");
	}

	while (1) {
		chunk = (len > ADMA_CHUNK_SIZE) ? ADMA_CHUNK_SIZE : len;
		adma->attr = ADMA2_VALID | ADMA2_TRAN;
		adma->addr = addr;
		adma->len = chunk;
		len -= chunk;

		if (len == 0) {
			adma->attr |= ADMA2_END;
			break;
		}

		addr += chunk;
		adma++;
	}

	out32(base + OMAP_MMCHS_ADMASAL, (unsigned)dma_ext->adma_des);
	return SDMMC_OK;
}

static int omap_adma_maxblks(sdmmc_t *sdmmc)
{
	return (MAX_BLKCNT);
}

static int omap_adma_done(sdmmc_t *sdmmc)
{
	unsigned	base = sdmmc->hc->sdmmc_pbase;
	unsigned	sts;

	/*
	 * Note: before calling this function, we already have the TC or ERR event
	 * so we don't have to wait for them again
	 * instead, we just need to wait for the DMA finish event only
	 */
	while (1) {
		while (!((sts = in32(base + OMAP_MMCHS_STAT)) & (INTR_TC | INTR_ERRI)));

		if (sts & INTR_ERRI) {
			omap_reset_cmd(sdmmc);
			return SDMMC_ERROR;
		}

		if (sts & INTR_TC) {
			break;
		}
	}

	return SDMMC_OK;
}

static void omap_sdmmc_adma(sdmmc_t *sdmmc, void *dma_ext)
{
	sdmmc->hc->dma_ext = dma_ext;
	sdmmc->hc->max_blks = omap_adma_maxblks;
	sdmmc->hc->setup_dma = omap_setup_adma_read;
	sdmmc->hc->dma_done = omap_adma_done;
}

/*--------------------------------------*/
/* SDMA engine driver					*/
/*--------------------------------------*/
/* setup DMA for data read */
static int omap_setup_sdma_read(sdmmc_t *sdmmc)
{
	sdmmc_hc_t		*hc = sdmmc->hc;
	omap_sdma_ext_t *dma_ext = (omap_sdma_ext_t *)hc->dma_ext;
	dma4_param	*param = (dma4_param *)(dma_ext->dma_pbase + DMA4_CCR(dma_ext->dma_chnl));

	if (omap_wait_bits(sdmmc, OMAP_MMCHS_PSTATE, PSTATE_DATI, 0)) {
		return SDMMC_ERROR;
	}

	out32(sdmmc->hc->sdmmc_pbase + OMAP_MMCHS_BLK, (sdmmc->cmd.bcnt << 16) | sdmmc->cmd.bsize);

	/* Clear all status bits */
	param->csr = 0x1FFE;
	param->cen = (sdmmc->cmd.bcnt * sdmmc->cmd.bsize) >> 2;
	param->cfn = 1;			// Number of frames
	param->cse = 1;
	param->cde = 1;
	param->cicr = 0;			// We don't want any interrupts

	/* setup receive SDMA channel */
	param->csdp = (2 << 0)	// DATA_TYPE = 0x2: 32 bit element
				| (0 << 2)	// RD_ADD_TRSLT = 0: Not used
				| (0 << 6)	// SRC_PACKED = 0x0: Cannot pack source data
				| (0 << 7)	// SRC_BURST_EN = 0x0: Cannot burst source
				| (0 << 9)	// WR_ADD_TRSLT = 0: Undefined
				| (0 << 13)	// DST_PACKED = 0x0: No packing
				| (3 << 14)	// DST_BURST_EN = 0x3: Burst at 16x32 bits
				| (1 << 16)	// WRITE_MODE = 0x1: Write posted
				| (0 << 18)	// DST_ENDIAN_LOCK = 0x0: Endianness adapt
				| (0 << 19)	// DST_ENDIAN = 0x0: Little Endian type at destination
				| (0 << 20)	// SRC_ENDIAN_LOCK = 0x0: Endianness adapt
				| (0 << 21);	// SRC_ENDIAN = 0x0: Little endian type at source
	param->ccr = DMA4_CCR_SYNCHRO_CONTROL(dma_ext->dma_rreq) // Synchro control bits
				| (1 << 5)	// FS = 1: Packet mode with BS = 0x1
				| (0 << 6)	// READ_PRIORITY = 0x0: Low priority on read side
				| (0 << 7)	// ENABLE = 0x0: The logical channel is disabled.
				| (0 << 8)	// DMA4_CCRi[8] SUSPEND_SENSITIVE = 0
				| (0 << 12)	// DMA4_CCRi[13:12] SRC_AMODE = 0x0: Constant address mode
				| (1 << 14)	// DMA4_CCRi[15:14] DST_AMODE = 0x1: Post-incremented address mode
				| (1 << 18)	// DMA4_CCRi[18] BS = 0x1: Packet mode with FS = 0x1
				| (1 << 24)	// DMA4_CCRi[24] SEL_SRC_DST_SYNC = 0x1: Transfer is triggered by the source. The packet element number is specified in the DMA4_CSFI register.
				| (0 << 25);	// DMA4_CCRi[25] BUFFERING_DISABLE = 0x0

	param->cssa = hc->sdmmc_pbase + OMAP_MMCHS_DATA;
	param->cdsa = (unsigned)sdmmc->cmd.dbuf;
	param->csf = sdmmc->cmd.bsize >> 2;

	/* Enable DMA event */
	param->ccr |= 1 << 7;

	return SDMMC_OK;
}

static int omap_sdma_done(sdmmc_t *sdmmc)
{
	sdmmc_hc_t		*hc = sdmmc->hc;
	unsigned		base = hc->sdmmc_pbase;
	omap_sdma_ext_t *dma_ext = (omap_sdma_ext_t *)hc->dma_ext;
	dma4_param	*param = (dma4_param *)(dma_ext->dma_pbase + DMA4_CCR(dma_ext->dma_chnl));

	while (1) {
		while (!(in32(base + OMAP_MMCHS_STAT) & (INTR_TC | INTR_ERRI)))
			;

		if (in32(base + OMAP_MMCHS_STAT) & INTR_ERRI) {
			omap_reset_cmd(sdmmc);
			return SDMMC_ERROR;
		}
		if (in32(base + OMAP_MMCHS_STAT) & INTR_TC) {
			break;
		}
	}
	while (param->cen != param->ccen)	// Make sure the DMA is done
		;

	return SDMMC_OK;
}

static int omap_sdma_maxblks(sdmmc_t *sdmmc)
{
	/* No limit */
	return (~0);
}

static void omap_sdmmc_sdma(sdmmc_t *sdmmc, void *dma_ext)
{
	sdmmc->hc->dma_ext = dma_ext;
	sdmmc->hc->max_blks = omap_sdma_maxblks;
	sdmmc->hc->setup_dma = omap_setup_sdma_read;
	sdmmc->hc->dma_done = omap_sdma_done;
}

/*--------------------------------------*/
/* OMAP Host Controller					*/
/*--------------------------------------*/
static sdmmc_hc_t omap_hc;

void omap_sdmmc_init_hc(sdmmc_t *sdmmc, unsigned base, unsigned clock, int verbose, int dma, void *dma_ext)
{
	omap_hc.set_clk        = omap_set_frq;
	omap_hc.set_bus_width  = omap_set_bus_width;
	omap_hc.set_bus_mode   = omap_set_bus_mode;
	omap_hc.set_timing     = omap_set_timing;
	omap_hc.send_cmd       = omap_send_cmd;
	omap_hc.pio_read       = omap_pio_read;
	omap_hc.init_hc        = omap_init_ctrl;
	omap_hc.dinit_hc       = omap_sdmmc_fini;
	omap_hc.signal_voltage = 0;
	omap_hc.tuning         = 0;
	omap_hc.sdmmc_pbase    = base;
	omap_hc.clock          = clock;

	sdmmc->hc              = &omap_hc;
	sdmmc->verbose         = verbose;
	sdmmc->caps            = 0;

	switch (dma) {
	case OMAP_SDMMC_ADMA:
		omap_sdmmc_adma(sdmmc, dma_ext);
		break;
	case OMAP_SDMMC_EDMA:
		omap_sdmmc_edma(sdmmc, dma_ext);
		break;
	case OMAP_SDMMC_SDMA:
		omap_sdmmc_sdma(sdmmc, dma_ext);
		break;
	case OMAP_SDMMC_DMA_NONE:
	default:
		break;
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/arm/omap_sdmmc.c $ $Rev: 798438 $")
#endif

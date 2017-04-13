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
#include "sdmmc.h"
#include <hw/inout.h>

/*
 * SDMMC card power up wait loop
 * On J5ECO, one CMD1 takes ~500us @ 400KHz, while according to JEDEC,
 * a non-partitioned eMMC device could take up to 1 second from receiving
 * the first CMD until it's fully powered up, thus we chose 4000 tries,
 * approximately timeout at 2 seconds.
 */
#define POWER_UP_WAIT				40000

/* Gets the card state */
static int sdmmc_get_state(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;
	unsigned	rsp;

	CMD_CREATE (sdmmc->cmd, MMC_SEND_STATUS, RELATIVE_CARD_ADDR(sdmmc), &rsp, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	/* Bits 9-12 define the CURRENT_STATE */
	sdmmc->card.state = rsp & MMC_CURRENT_STATE;
	return SDMMC_OK;
}

static unsigned sdmmc_tran_speed(unsigned char ts)
{
	static const unsigned sdmmc_ts_exp[] = { 100, 1000, 10000, 100000, 0, 0, 0, 0 };
	static const unsigned sdmmc_ts_mul[] = { 0, 1000, 1200, 1300, 1500, 2000, 2500, 3000,
										3500, 4000, 4500, 5000, 5500, 6000, 7000, 8000 };

	return sdmmc_ts_exp[(ts & 0x7)] * sdmmc_ts_mul[(ts & 0x78) >> 3];
}

static void sd_parse_cid(sdmmc_t *sdmmc, unsigned *resp)
{
	sdmmc_cid_t *cid = &sdmmc->cid;

	cid->mid = (resp[3] >> 24) & 0xFF;
	cid->oid = (resp[3] >> 8) & 0xFFFF;
	cid->pnm[0] = resp[3];
	cid->pnm[1] = resp[2] >> 24;
	cid->pnm[2] = resp[2] >> 16;
	cid->pnm[3] = resp[2] >> 8;
	cid->pnm[4] = resp[2];
	cid->pnm[5] = 0;
	cid->prv = (resp[1] >> 24) & 0xFF;
	cid->psn = (resp[1] << 8) | (resp[0] >> 24);
	cid->mdt = (resp[0] >> 8) & 0xFFF;
}

static void mmc_parse_cid(sdmmc_t *sdmmc, unsigned *resp)
{
	sdmmc_cid_t	*cid = &sdmmc->cid;
	sdmmc_csd_t *csd = &sdmmc->csd;

	cid->pnm[0] = resp[3];
	cid->pnm[1] = resp[2] >> 24;
	cid->pnm[2] = resp[2] >> 16;
	cid->pnm[3] = resp[2] >> 8;
	cid->pnm[4] = resp[2];
	cid->pnm[5] = resp[1] >> 24;
	cid->mdt = (resp[0] >> 8) & 0xFF;

	if (csd->spec_vers < 2) {
		cid->mid = (resp[3] >> 8) & 0xFFFFFF;
		cid->oid = 0;
		cid->prv = (resp[1] >> 8) & 0xFF;;
		cid->psn = 0;
		cid->pnm[6] = resp[1] >> 16;
		cid->pnm[7] = 0;
	} else {
		cid->mid = (resp[3] >> 24) & 0xFF;
		cid->oid = (resp[3] >> 8) & 0xFF;
		cid->prv = (resp[1] >> 16) & 0xFF;;
		cid->psn = ((resp[1] & 0xFFFF) << 16) | (resp[0] >> 16);
		cid->pnm[6] = 0;
	}
}

static void mmc_parse_ext_csd(sdmmc_t *sdmmc, unsigned char *raw_ecsd)
{
	mmc_ecsd_t *ecsd = &sdmmc->mmc_ecsd;
	card_t *card = &sdmmc->card;

	ecsd->card_type = raw_ecsd[ECSD_CARD_TYPE] & ECSD_CARD_TYPE_MSK;
	ecsd->ext_csd_rev = raw_ecsd[ECSD_REV];

	if (ecsd->card_type & ECSD_CARD_TYPE_52MHZ) {
		card->speed = DTR_MAX_HS52;
	} else {
		card->speed = DTR_MAX_HS26;
	}

	if (sdmmc->verbose) {
		ser_putstr("Ext CSD:\n");
		ser_putstr("	CARD_TYPE 0x"); ser_puthex(ecsd->card_type);
		ser_putstr("	EXT_CSD_REV 0x"); ser_puthex(ecsd->ext_csd_rev); ser_putstr("\n");
		ser_putstr("	DTR "); ser_putdec(card->speed); ser_putstr("\n");
	}
}

static void sd_parse_csd(sdmmc_t *sdmmc, unsigned *resp)
{
	card_t *card = &sdmmc->card;
	sdmmc_csd_t *csd = &sdmmc->csd;
	int	bsize, csize, csizem;

	csd->csd_structure = resp[3] >> 30;
	csd->tran_speed	= resp[3];
	csd->ccc = resp[2] >> 20;
	csd->read_bl_len = (resp[2] >> 16) & 0x0F;

	if (csd->csd_structure == 0) {
		csd->c_size = ((resp[2] & 0x3FF) << 2) | (resp[1] >> 30);
		csd->c_size_mult = (resp[1] >> 15) & 0x07;
	} else {
		csd->c_size = ((resp[2] & 0x3F) << 16) | (resp[1] >> 16);
	}

	if (csd->csd_structure == 0) {
		bsize = 1 << csd->read_bl_len;
		csize = csd->c_size + 1;
		csizem = 1 << (csd->c_size_mult + 2);
	} else {
		bsize = SDMMC_BLOCKSIZE;
		csize = csd->c_size + 1;
		csizem = 1024;
	}

	/* force to 512 byte block */
	if (bsize > SDMMC_BLOCKSIZE && (bsize % SDMMC_BLOCKSIZE) == 0) {
		unsigned ts = bsize / SDMMC_BLOCKSIZE;
		csize = csize * ts;
		bsize = SDMMC_BLOCKSIZE;
	}

	card->blk_size = bsize;
	card->blk_num = csize * csizem;
	card->speed	= sdmmc_tran_speed(csd->tran_speed) / 1000;
}

static void mmc_parse_csd(sdmmc_t *sdmmc, unsigned *resp)
{
	card_t	*card = &sdmmc->card;
	sdmmc_csd_t *csd = &sdmmc->csd;
	int bsize, csize, csizem;

	csd->csd_structure = resp[3] >> 30;
	csd->spec_vers = (resp[3] >> 26) & 0x0F;
	csd->tran_speed	= resp[3];
	csd->ccc = (resp[2] >> 20) & 0xFFF;
	csd->read_bl_len = (resp[2] >> 16) & 0x0F;
	csd->c_size = ((resp[2] & 0x3FF) << 2) | (resp[1] >> 30);
	csd->c_size_mult = (resp[1] >> 15) & 0x07;

	bsize = 1 << csd->read_bl_len;
	csize = csd->c_size + 1;
	csizem = 1 << (csd->c_size_mult + 2);

	if (sdmmc->csd.spec_vers < CSD_SPEC_VER_4) {
		sdmmc->caps &= ~SDMMC_CAP_UHS;
	}

	/* force to 512 byte block */
	if (bsize > SDMMC_BLOCKSIZE && (bsize % SDMMC_BLOCKSIZE) == 0) {
		unsigned ts = bsize / SDMMC_BLOCKSIZE;
		csize = csize * ts;
		bsize = SDMMC_BLOCKSIZE;
	}

	card->blk_size = bsize;
	card->blk_num = csize * csizem;
}

static int sdmmc_go_idle(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;

	CMD_CREATE (sdmmc->cmd, MMC_GO_IDLE_STATE, 0, 0, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}
	return SDMMC_OK;
}

static int sdmmc_send_if_cond(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;

	CMD_CREATE (sdmmc->cmd, MMC_IF_COND, sdmmc->icr, 0, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}
	return SDMMC_OK;
}

/* Check OCR for comliance with controller OCR */
static int sdmmc_check_ocr(sdmmc_t *sdmmc, unsigned ocr)
{
	if (ocr & sdmmc->ocr) {
		return SDMMC_OK;
	}
	return sdmmc_go_idle(sdmmc);
}

static int mmc_send_op_cond(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;
	card_t	*card = &sdmmc->card;
	unsigned rsp[4];
	int i;

	CMD_CREATE (sdmmc->cmd, MMC_SEND_OP_COND, OCR_PWRUP_CMP | OCR_HCS | sdmmc->ocr, rsp, 0, 0, 0);
	for (i = 0; i < POWER_UP_WAIT; i++) {
		if (SDMMC_OK != hc->send_cmd(sdmmc)) {
			return SDMMC_ERROR;
		}

		if ((i == 0) && (SDMMC_OK != sdmmc_check_ocr(sdmmc, rsp[0]))) {
			ser_putstr("Failed to power up MMC card\n");
			return SDMMC_ERROR;
		}

		if (rsp[0] & OCR_PWRUP_CMP) {
			break;
		}
	}

	/* check if time out */
	if (i >= POWER_UP_WAIT) {
		ser_putstr("Failed to power up MMC card\n");
		return SDMMC_ERROR;
	}

	/* test for HC bit set */
	if (rsp[0] & OCR_HCS) {
		card->type = eMMC_HC;
	}

	return SDMMC_OK;
}

static int sd_send_op_cond(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;
	card_t	*card = &sdmmc->card;
	unsigned rsp[4];
	int i;

	for (i = 0; i < POWER_UP_WAIT; i++) {
		/* when ACMDx(SD_SEND_OP_COND) is to be issued, send CMD55 first */
		CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, RELATIVE_CARD_ADDR(sdmmc), 0, 0, 0, 0);
		if (SDMMC_OK != hc->send_cmd(sdmmc)) {
			return SDMMC_ERROR;
		}

		CMD_CREATE (sdmmc->cmd, SD_SEND_OP_COND, OCR_PWRUP_CMP | OCR_HCS | sdmmc->ocr, rsp, 0, 0, 0);
		if (SDMMC_OK != hc->send_cmd(sdmmc)) {
			return SDMMC_ERROR;
		}

		if ((i == 0) && (SDMMC_OK != sdmmc_check_ocr(sdmmc, rsp[0]))) {
			return SDMMC_ERROR;
		}

		if (rsp[0] & OCR_PWRUP_CMP) {
			break;
		}
	}

	/* check if time out */
	if (i >= POWER_UP_WAIT) {
		ser_putstr("Failed to power up SD card\n");
		return SDMMC_ERROR;
	}

	/* test for HC bit set */
	if (rsp[0] & OCR_HCS) {
		card->type = eSDC_HC;
	} else {
		card->type = eSDC;
	}

	return SDMMC_OK;
}

static int sdmmc_send_all_cid(sdmmc_t *sdmmc, unsigned *cid_raw)
{
	sdmmc_hc_t *hc = sdmmc->hc;

	CMD_CREATE (sdmmc->cmd, MMC_ALL_SEND_CID, 0, cid_raw, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	return SDMMC_OK;
}

static void sdmmc_parse_cid(sdmmc_t *sdmmc, unsigned *cid_raw)
{
	card_t *card = &sdmmc->card;
	sdmmc_cid_t	*cid = &sdmmc->cid;

	if (IS_SD_CARD(card)) {
		sd_parse_cid(sdmmc, cid_raw);
	} else {
		mmc_parse_cid(sdmmc, cid_raw);
	}

	if (sdmmc->verbose) {
		ser_putstr("Card IDentification:\n");
		ser_putstr("	MID 0x"); ser_puthex(cid->mid);
		ser_putstr("	OID 0x"); ser_puthex(cid->oid);
		ser_putstr("	PNM "); ser_putstr((const char *)cid->pnm); ser_putstr("\n");
		ser_putstr("	PRV 0x"); ser_puthex(cid->prv);
		ser_putstr("	PSN 0x"); ser_puthex(cid->psn); ser_putstr("\n");
	}
}

static int sdmmc_read_parse_csd(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;
	sdmmc_csd_t *csd = &sdmmc->csd;
	card_t	*card = &sdmmc->card;
	unsigned rsp[4];

	CMD_CREATE (sdmmc->cmd, MMC_SEND_CSD, RELATIVE_CARD_ADDR(sdmmc), rsp, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	if (IS_SD_CARD(card)) {
		sd_parse_csd(sdmmc, rsp);
	} else {
		mmc_parse_csd(sdmmc, rsp);
	}

	if (sdmmc->verbose) {
		ser_putstr("CSD:\n");
		ser_putstr("	CSD_STRUCTURE "); ser_putdec(csd->csd_structure);

		if (IS_EMMC_CARD(card)) {
			ser_putstr("	SPEC_VERS "); ser_putdec(csd->spec_vers);
		}

		ser_putstr("	CCC 0x"); ser_puthex(csd->ccc); ser_putstr("\n");
		ser_putstr("	TRAN_SPEED "); ser_putdec(csd->tran_speed);
		ser_putstr("	READ_BL_LEN "); ser_puthex(csd->read_bl_len); ser_putstr("\n");
		ser_putstr("	C_SIZE "); ser_putdec(csd->c_size);
		ser_putstr("	C_SIZE_MULT "); ser_putdec(csd->c_size_mult); ser_putstr("\n");
		ser_putstr("	blksz "); ser_putdec(card->blk_size);
		ser_putstr("	sectors "); ser_putdec(card->blk_num); ser_putstr("\n");
	}
	return SDMMC_OK;
}

static int sdmmc_set_relative_addr(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;
	card_t	*card = &sdmmc->card;
	unsigned rsp[4];

	card->rca = 0x0001;
	CMD_CREATE (sdmmc->cmd, MMC_SET_RELATIVE_ADDR, RELATIVE_CARD_ADDR(sdmmc), rsp, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	if (IS_SD_CARD(card)) {
		card->rca = rsp[0] >> 16;
	}

	if (SDMMC_OK != sdmmc_get_state(sdmmc)) {
		return SDMMC_ERROR;
	}

	return SDMMC_OK;
}

static int sdmmc_select_card(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;

	/* Select the Card */
	CMD_CREATE (sdmmc->cmd, MMC_SEL_DES_CARD, RELATIVE_CARD_ADDR(sdmmc), 0, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	if (SDMMC_OK != sdmmc_get_state(sdmmc)) {
		return SDMMC_ERROR;
	}

	return SDMMC_OK;
}

static int mmc_switch(sdmmc_t *sdmmc, int mode, int index, int value, int cmdset)
{
	sdmmc_hc_t *hc = sdmmc->hc;

	CMD_CREATE (sdmmc->cmd, MMC_SWITCH, (mode << MMC_SWITCH_MODE_OFF)
									| (index << MMC_SWITCH_INDEX_OFF)
									| (value << MMC_SWITCH_VALUE_OFF) | cmdset, 0, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	do {
		if (SDMMC_OK != sdmmc_get_state(sdmmc)) {
			return SDMMC_ERROR;
		}
	} while (MMC_PRG == sdmmc->card.state);

	return SDMMC_OK;
}

static int sd_switch(sdmmc_t *sdmmc, int mode, int group, int value, unsigned char *buf)
{
	sdmmc_hc_t *hc = sdmmc->hc;
	unsigned arg;

	/* By default group function is all "1" */
	arg = (mode << SD_SF_MODE_OFF) | 0x00ffffff;
	arg &= ~(SD_SF_GRP_DFT_VALUE << (group * SD_SF_GRP_WIDTH));
	arg |= (value << (group * SD_SF_GRP_WIDTH));

	CMD_CREATE (sdmmc->cmd, MMC_SWITCH, arg, 0, 0, 0, 0);
	if (SDMMC_OK != hc->pio_read(sdmmc, buf, SD_SF_STATUS_SIZE)) {
		return SDMMC_ERROR;
	}

	do {
		if (SDMMC_OK != sdmmc_get_state(sdmmc)) {
			return SDMMC_ERROR;
		}
	} while (MMC_PRG == sdmmc->card.state);

	return SDMMC_OK;
}

static int mmc_init_hs(sdmmc_t *sdmmc)
{
	card_t	*card = &sdmmc->card;
	sdmmc_hc_t *hc = sdmmc->hc;

	if (mmc_switch(sdmmc, MMC_SWITCH_MODE_WRITE, ECSD_HS_TIMING, ECSD_HS_TIMING_HS, MMC_SWITCH_CMDSET_DFLT)) {
		ser_putstr("Can't switch to High Speed mode\n");
		return SDMMC_ERROR;
	}

	if (mmc_switch(sdmmc, MMC_SWITCH_MODE_WRITE, ECSD_BUS_WIDTH, ECSD_BUS_WIDTH_8_BIT, MMC_SWITCH_CMDSET_DFLT)) {
		ser_putstr("Can't switch to 8 bit mode\n");
		return SDMMC_ERROR;
	}

	hc->set_bus_width(sdmmc, 8);
	hc->set_timing(sdmmc, TIMING_HS);
	hc->set_clk(sdmmc, card->speed);
	return SDMMC_OK;
}

static int mmc_init_ddr(sdmmc_t *sdmmc)
{
	card_t	*card = &sdmmc->card;
	sdmmc_hc_t *hc = sdmmc->hc;

	/* switch to 8 bit mode, DDR */
	if (mmc_switch(sdmmc, MMC_SWITCH_MODE_WRITE, ECSD_BUS_WIDTH, ECSD_BUS_WIDTH_DDR_8_BIT, MMC_SWITCH_CMDSET_DFLT)) {
		ser_putstr("Can't switch to 8 bit mode\n");
		return SDMMC_ERROR;
	}

	hc->set_timing(sdmmc, TIMING_DDR50);
	hc->set_clk(sdmmc, card->speed);
	return SDMMC_OK;
}

static int sd_read_parse_scr(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;
	sd_scr_t *scr = &sdmmc->scr;
	unsigned char buf[64];

	CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, RELATIVE_CARD_ADDR(sdmmc), 0, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	CMD_CREATE (sdmmc->cmd, MMC_SEND_SCR, 0, 0, 0, 0, 0);
	if (SDMMC_OK != hc->pio_read(sdmmc, buf, SD_SCR_SIZE)) {
		return SDMMC_ERROR;
	}

	scr->scr_structure = (buf[0] >> 4) & 0xF;
	scr->sd_spec = buf[0] & 0xF;
	scr->sd_bus_widths = buf[1] & 0xF;
	scr->sd_spec3 = (buf[2] >> 7) & 0x1;

	if (scr->sd_spec < CSD_SPEC_VER_1) {
		ser_putstr("CSR: SD version < 1, no high speed support\n");
		sdmmc->card.speed = DTR_MAX_SDR12;
	}

	if (sdmmc->verbose) {
		ser_putstr("SD Card Configuration:\n");
		ser_putstr("	SCR_STRUCTURE "); ser_putdec(scr->scr_structure);
		ser_putstr("	SD_SPEC "); ser_putdec(scr->sd_spec); ser_putstr("\n");
		ser_putstr("	SD_BUS_WIDTHS "); ser_putdec(scr->sd_bus_widths);
		ser_putstr("	SD_SPEC3 "); ser_putdec(scr->sd_spec3); ser_putstr("\n");
	}
	return SDMMC_OK;
}

static int sd_capabilities(sdmmc_t *sdmmc)
{
	card_t	*card = &sdmmc->card;
	sd_scr_t *scr = &sdmmc->scr;
	unsigned char buf[64];

	if (sd_read_parse_scr(sdmmc)) {
		ser_putstr("sd_capabilities: read SCR failed\n");
		return SDMMC_ERROR;
	}

	/* Get the current bus speed */
	if (sd_switch(sdmmc, SD_SF_MODE_CHECK, SD_SF_GRP_BUS_SPD, SD_SF_GRP_DFT_VALUE, buf)) {
		ser_putstr("sd_capabilities: check mode failed\n");
		return SDMMC_ERROR;
	}

	/* SDHC card */
	if ((buf[13] & 0x2 ) ) {
		sdmmc->card.speed = DTR_MAX_SDR25;
	} else {
		ser_putstr("sd_capabilities: No high speed support\n");
		sdmmc->card.speed = DTR_MAX_SDR12;
	}

	if (scr->sd_spec3 ) {
		card->bus_mode = buf[13] & SD_BUS_MODE_MSK;
		card->drv_type = buf[9] & SD_DRV_TYPE_MSK;
		card->curr_limit = buf[7] & SD_CURR_LIMIT_MSK;

		if (sdmmc->verbose > SDMMC_VERBOSE_LVL_0) {
			ser_putstr("SD: bus_mode 0x"); ser_puthex(card->bus_mode); ser_putstr("\n");
			ser_putstr("SD: drv_type 0x"); ser_puthex(card->drv_type); ser_putstr("\n");
			ser_putstr("SD: curr_limit 0x"); ser_puthex(card->curr_limit); ser_putstr("\n");
		}
	} else {
		sdmmc->caps &= ~SDMMC_CAP_UHS;
	}

	return SDMMC_OK;
}

static int sd_init_bus(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;
	card_t	*card = &sdmmc->card;
	unsigned char tmp[SD_SF_STATUS_SIZE];

	/* when ACMDx(SD_SEND_OP_COND) is to be issued, send CMD55 first */
	CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, RELATIVE_CARD_ADDR(sdmmc), 0, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	/* switch to 4 bits bus */
	CMD_CREATE (sdmmc->cmd, SD_SET_BUS_WIDTH, 2, 0, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		ser_putstr("Failed to switch to bus width 4\n");
		return SDMMC_ERROR;
	}

	hc->set_bus_width(sdmmc, 4);

	/* Set bus speed to HS */
	if (sdmmc->card.speed == DTR_MAX_SDR25) {
		if (sd_switch(sdmmc, SD_SF_MODE_SET, SD_SF_GRP_BUS_SPD, 1, tmp)) {
			ser_putstr("Set high speed failed\n");
			return SDMMC_ERROR;
		}

		if ((tmp[16] & 0x0F) == 1) {
			if (sdmmc->verbose >= SDMMC_VERBOSE_LVL_1) {
				ser_putstr("Switch to high speed mode successful\n");
			}
		}
	}

	hc->set_timing(sdmmc, TIMING_HS);
	hc->set_clk(sdmmc, card->speed);

	return SDMMC_OK;
}

static int mmc_init_bus(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;

	/* If UHS is not needed, we don't bother to check ESCD */
	if (!(sdmmc->caps & SDMMC_CAP_SKIP_IDENT) && (sdmmc->caps & SDMMC_CAP_UHS)) {
		unsigned char raw_ecsd[MMC_EXT_CSD_SIZE];

		/* Get Ext CSD */
		CMD_CREATE (sdmmc->cmd, MMC_SEND_EXT_CSD, 0, 0, 0, 0, 0);
		if (SDMMC_OK != hc->pio_read(sdmmc, raw_ecsd, MMC_EXT_CSD_SIZE)) {
			ser_putstr("Failed to read ECSD register\n");
			return SDMMC_ERROR;
		}

		mmc_parse_ext_csd(sdmmc, raw_ecsd);
	}

	if (mmc_init_hs(sdmmc)) {
		return SDMMC_ERROR;
	}

	if (sdmmc->caps & SDMMC_CAP_DDR) {
		return mmc_init_ddr(sdmmc);
	}

	return SDMMC_OK;
}

/*
 * eMMC card could take very long time to power itself up
 * We can start power it up in IPL and do the remaining initialization sometimes later
 * i.e. in startup or even wait until Neutrino is up and running
 */
int sdmmc_powerup_card(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;

	if (SDMMC_OK != hc->init_hc(sdmmc)) {
		ser_putstr("SDMMC interface init failed\n");
		return SDMMC_ERROR;
	}

	if (sdmmc_go_idle(sdmmc)) {
		return SDMMC_ERROR;
	}

	CMD_CREATE (sdmmc->cmd, MMC_SEND_OP_COND, OCR_PWRUP_CMP | OCR_HCS | sdmmc->ocr, 0, 0, 0, 0);
	return hc->send_cmd(sdmmc);
}

/* SDMMC card identification and initialisation */
static int sdmmc_init_card(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;
	card_t	*card = &sdmmc->card;
	unsigned cid_raw[4];

	/* initial relative card address */
	card->rca = 0;
	card->state = MMC_IDLE;

	hc->set_bus_mode(sdmmc, BUS_MODE_OPEN_DRAIN);

	/* Probe for SDC card */
	if (sdmmc_go_idle(sdmmc)) {
		return SDMMC_ERROR;
	}

	if (!IS_EMMC_CARD(card)) {
		if (SDMMC_OK == sdmmc_send_if_cond(sdmmc)) {
			card->type = eSDC_V200;

			/* eSDC_V200: need to send MMC_IF_COND again */
			if (sdmmc_send_if_cond(sdmmc)) {
				return SDMMC_ERROR;
			}
		} else {
			CMD_CREATE (sdmmc->cmd, MMC_APP_CMD, 0, 0, 0, 0, 0);
			if (SDMMC_OK == hc->send_cmd(sdmmc)) {
				card->type = eSDC;
			} else {
				card->type = eMMC;
			}
		}
	}

	/*
	 * This state is not the exact meaning of what the SD/MMC spec states
	 * The only purpose of this statement is to let the driver know
	 * that before this, there could be some expected errors
	 */
	card->state = MMC_READY;

	if (IS_EMMC_CARD(card)) {
		if (mmc_send_op_cond(sdmmc)) {
			return SDMMC_ERROR;
		}
	} else {
		if (sd_send_op_cond(sdmmc)) {
			return SDMMC_ERROR;
		}
	}

	if (sdmmc_send_all_cid(sdmmc, cid_raw)) {
		return SDMMC_ERROR;
	}

	if (sdmmc_set_relative_addr(sdmmc)) {
		return SDMMC_ERROR;
	}

	hc->set_bus_mode(sdmmc, BUS_MODE_PUSH_PULL);

	if (sdmmc_read_parse_csd(sdmmc)) {
		return SDMMC_ERROR;
	}

	if (!(sdmmc->caps & SDMMC_CAP_SKIP_IDENT)) {
		sdmmc_parse_cid(sdmmc, cid_raw);
	}

	if (sdmmc_select_card(sdmmc)) {
		return SDMMC_ERROR;
	}

	/* Set block size in case the default blocksize differs from 512 */
	card->blk_num *= card->blk_size / SDMMC_BLOCKSIZE;
	card->blk_size = SDMMC_BLOCKSIZE;

	CMD_CREATE (sdmmc->cmd, MMC_SET_BLOCKLEN, card->blk_size, 0, 0, 0, 0);
	if (SDMMC_OK != hc->send_cmd(sdmmc)) {
		return SDMMC_ERROR;
	}

	if (SDMMC_OK != sdmmc_get_state(sdmmc)) {
		return SDMMC_ERROR;
	}

	if (IS_SD_CARD(card)) {
		if (sd_capabilities(sdmmc)) {
			return SDMMC_ERROR;
		}

		return sd_init_bus(sdmmc);
	}

	return mmc_init_bus(sdmmc);
}

int sdmmc_init_sd(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;

	if (SDMMC_OK != hc->init_hc(sdmmc)) {
		ser_putstr("SDMMC interface init failed\n");
		return SDMMC_ERROR;
	}

	card_t	*card = &sdmmc->card;
	card->type = NONE;

	/* Default speed */
	sdmmc->card.speed = DTR_MAX_SDR25;

	if (sdmmc_init_card(sdmmc)) {
		ser_putstr("SD/MMC card init failed\n");
		hc->dinit_hc(sdmmc);
		return SDMMC_ERROR;
	}

	return SDMMC_OK;
}

int sdmmc_init_mmc(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;

	if (SDMMC_OK != hc->init_hc(sdmmc)) {
		ser_putstr("SDMMC interface init failed\n");
		return SDMMC_ERROR;
	}

	card_t	*card = &sdmmc->card;
	card->type = eMMC;

	/* Default speed */
	card->speed = DTR_MAX_HS52;

	if (sdmmc_init_card(sdmmc)) {
		ser_putstr("SD/MMC card init failed\n");
		hc->dinit_hc(sdmmc);
		return SDMMC_ERROR;
	}

	return SDMMC_OK;
}


static int _sdmmc_read_write(void *ext, void *buf, unsigned blkno, unsigned blkcnt, int read){
	int cmd;
	unsigned arg;
	unsigned cblkcnt, addr;
	sdmmc_t	*sdmmc = (sdmmc_t *)ext;
	sdmmc_hc_t	*hc = sdmmc->hc;

	if (sdmmc->verbose > SDMMC_VERBOSE_LVL_1) {
		ser_putstr("sdmmc_read_write(), blkno: ");
		ser_putdec(blkno);
		ser_putstr(" blkcnt: ");
		ser_putdec(blkcnt);
		ser_putstr("\n");
	}

	if ((blkno >= sdmmc->card.blk_num) || ((blkno + blkcnt) > sdmmc->card.blk_num) || ((blkno + blkcnt) < blkno)) {
		return SDMMC_ERROR;
	}

	addr = (unsigned)buf;

	while (1) {
		/* Some controller has restriction on the maximum blks per transaction */
		cblkcnt = (blkcnt > hc->max_blks(sdmmc)) ? hc->max_blks(sdmmc): blkcnt;

		if (cblkcnt == 0) {
			return SDMMC_OK;
		}

		if (cblkcnt == 1) {
			cmd = MMC_READ_SINGLE_BLOCK;
		} else {
			cmd = MMC_READ_MULTIPLE_BLOCK;
		}

		arg = blkno;

		if ((sdmmc->card.type == eMMC) || (sdmmc->card.type == eSDC)) {
			arg *= SDMMC_BLOCKSIZE;
		}

		CMD_CREATE (sdmmc->cmd, cmd, arg, 0, SDMMC_BLOCKSIZE, cblkcnt, (void *)addr);
		if (SDMMC_OK != hc->send_cmd(sdmmc)) {
			return SDMMC_ERROR;
		}

		while (sdmmc->card.state != MMC_TRAN) {
			if (SDMMC_OK != sdmmc_get_state(sdmmc)) {
				return SDMMC_ERROR;
			}
		}

		blkcnt -= cblkcnt;
		blkno += cblkcnt;
		addr += cblkcnt * SDMMC_BLOCKSIZE;
	}

	return SDMMC_OK;
}

/* read blocks from SDMMC */
int sdmmc_read(void *ext, void *buf, unsigned blkno, unsigned blkcnt)
{
	return (_sdmmc_read_write(ext, buf, blkno, blkcnt, 1));
}

/* write blocks to SDMMC */
int sdmmc_write(void *ext, void *buf, unsigned blkno, unsigned blkcnt)
{
	return (_sdmmc_read_write(ext, buf, blkno, blkcnt, 0));
}

int sdmmc_fini(sdmmc_t *sdmmc)
{
	sdmmc_hc_t *hc = sdmmc->hc;
	return (hc->dinit_hc(sdmmc));
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/sdmmc/sdmmc.c $ $Rev: 808052 $")
#endif

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

#include "ipl.h"
#include "mx_sdhc.h"
#include <hw/inout.h>


/*--------------------------------------*/
/* MX SDMMC generci driver code     */
/*--------------------------------------*/

/*
 * SDMMC command table
 */
static const struct cmd_str
{
    int         cmd;
    unsigned    sdmmc_cmd;
} cmdtab[] =
{
    // MMC_GO_IDLE_STATE
    { 0,            (0 << 24) },
    // MMC_SEND_OP_COND (R3)
    { 1,            (1 << 24)|MMCCMDXTYPE_RSPTYPL48 },
    // MMC_ALL_SEND_CID (R2)
    { 2,            (2 << 24)|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL136 },
    // MMC_SET_RELATIVE_ADDR (R6)
    { 3,            (3 << 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // MMC_SWITCH (R1b)
    { 6,            (6 << 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48B },
    // MMC_SEL_DES_CARD (R1b)
    { 7,            (7 << 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48B },
    // MMC_IF_COND (R7)
    { 8,            (8 << 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // MMC_SEND_CSD (R2)
    { 9,            (9 << 24)|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL136 },
    // MMC_SEND_STATUS (R1)
    {13,            (13<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // MMC_SET_BLOCKLEN (R1)
    {16,            (16<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // MMC_READ_SINGLE_BLOCK (R1)
    {17,            (17<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48|MMCCMDXTYPE_DPSEL },
    // MMC_READ_MULTIPLE_BLOCK (R1)
    {18,            (18<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48|MMCCMDXTYPE_DPSEL},
    // MMC_APP_CMD (R1)
    {55,            (55<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // SD_SET_BUS_WIDTH (R1)
    {(55<<8)| 6,    (6 << 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48 },
    // SD_SEND_OP_COND (R3)
    {(55<<8)|41,    (41<< 24)|MMCCMDXTYPE_RSPTYPL48 },
    // end of command list
    {-1 },
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

/* sets the sdmmc clock frequency */
static void mx_set_frq(sdmmc_t *sdmmc, unsigned frq)
{
    sdmmc_hc_t  *hc = sdmmc->hc;
    unsigned    base = hc->sdmmc_pbase;
    unsigned div = 1, pre_div = 1, sctl;
    int ddr_mode = 0;

    frq = frq * 1000;
    if (sdmmc->timing == TIMING_DDR50) {
        ddr_mode = 1;
        pre_div = 2;
    }

    if (frq > hc->clock) {
        frq = hc->clock;
    }

    while ((hc->clock / (pre_div * 16) > frq) && (pre_div < 256)) {
        pre_div *= 2;
        }
    while ((hc->clock / (pre_div * div) > frq) && (div < 16)) {
        div++;
    }
    pre_div >>= (1 + ddr_mode);
    div -= 1;

    sctl = (0xE << MMCSYSCTL_DTOCVSHIFT) |
        (pre_div << MMCSYSCTL_SDCLKFSSHIFT) |
        (div << MMCSYSCTL_DVSSHIFT) | 0xF;

    if (sdmmc->verbose > SDMMC_VERBOSE_LVL_0) {
        ser_putstr("Set clock to ");
        ser_putdec(frq/1000);
        ser_putstr("KHz ref clock ");
        ser_putdec(hc->clock/1000);
        ser_putstr("KHz sysctl 0x");
        ser_puthex(sctl);
        ser_putstr(" pre_div ");
        ser_putdec(pre_div);
        ser_putstr(" div ");
        ser_putdec(div);
        ser_putstr("\n");
    }

    out32(base + MX_MMCSYSCTL, in32(base + MX_MMCSYSCTL) & ~(MMCSYSCTL_SDCLKEN)); /* Clear SDCLKEN to change the SD clock frequency */

    out32(base + MX_MMCSYSCTL, in32(base + MX_MMCSYSCTL) & ~(MMCSYSCTL_SDCLKFSMASK | MMCSYSCTL_DVSMASK)); /* Clear clock masks */

    out32(base + MX_MMCSYSCTL, in32(base + MX_MMCSYSCTL) | sctl); /* Set the clock */

    while (!(in32(base + MX_MMCPRSNTST) & MMCPRSNTST_SDSTB)) ; /* Wait till the SD clock is stable */

    // Wait for data and CMD signals to finish reset
    while ((in32(base + MX_MMCSYSCTL) & (MMCSYSCTL_RSTC | MMCSYSCTL_RSTD)) != 0) ;

    out32(base + MX_MMCSYSCTL, in32(base + MX_MMCSYSCTL) | MMCSYSCTL_SDCLKEN); /* Enable the SD clock */
}

/* sets the data bus width */
static void mx_set_bus_width(sdmmc_t *sdmmc, int width)
{
    unsigned    base = sdmmc->hc->sdmmc_pbase;
    unsigned    tmp = in32(base + MX_MMCPROTOCTL);

    if (sdmmc->verbose > SDMMC_VERBOSE_LVL_0) {
        ser_putstr("Bus width ");
        ser_putdec(width);
        ser_putstr("\n");
    }

    out32(base + MX_MMCPROTOCTL, tmp & MMCPROTOCTL_DTW1BIT); /* Clearing the Data transfer width field */
    tmp = in32(base + MX_MMCPROTOCTL); /* Read the register again */

    if (width == 8) {
        out32(base + MX_MMCPROTOCTL, tmp | MMCPROTOCTL_EMODLE | MMCPROTOCTL_DTW8BIT);
    } else if (width == 4) {
        out32(base + MX_MMCPROTOCTL, tmp | MMCPROTOCTL_EMODLE | MMCPROTOCTL_DTW4BIT);
    } else {
        out32(base + MX_MMCPROTOCTL, tmp | MMCPROTOCTL_EMODLE);
    }
}

static void mx_set_bus_mode(sdmmc_t *sdmmc, int mode)
{
    return;
}

static void mx_set_timing(sdmmc_t *sdmmc, int timing)
{
    unsigned base = sdmmc->hc->sdmmc_pbase;
    int mix_ctrl = in32(base + MX_MMCMIXCTRL);

    static const char   *name[9] = { "INVALID", "LS", "HS", "HS DDR50", "SDR12", "SDR25", "SDR50", "SDR104", "HS200" };

    if (sdmmc->verbose > SDMMC_VERBOSE_LVL_0) {
        ser_putstr("Timing ");
        ser_putstr(name[timing]);
        ser_putstr("\n");
    }

    if (timing == TIMING_DDR50)
        mix_ctrl |= MMCMIXCTRL_DDR_EN;
    else
        mix_ctrl &= ~MMCMIXCTRL_DDR_EN;
    out32(base + MX_MMCMIXCTRL, mix_ctrl);

    sdmmc->timing = timing;

    return;
}

/* initializes the SDMMC controller */
static int mx_init_ctrl(sdmmc_t *sdmmc)
{
    unsigned base = sdmmc->hc->sdmmc_pbase;

    sdmmc->icr  = 0x000001aa;             /* 2.7 V - 3.6 V */
    sdmmc->ocr  = 0x00300000;             /* 3.2 V - 3.4 V */

    /* Cleanup some registers first */
    mx_sdmmc_fini(sdmmc);

    out32(base + MX_MMCSYSCTL, in32(base + MX_MMCSYSCTL) | MMCSYSCTL_RSTA); /* Software Reset for ALL */

    while(in32(base + MX_MMCSYSCTL) & MMCSYSCTL_RSTA); /* Wait till the Reset bit gets cleared */

    mx_set_frq(sdmmc, 400);

    out32(base + MX_MMCPROTOCTL, 0x00000020); /* Reset value for Protocol Control Register */

    out32(base + MX_MMCINTRSIGEN, 0x017F8033); /* Enable all interrupt signals */

    out32(base + MX_MMCINTRSTEN, 0x017F8033); /* Enable all interrupt status bits */

    return SDMMC_OK;
}

/* clean up the SDMMC controller */
int mx_sdmmc_fini(sdmmc_t *sdmmc)
{
    unsigned base = sdmmc->hc->sdmmc_pbase;

    out32(base + MX_MMCINTRSIGEN, 0);
    out32(base + MX_MMCINTRSTEN, 0);
    out32(base + MX_MMCINTRST, in32(base + MX_MMCINTRST));
    out32(base + MX_MMCSYSCTL, 0);
    out32(base + MX_MMCCMDXTYPE, 0);

    return 0;
}

/* setup DMA for data read */
static void
mx6x_setup_dma_read(sdmmc_t *sdmmc)
{
    unsigned        base = sdmmc->hc->sdmmc_pbase;
    unsigned int sys_ctl;

    out32(base + MX_MMCBLATR, (sdmmc->cmd.bcnt << 16) | sdmmc->cmd.bsize);
    out32(base + MX_MMCDMASYSADD, (unsigned)sdmmc->cmd.dbuf);

    sys_ctl = in32(base + MX_MMCSYSCTL);
    sys_ctl &= ~MMCSYSCTL_DTOCVMASK;
    sys_ctl |= (14 << MMCSYSCTL_DTOCVSHIFT);
    out32(base + MX_MMCSYSCTL, sys_ctl);
}

/* issues a command on the SDMMC bus */
static int mx_send_cmd(sdmmc_t *sdmmc)
{
    struct cmd_str  *command;
    int             data_txf = 0;
    unsigned        base = sdmmc->hc->sdmmc_pbase;
    unsigned int    mix_ctrl = 0;
    sdmmc_cmd_t     *cmd  = &sdmmc->cmd;

    if (0 == (command = get_cmd (cmd->cmd))) {
        ser_putstr("Failed to get SDMMC command\n");
        return SDMMC_ERROR;
    }

    if (sdmmc->verbose > SDMMC_VERBOSE_LVL_2) {
        ser_putstr("mx_send_cmd(), CMD");
        ser_putdec(command->cmd);
        ser_putstr(" ARG: 0x");
        ser_puthex(cmd->arg);
        ser_putstr("\n");
    }

    /* check if need data transfer */
    data_txf = command->sdmmc_cmd & MMCCMDXTYPE_DPSEL;

    while ((in32(base + MX_MMCPRSNTST) & MMCPRSNTST_CDIHB) ||
           (in32(base + MX_MMCPRSNTST) & MMCPRSNTST_CIHB));

    while (in32(base + MX_MMCPRSNTST) & MMCPRSNTST_DLA);

    if (command->cmd == 0) {
        out32(base + MX_MMCSYSCTL, in32(base + MX_MMCSYSCTL) | MMCSYSCTL_INITA | (0x2 << MMCSYSCTL_DTOCVSHIFT)); /* Set Initialization Active */
        while (in32(base + MX_MMCSYSCTL) & MMCSYSCTL_INITA); /* Wait till the initialization bit gets cleared */
    } else if (command->cmd == MMC_READ_SINGLE_BLOCK) {
        mix_ctrl = (MMCMIXCTRL_DTDSEL | MMCMIXCTRL_DMAEN);
    } else if (command->cmd == MMC_READ_MULTIPLE_BLOCK) {
        mix_ctrl = (MMCMIXCTRL_MSBSEL | MMCMIXCTRL_BCEN | MMCMIXCTRL_AC12EN |
            MMCMIXCTRL_DTDSEL | MMCMIXCTRL_DMAEN);
    }

    if (data_txf) {
        mx6x_setup_dma_read(sdmmc);
    }

    /* clear SDMMC interrupt status register, write 1 to clear */
    out32(base + MX_MMCINTRST, 0xffffffff);

    /*
     * Rev 1.0 i.MX6x chips require the watermark register to be set prior to
     * every SD command being sent. If the watermark is not set only SD interface 3 works.
     */
    out32(base + MX_MMCWATML, (0x80 << MMCWATML_RDWMLSHIFT));

    /* setup the argument register and send command */
    out32(base + MX_MMCCMDARG, cmd->arg);

    mix_ctrl |= (in32(base + MX_MMCMIXCTRL) & ( MMCMIXCTRL_EXETUNE| MMCMIXCTRL_SMPCLKSEL |
          MMCMIXCTRL_AUTOTUNEEN | MMCMIXCTRL_FBCLKSEL | MMCMIXCTRL_DDR_EN));
    out32(base + MX_MMCMIXCTRL, mix_ctrl);

    out32(base + MX_MMCCMDXTYPE, command->sdmmc_cmd);

    /* wait for command finish */
    while (!(in32(base + MX_MMCINTRST) & (MMCINTR_CC | MMCINTR_ERRI)))
        ;

    /* check error status */
    unsigned temp = in32(base + MX_MMCINTRST);
    if (temp & MMCINTR_ERRI) {
        cmd->erintsts = temp;
        out32(base + MX_MMCSYSCTL, in32(base + MX_MMCSYSCTL) | MMCSYSCTL_RSTC);
        while (in32(base + MX_MMCSYSCTL) & MMCSYSCTL_RSTC)
            ;

        if (temp != 2) {
            /* We know some expected errors in IDLE state */
            if (sdmmc->card.state != MMC_IDLE) {
                ser_putstr("mx_send_cmd() failed at CMD");
                ser_putdec(command->cmd);
                ser_putstr(" MX_MMCINTRST: 0x");
                ser_puthex(temp);
                ser_putstr("\n");
            }
            return SDMMC_ERROR;
        }
    }

    /* get command response */
    if (cmd->rsp != 0) {
        cmd->rsp[0] = in32(base + MX_MMCCMDRSP0);

        if ((command->sdmmc_cmd & MMCCMDXTYPE_RSPTYPMASK) == MMCCMDXTYPE_RSPTYPL136) {
            cmd->rsp[1] = in32(base + MX_MMCCMDRSP1);
            cmd->rsp[2] = in32(base + MX_MMCCMDRSP2);
            cmd->rsp[3] = in32(base + MX_MMCCMDRSP3);

            /*
            * CRC is not included in the response register,
            * we have to left shift 8 bit to match the 128 CID/CSD structure
            */
            cmd->rsp[3] = (cmd->rsp[3] << 8) | (cmd->rsp[2] >> 24);
            cmd->rsp[2] = (cmd->rsp[2] << 8) | (cmd->rsp[1] >> 24);
            cmd->rsp[1] = (cmd->rsp[1] << 8) | (cmd->rsp[0] >> 24);
            cmd->rsp[0] = (cmd->rsp[0] << 8);
        }
    }

    if (data_txf) {
        while (1) {
            while (!(in32(base + MX_MMCINTRST) & (MMCINTR_TC | MMCINTR_ERRI)))
                ;

            if (in32(base + MX_MMCINTRST) & MMCINTR_ERRI) {
                out32(base + MX_MMCSYSCTL, in32(base + MX_MMCSYSCTL) | MMCSYSCTL_RSTC);
                while (in32(base + MX_MMCSYSCTL) & MMCSYSCTL_RSTC)
                    ;

                if (sdmmc->card.state != MMC_IDLE) {
                    ser_putstr("mx_send_cmd() failed at CMD");
                    ser_putdec(command->cmd);
                    ser_putstr(" MX_MMCINTRST: 0x");
                    ser_puthex(in32(base + MX_MMCINTRST));
                    ser_putstr("\n");
                }

                return SDMMC_ERROR;
            }
            if (in32(base + MX_MMCINTRST) & MMCINTR_TC) {
                break;
            }
        }
    }

    return SDMMC_OK;
}

static int mx_pio_read(sdmmc_t *sdmmc, void *buf, unsigned len)
{
    unsigned    base = sdmmc->hc->sdmmc_pbase;
    sdmmc_cmd_t     *cmd  = &sdmmc->cmd;
    unsigned int    mix_ctrl = 0;
    unsigned        *pbuf = (unsigned *)buf;

    /*wait for ready */
    while ((in32(base + MX_MMCPRSNTST) & MMCPRSNTST_CDIHB) ||
           (in32(base + MX_MMCPRSNTST) & MMCPRSNTST_CIHB));
    while (in32(base + MX_MMCPRSNTST) & MMCPRSNTST_DLA);

    /*setup PIO read */
    out32(base + MX_MMCBLATR, (1<<16) | len);

    /* clear SDMMC status */
    out32(base + MX_MMCINTRST, 0xffffffff);

    mix_ctrl = MMCMIXCTRL_DTDSEL |(in32(base + MX_MMCMIXCTRL) & (MMCMIXCTRL_EXETUNE| MMCMIXCTRL_SMPCLKSEL |
          MMCMIXCTRL_AUTOTUNEEN | MMCMIXCTRL_FBCLKSEL | MMCMIXCTRL_DDR_EN));
    out32(base + MX_MMCMIXCTRL, mix_ctrl);
    /*
     * Rev 1.0 i.MX6x chips require the watermark register to be set prior to
     * every SD command being sent. If the watermark is not set only SD interface 3 works.
     */
    out32(base + MX_MMCWATML, (0x80 << MMCWATML_RDWMLSHIFT));

    /* setup the argument register and send command */
    out32(base + MX_MMCCMDARG, cmd->arg);
    out32(base + MX_MMCCMDXTYPE, (cmd->cmd<< 24)|MMCCMDXTYPE_CICEN|MMCCMDXTYPE_CCCEN|MMCCMDXTYPE_RSPTYPL48|MMCCMDXTYPE_DPSEL);

    /* wait for command finish */
    while (!(in32(base + MX_MMCINTRST) & (MMCINTR_BRR | MMCINTR_ERRI)))
        ;

    /* check error status */
    if (in32(base + MX_MMCINTRST) & MMCINTR_ERRI) {
        cmd->erintsts = in32(base + MX_MMCINTRST);
        out32(base + MX_MMCSYSCTL, in32(base + MX_MMCSYSCTL) | MMCSYSCTL_RSTC);
        while (in32(base + MX_MMCSYSCTL) & MMCSYSCTL_RSTC)
            ;
        if(cmd->erintsts != 2) {
            if (sdmmc->card.state != MMC_IDLE) {
                ser_putstr("mx_pio_read() failed. MX_MMCINTRST: 0x");
                ser_puthex(cmd->erintsts);
                ser_putstr("\n");
            }
            return SDMMC_ERROR;
        }
    }

    /* get command responce */
    if (cmd->rsp != 0) {
        cmd->rsp[0] = in32(base + MX_MMCCMDRSP0);
    }

    /*now read from FIFO */
    for (; len > 0; len -= 4){
        *pbuf++ = in32(base + MX_MMCDATAPORT);
    }
#if 0
    if ( len < MMCHS_FIFO_SIZE ) {
        int    cnt;
        for ( cnt = 2000; cnt; cnt-- ) {
            if ( !( in32( base + OMAP4_MMCHS_PSTATE ) & PSTATE_RTA ) ) {
                break;
            }
            delay( 100 );
        }
    }
#endif
    return SDMMC_OK;
}

static int mx_dma_mxblks(sdmmc_t *sdmmc)
{
    /* No limit */
    return (~0);
}
/*--------------------------------------*/
/* MX Host Controller               */
/*--------------------------------------*/
static sdmmc_hc_t mx_hc;

void mx_sdmmc_init_hc(sdmmc_t *sdmmc, unsigned base, unsigned clock, int verbose)
{
    mx_hc.set_clk        = mx_set_frq;
    mx_hc.set_bus_width  = mx_set_bus_width;
    mx_hc.set_bus_mode   = mx_set_bus_mode;
    mx_hc.set_timing     = mx_set_timing;
    mx_hc.send_cmd       = mx_send_cmd;
    mx_hc.pio_read       = mx_pio_read;
    mx_hc.init_hc        = mx_init_ctrl;
    mx_hc.dinit_hc       = mx_sdmmc_fini;
    mx_hc.signal_voltage = 0;
    mx_hc.tuning         = 0;
    mx_hc.sdmmc_pbase    = base;
    mx_hc.clock          = clock;

    sdmmc->hc            = &mx_hc;
    sdmmc->verbose       = verbose;
    sdmmc->hc->max_blks  = mx_dma_mxblks;
    sdmmc->caps          = 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/arm/mx_sdmmc.c $ $Rev: 798438 $")
#endif

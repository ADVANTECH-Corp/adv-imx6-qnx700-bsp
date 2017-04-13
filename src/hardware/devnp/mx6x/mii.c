/*
 * $QNXLicenseC:
 * Copyright 2010, 2011 QNX Software Systems.
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


#include "mx6q.h"

static pthread_mutex_t  mii_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * On the i.MX6 SoloX there are two interfaces but the PHYs are
 * typically both hung off the MDIO on the first. To overcome this
 * we use mx6q->phy_reg here in the MII accesses, on fec0 it will
 * be set to the normal register space, however on fec1 it will
 * be set across to fec0. See mx6q_parse_options() for where it gets
 * picked up at "Find MDIO base address" and mx6q_attach() for where it
 * gets mapped at "map MDIO registers into virtual memory, if needed".
 *
 * One problem with this is that fec1 cannot guarantee that the MDC setup
 * on fec0 is always good, e.g. if fec0 isn't discovered or if fec0 gets
 * destroyed which triggers a reset of the fec0 block. We must always ensure
 * the MDC setup is correct on each read/write access.
 */
uint16_t
mx6q_mii_read (void *handle, uint8_t phy_add, uint8_t reg_add)
{
    mx6q_dev_t         *mx6q = (mx6q_dev_t *) handle;
    volatile uint32_t  *base = mx6q->phy_reg;
    int                 timeout = MPC_TIMEOUT;
    uint32_t            val;
    uint16_t            retval;

    _mutex_lock(&mii_mutex);

    /*
     * Internal MAC clock is 66MHz so MII_SPEED=0xD and HOLDTIME=0x2
     * See reference manual for field mapping
     */
    *(base + MX6Q_MII_SPEED) = 0x0000011A;

    *(base + MX6Q_IEVENT) = IEVENT_MII;
    val = ((1 << 30) | (0x2 << 28) | (phy_add << 23) | (reg_add << 18) | (2 << 16));
    *(base + MX6Q_MII_DATA) = val;

    while (timeout--) {
        if (*(base + MX6Q_IEVENT) & IEVENT_MII) {
                *(base + MX6Q_IEVENT) = IEVENT_MII;
                 break;
        }
        nanospin_ns (10000);
    }

    retval = ((timeout <= 0) ? 0 : (*(base + MX6Q_MII_DATA) & 0xffff));

    _mutex_unlock(&mii_mutex);

    return retval;
}

void
mx6q_mii_write (void *handle, uint8_t phy_add, uint8_t reg_add, uint16_t data)
{
    mx6q_dev_t         *mx6q = (mx6q_dev_t *) handle;
    volatile uint32_t  *base = mx6q->phy_reg;
    int                 timeout = MPC_TIMEOUT;
    uint32_t            phy_data;

    _mutex_lock(&mii_mutex);

    /*
     * Internal MAC clock is 66MHz so MII_SPEED=0xD and HOLDTIME=0x2
     * See reference manual for field mapping
     */
    *(base + MX6Q_MII_SPEED) = 0x0000011A;

    *(base + MX6Q_IEVENT) = IEVENT_MII;
    phy_data = ((1 << 30) | (0x1 << 28) | (phy_add << 23) | (reg_add << 18) | (2 << 16) | data);
    *(base + MX6Q_MII_DATA) = phy_data;
    while (timeout--) {
        if (*(base + MX6Q_IEVENT) & IEVENT_MII) {
            *(base + MX6Q_IEVENT) = IEVENT_MII;
            break;
        }
        nanospin_ns (10000);
    }

    _mutex_unlock(&mii_mutex);
}


//
// drvr lib callback when PHY link state changes
//
void
mx6q_mii_callback(void *handle, uchar_t phy, uchar_t newstate)
{
    mx6q_dev_t      *mx6q = handle;
    nic_config_t        *cfg  = &mx6q->cfg;
    char            *s;
    int         i;
    int         mode;
    struct ifnet        *ifp = &mx6q->ecom.ec_if;
    volatile uint32_t   *base = mx6q->reg;
    int         phy_idx = cfg->phy_addr;
    uint16_t        advert, lpadvert;

    switch(newstate) {
    case MDI_LINK_UP:
        if ((i = MDI_GetActiveMedia(mx6q->mdi, cfg->phy_addr, &mode)) != MDI_LINK_UP) {
            log(LOG_INFO, "%s(): MDI_GetActiveMedia() failed: %x", __FUNCTION__, i);
            mode = 0;  // force default case below - all MDI_ macros are non-zero
        }

        switch(mode) {
        case MDI_10bTFD:
            s = "10 BaseT Full Duplex";
            cfg->duplex = 1;
            cfg->media_rate = 10 * 1000L;
            *(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
            break;
        case MDI_10bT:
            s = "10 BaseT Half Duplex";
            cfg->duplex = 0;
            cfg->media_rate = 10 * 1000L;
            *(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
            break;
        case MDI_100bTFD:
            s = "100 BaseT Full Duplex";
            cfg->duplex = 1;
            cfg->media_rate = 100 * 1000L;
            *(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
            break;
        case MDI_100bT:
            s = "100 BaseT Half Duplex";
            cfg->duplex = 0;
            cfg->media_rate = 100 * 1000L;
            *(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
            break;
        case MDI_100bT4:
            s = "100 BaseT4";
            cfg->duplex = 0;
            cfg->media_rate = 100 * 1000L;
            *(base + MX6Q_ECNTRL) &= ~ECNTRL_ETH_SPEED;
            break;
        case MDI_1000bT:
            s = "1000 BaseT Half Duplex";
            cfg->duplex = 0;
            cfg->media_rate = 1000 * 1000L;
            *(base + MX6Q_ECNTRL) |= ECNTRL_ETH_SPEED;
            break;
        case MDI_1000bTFD:
            s = "1000 BaseT Full Duplex";
            cfg->duplex = 1;
            cfg->media_rate = 1000 * 1000L;
            *(base + MX6Q_ECNTRL) |= ECNTRL_ETH_SPEED;
            break;
        default:
            log(LOG_ERR, "%s(): unknown link mode 0x%X", __FUNCTION__, mode);
            s = "Unknown";
            cfg->duplex = 0;
            cfg->media_rate = 0L;
            return;
        }

	// immediately set new speed and duplex in nic config registers
	mx6q_speeduplex(mx6q);

	switch (mx6q->set_flow) {
	case MX6_FLOW_AUTO:
	    /* Flow control was autoneg'd, set what we got in the MAC */
	    advert = mx6q_mii_read(mx6q, phy_idx, MDI_ANAR);
	    lpadvert = mx6q_mii_read(mx6q, phy_idx, MDI_ANLPAR);
	    if (advert & MDI_FLOW) {
		if (lpadvert & MDI_FLOW) {
		    /* Enable Tx and Rx of Pause */
		    *(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0x82;
		    *(base + MX6Q_R_CNTRL) |= RCNTRL_FCE;
		    mx6q->flow_status = MX6_FLOW_BOTH;
		} else if ((advert & MDI_FLOW_ASYM) &&
			   (lpadvert & MDI_FLOW_ASYM)) {
		    /* Enable Rx of Pause */
		    *(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0;
		    *(base + MX6Q_R_CNTRL) |= RCNTRL_FCE;
		    mx6q->flow_status = MX6_FLOW_RX;
		} else {
		    /* Disable all pause */
		    *(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0;
		    *(base + MX6Q_R_CNTRL) &= ~RCNTRL_FCE;
		    mx6q->flow_status = MX6_FLOW_NONE;
		}
	    } else if ((advert & MDI_FLOW_ASYM) &&
		       (lpadvert & MDI_FLOW) &&
		       (lpadvert & MDI_FLOW_ASYM)) {
		/* Enable Tx of Pause */
		*(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0x82;
		*(base + MX6Q_R_CNTRL) &= ~RCNTRL_FCE;
		mx6q->flow_status = MX6_FLOW_TX;
	    } else {
		/* Disable all pause */
		*(base + MX6Q_R_SECTION_EMPTY_ADDR) = 0;
		*(base + MX6Q_R_CNTRL) &= ~RCNTRL_FCE;
		mx6q->flow_status = MX6_FLOW_NONE;
	    }
	    break;

	default:
	    /* Forced */
	    mx6q->flow_status = mx6q->set_flow;
	    break;
	}

	cfg->flags &= ~NIC_FLAG_LINK_DOWN;
	if (cfg->verbose) {
	    log(LOG_INFO, "%s(): link up lan %d idx %d (%s)",
		__FUNCTION__, cfg->lan, cfg->device_index, s);
	}
	if_link_state_change(ifp, LINK_STATE_UP);
	break;

    case MDI_LINK_DOWN:
        cfg->media_rate = cfg->duplex = -1;
        cfg->flags |= NIC_FLAG_LINK_DOWN;

        if (cfg->verbose) {
            log(LOG_INFO,
                "%s(): Link down lan %d idx %d, calling MDI_AutoNegotiate()",
                __FUNCTION__, cfg->lan, cfg->device_index);
        }
        MDI_AutoNegotiate(mx6q->mdi, cfg->phy_addr, NoWait);
        if_link_state_change(ifp, LINK_STATE_DOWN);
        break;

    default:
        log(LOG_ERR, "%s(): idx %d: Unknown link state 0x%X",
	    __FUNCTION__, cfg->device_index, newstate);
        break;
    }
}

/*****************************************************************************/
/* Check for link up/down condition of NXP PHY.                              */
/*****************************************************************************/

void mx6_nxp_phy_state (mx6q_dev_t *mx6q)
{
    uint16_t		phy_data, snr;
    nic_config_t	*cfg;
    struct ifnet	*ifp;

    cfg = &mx6q->cfg;
    ifp = &mx6q->ecom.ec_if;

    phy_data = mx6q_mii_read(mx6q, cfg->phy_addr, INT_STATUS_REG);
    if (mx6q->brmast) {
        if (phy_data & TRAINING_FAILED) {
            phy_data = mx6q_mii_read(mx6q, cfg->phy_addr, EXT_CONTROL_REG);
            mx6q_mii_write(mx6q, cfg->phy_addr, EXT_CONTROL_REG,
			   (phy_data | TRAINING_RESTART));
            return;
        }
    }
    if (phy_data & PHY_INT_ERR) {
	log(LOG_ERR, "%s(): PHY INT ERR 0x%x", __FUNCTION__, phy_data);
        if (phy_data & PHY_INIT_FAIL) {
            log(LOG_ERR, " - PHY Init failure");
	}
        if (phy_data & LINK_STATUS_FAIL) {
	    log(LOG_ERR, " - Link status failure");
	}
        if (phy_data & SYM_ERR) {
            log(LOG_ERR, " - Symbol error");
	}
        if (phy_data & CONTROL_ERR) {
            log(LOG_ERR, " - SMI control error");
	}
        if (phy_data & UV_ERR) {
            log(LOG_ERR, " - Under voltage error");
	}
        if (phy_data & TEMP_ERR) {
            log(LOG_ERR, " - Temperature error");
	}
    }

    phy_data = mx6q_mii_read(mx6q, cfg->phy_addr, COMM_STATUS_REG);
    snr = (phy_data & SNR_MASK) >> SNR_SHIFT;
    if (snr != mx6q->br_snr && (phy_data & LINK_UP)) {
        mx6q->br_snr = snr;
        if (!snr) {
	    log(LOG_ERR, "%s(): SNR Worse than class A", __FUNCTION__);
	} else {
	    log(LOG_ERR, "%s(): SNR class %c", __FUNCTION__, snr + 0x40);
	}
    }
    if (phy_data & COMM_STATUS_ERR) {
	log(LOG_ERR, "%s(): COMM STATUS ERR 0x%x", __FUNCTION__, phy_data);
        if (phy_data & SSD_ERR) {
            log(LOG_ERR, " - SSD error");
	}
        if (phy_data & ESD_ERR) {
            log(LOG_ERR, " - ESD error");
	}
        if (phy_data & RX_ERR) {
            log(LOG_ERR, " - Receive error");
	}
        if (phy_data & TX_ERR) {
            log(LOG_ERR, " - Transmit error");
	}
    }

    if ((cfg->flags & NIC_FLAG_LINK_DOWN) && (phy_data & LINK_UP)) {
        /* Link was down and is now up */
        if (cfg->verbose) {
            log(LOG_INFO, "%s(): Link up", __FUNCTION__);
        }
        cfg->flags &= ~NIC_FLAG_LINK_DOWN;
        if_link_state_change (ifp, LINK_STATE_UP);
        phy_data = mx6q_mii_read(mx6q, cfg->phy_addr, BASIC_CONTROL);
        cfg->media_rate = (phy_data & BC_SPEED_SEL) ? 100000L : 10000L;
        cfg->duplex = (phy_data & BC_DUPLEX) ? 1 : 0;

    } else if (((cfg->flags & NIC_FLAG_LINK_DOWN) == 0) &&
	       ((phy_data & LINK_UP) == 0)) {
        /* Link was up and is now down */
        if (cfg->verbose) {
            log(LOG_INFO, "%s(): Link down", __func__);
        }
        mx6q->br_snr = 0;
        cfg->flags |= NIC_FLAG_LINK_DOWN;
        if_link_state_change (ifp, LINK_STATE_DOWN);
    }

    phy_data = mx6q_mii_read(mx6q, cfg->phy_addr, EXTERN_STATUS_REG);
    if (phy_data) {
        log(LOG_ERR, "%s: External status 0x%x", __FUNCTION__, phy_data);
	if (phy_data & UV_VDDA_3V3) {
	    log(LOG_ERR, " - Undervoltage 3V3");
	}
	if (phy_data & UV_VDDD_1V8) {
	    log(LOG_ERR, " - Undervoltage VDDD 1V8");
	}
	if (phy_data & UV_VDDA_1V8) {
	    log(LOG_ERR, " - Undervoltage VDDA 1V8");
	}
	if (phy_data & UV_VDDIO) {
	    log(LOG_ERR, " - Undervoltage VDDIO");
	}
	if (phy_data & TEMP_HIGH) {
	    log(LOG_ERR, " - Temperature high");
	}
	if (phy_data & TEMP_WARN) {
	    log(LOG_ERR, " - Temperature warning");
	}
	if (phy_data & SHORT_DETECT) {
	    log(LOG_ERR, " - Short circuit detected");
	}
	if (phy_data & OPEN_DETECT) {
	    log(LOG_ERR, " - Open circuit detected");
	}
	if (phy_data & POL_DETECT) {
	    log(LOG_ERR, " - Polarity inversion detected");
	}
	if (phy_data & INTL_DETECT) {
	    log(LOG_ERR, " - Interleave detect");
	}
    }
}

void mx6_br_phy_state(mx6q_dev_t *mx6q)
{
    uint16_t    val;
    nic_config_t    *cfg = &mx6q->cfg;
    struct ifnet    *ifp = &mx6q->ecom.ec_if;

    if (mx6q->mdi->PhyData[cfg->phy_addr]->VendorOUI == NXP) {
        mx6_nxp_phy_state (mx6q);
        return;
    }

    /* Link state latches low so double read to clear */
    val = mx6q_mii_read(mx6q,  cfg->phy_addr, 1);
    val = mx6q_mii_read(mx6q,  cfg->phy_addr, 1);

    if ((cfg->flags & NIC_FLAG_LINK_DOWN) &&
        (val & 4)) {
        /* Link was down and is now up */
        if (cfg->verbose) {
            log(LOG_INFO, "%s(): link up", __FUNCTION__);
        }
        cfg->flags &= ~NIC_FLAG_LINK_DOWN;
        if_link_state_change(ifp, LINK_STATE_UP);
        // if link up again then arm callout to check sqi
        if (cfg->flags & ~NIC_FLAG_LINK_DOWN)
            callout_msec(&mx6q->sqi_callout, MX6Q_SQI_SAMPLING_INTERVAL * 1000, mx6q_BRCM_SQI_Monitor, mx6q);
    } else if (((cfg->flags & NIC_FLAG_LINK_DOWN) == 0) &&
           ((val & 4) == 0)) {
        /* Link was up and is now down */
        if (cfg->verbose) {
            log(LOG_INFO, "%s(): link down", __FUNCTION__);
        }
        cfg->flags |= NIC_FLAG_LINK_DOWN;
        if_link_state_change(ifp, LINK_STATE_DOWN);
    }
}

int mx6_is_br_phy (mx6q_dev_t *mx6q) {
    uint32_t	PhyAddr;
    int		is_br;

    PhyAddr = mx6q->cfg.phy_addr;

    switch (mx6q->mdi->PhyData[PhyAddr]->VendorOUI) {
    case BROADCOM3:
        switch(mx6q->mdi->PhyData[PhyAddr]->Model) {
	case BCM89811:
	    is_br = 1;
	    break;
	default:
	    is_br = 0;
	    break;
        }
        break;
    case BROADCOM2:
        switch (mx6q->mdi->PhyData[PhyAddr]->Model) {
	case BCM89810:
	    is_br = 1;
	    break;
	default:
	    is_br = 0;
	    break;
        }
        break;
    case NXP:
	switch (mx6q->mdi->PhyData[PhyAddr]->Model) {
	case TJA1100:
	case TJA1100_1:
	    is_br = 1;
	    break;
	default:
	    is_br = 0;
	    break;
        }
        break;
    default:
        is_br = 0;
        break;
    }
    return is_br;
}

//
// periodically called by stack to probe phy state
// and to clean out tx descriptor ring
//
void
mx6q_MDI_MonitorPhy (void *arg)
{
    mx6q_dev_t      *mx6q   = arg;
    nic_config_t        *cfg        = &mx6q->cfg;
    struct ifnet        *ifp        = &mx6q->ecom.ec_if;

    //
    // we will probe the PHY if:
    //   the user has forced it from the cmd line, or
    //   we have not rxd any packets since the last time we ran, or
    //   the link is considered down
    //
    if (mx6q->probe_phy ||
      !mx6q->rxd_pkts   ||
      cfg->media_rate <= 0 ||
      cfg->flags & NIC_FLAG_LINK_DOWN) {
        if (cfg->verbose > 4) {
            log(LOG_ERR, "%s(): calling MDI_MonitorPhy()\n",  __FUNCTION__);
        }
        if (!mx6_is_br_phy(mx6q)) {
            MDI_MonitorPhy(mx6q->mdi);
        } else {
            mx6_br_phy_state(mx6q);
        }

    } else {
        if (cfg->verbose > 4) {
            log(LOG_ERR, "%s(): NOT calling MDI_MonitorPhy()\n",  __FUNCTION__);
        }
    }
    mx6q->rxd_pkts = 0;  // reset for next time we are called

    //
    // Clean out the tx descriptor ring if it has not
    // been done by the start routine in the last 2 seconds
    //
    if (!mx6q->tx_reaped) {
        NW_SIGLOCK(&ifp->if_snd_ex, mx6q->iopkt);

        mx6q_transmit_complete(mx6q, 0);

        NW_SIGUNLOCK(&ifp->if_snd_ex, mx6q->iopkt);
    }
    mx6q->tx_reaped = 0;  // reset for next time we are called


    // restart timer to call us again in 2 seconds
    callout_msec(&mx6q->mii_callout, 2 * 1000, mx6q_MDI_MonitorPhy, mx6q);
}

static int mx6_get_phy_addr (mx6q_dev_t *mx6q)
{
    int     phy_idx;

    // Get PHY address
    for (phy_idx = 0; phy_idx < 32; phy_idx++) {
        if (MDI_FindPhy(mx6q->mdi, phy_idx) == MDI_SUCCESS) {
            if (mx6q->cfg.verbose) {
                log(LOG_ERR, "%s(): PHY found at address %d",
                    __FUNCTION__, phy_idx);
            }
            break;
        }
    }

    if (phy_idx == 32) {
        log(LOG_ERR,"Unable to detect PHY");
        return -1;
    } else {
        return phy_idx;
    }
}

/* this function disables Wirespeed mode (register 0x18, shadow 7, bit 4).
 */
int mx6_paves3_phy_init(mx6q_dev_t *mx6q)
{
    nic_config_t    *cfg    = &mx6q->cfg;
    int     phy_idx = cfg->phy_addr;
    unsigned short val;

    mx6q_mii_write(mx6q, phy_idx, 0x18, 0x7007);
    val = mx6q_mii_read(mx6q, phy_idx, 0x18);
    val &= 0xffef;
    val |= 0x8000;
    mx6q_mii_write(mx6q, phy_idx, 0x18, val);

    /* register 0x10, bit 14 - mdi autoneg disable */
    val = mx6q_mii_read(mx6q, phy_idx, 0x10);
    val |= 0x4000;
    mx6q_mii_write(mx6q, phy_idx, 0x10, val);
    
    return 0;
}

int mx6_paves3_twelve_inch_phy_init(mx6q_dev_t *mx6q)
{
    nic_config_t    *cfg    = &mx6q->cfg;
    int     phy_idx = cfg->phy_addr;
    unsigned short val;
    /* set page to 0 */
    mx6q_mii_write(mx6q, phy_idx, 0x16, 0);

    /* force MDI mode, disable polarity reversal */
    val = mx6q_mii_read(mx6q, phy_idx, 0x10);
    val &= 0xff9f;
    val |= 0x2;
    mx6q_mii_write(mx6q, phy_idx, 0x10, val);

    /* Software reset */
    val = mx6q_mii_read(mx6q, phy_idx, 0x0);
    val |= 0x8000;
    mx6q_mii_write(mx6q, phy_idx, 0x0, val);

    return 0;
}

void mx6_broadreach_bcm89810_phy_init(mx6q_dev_t *mx6q)
{
    int phy_idx = mx6q->cfg.phy_addr;

    /*
     * The following came from Broadcom as 89810A2_script_v2_2.vbs
     *
     * Broadcom refuse to document what exactly is going on.
     * They insist these register writes are correct despite the
     * way sometimes the same register is written back-to-back with
     * contradictory values and others are written with default values.
     * There is also much writing to undocumented registers and reserved
     * fields in documented registers.
     */
    mx6q_mii_write(mx6q, phy_idx, 0, 0x8000); //reset

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F93);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x107F);
    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F90);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0001);
    mx6q_mii_write(mx6q, phy_idx, 0x00, 0x3000);
    mx6q_mii_write(mx6q, phy_idx, 0x00, 0x0200);

    mx6q_mii_write(mx6q, phy_idx, 0x18, 0x0C00);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F90);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0000);
    mx6q_mii_write(mx6q, phy_idx, 0x00, 0x0100);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0001);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0027);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x000E);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x9B52);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x000F);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0xA04D);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F90);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0001);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F92);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x9225);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x000A);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0323);

    /* Shut off unused clocks */
    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0FFD);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x1C3F);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0FFE);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x1C3F);

    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F99);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x7180);
    mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F9A);
    mx6q_mii_write(mx6q, phy_idx, 0x15, 0x34C0);

    if (mx6q->mii) {
        /* MII-Lite config */
        mx6q_mii_write(mx6q, phy_idx, 0x18, 0xF167);
        mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F0E);
        mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0800);
        mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F9F);
        mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0000);
    } else {/* RGMII config */
        mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F0E);
        mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0000);
        mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0F9F);
        mx6q_mii_write(mx6q, phy_idx, 0x15, 0x0000);
        mx6q_mii_write(mx6q, phy_idx, 0x18, 0xF1E7);
    }

    /* Disable LDS, 100Mb/s, one pair */
    if (!mx6q->brmast) {
        /* Set phy to slave mode */
        log(LOG_INFO, "devnp-mx6x: Setting BroadrReach phy to slave");
        mx6q_mii_write(mx6q, phy_idx, 0, 0x0200);
    } else {
        /* Set phy to master mode */
        log(LOG_INFO, "devnp-mx6x: Setting BroadrReach phy to master");
        mx6q_mii_write(mx6q, phy_idx, 0, 0x0208);
    }
}

void mx6_broadreach_bcm89811_phy_init(mx6q_dev_t *mx6q)
{
    int phy_idx = mx6q->cfg.phy_addr;

    /*
     * The following came from Broadcom as 89811_script_v0_93.vbs
     *
     * Broadcom refuse to document what exactly is going on.
     * They insist these register writes are correct despite the
     * way sometimes the same register is written back-to-back with
     * contradictory values and others are written with default values.
     * There is also much writing to undocumented registers and reserved
     * fields in documented registers.
     */

	/* begin EMI optimization */
    mx6q_mii_write(mx6q, phy_idx, 0, 0x8000); //reset
    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0028);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x0C00);

    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0312);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x030B);

    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x030A);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x34C0);

    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0166);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x0020);

    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x012D);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x9B52);
    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x012E);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0xA04D);

    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0123);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x00C0);

    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0154);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x81C4);


    //IOUT = &H2&    ' 10=4.0mA
    //SLEW = &H2&    ' 10=3xdefault_slew
    //MII_PAD_SETTING = SLEW + 4*IOUT
    //v = &H0000& Or MII_PAD_SETTING * 2048
    //App.WrMii PORT, &H001E&, &H0811&   '
    //App.WrMii PORT, &H001F&, v   '
    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0811);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, (0x2+(0x2<<2))<<11);

    //v = &H0064&
    //App.WrMii PORT, &H001E&, &H01D3&   '
    //App.WrMii PORT, &H001F&, v   '
    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x01D3);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x0064);

    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x01C1);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0xA5F7);

    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0028);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x0400);

	/* End EMI optimization */

	/*begin LED setup portion*/
    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x001D);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x3411);

    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0820);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x0401);
	/*end of LED setup*/

    /*MII config*/
    if (mx6q->rmii) {
        mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x002F);
        mx6q_mii_write(mx6q, phy_idx, 0x1F, 0xF167);
        mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0045);
        mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x0700);
    } else if (mx6q->mii) {
        mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x002F);
        mx6q_mii_write(mx6q, phy_idx, 0x1F, 0xF167);
        mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0045);
        mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x0000);
    } else {  //ENET defaults to RGMII mode
        mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x0045);
        mx6q_mii_write(mx6q, phy_idx, 0x1F, 0x0000);
        mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x002F);
        mx6q_mii_write(mx6q, phy_idx, 0x1F, 0xF1E7);
    }

    /* Disable LDS, 100Mb/s, one pair */
    if (!mx6q->brmast) {
        /* Set phy to slave mode */
        log(LOG_INFO, "devnp-mx6x: Setting BroadrReach phy to slave");
        mx6q_mii_write(mx6q, phy_idx, 0, 0x0200);
    } else {
        /* Set phy to master mode */
        log(LOG_INFO, "devnp-mx6x: Setting BroadrReach phy to master");
        mx6q_mii_write(mx6q, phy_idx, 0, 0x0208);
    }
}

int mx6_broadreach_nxp_phy_init (mx6q_dev_t *mx6q)
{
    int         phy_addr;
    int         timeout;
    uint16_t    phy_data;

    phy_addr = mx6q->cfg.phy_addr;

    phy_data = mx6q_mii_read(mx6q, phy_addr, INT_STATUS_REG);
    if (phy_data & PHY_INIT_FAIL) {
	phy_data = BC_RESET;
	mx6q_mii_write(mx6q, phy_addr, BASIC_CONTROL, phy_data);
	timeout = 100;
	while (timeout) {
	    if (!(phy_data = mx6q_mii_read(mx6q, phy_addr, BASIC_CONTROL) &
		  BC_RESET)) {
		break;
	    }
            timeout--;
            nanospin_ns(1000);
        }
        if (!timeout) {
	    log(LOG_ERR, "%s(): PHY did not come out of reset", __FUNCTION__);
            return -1;
        }
        phy_data = mx6q_mii_read(mx6q, phy_addr, INT_STATUS_REG);
    }
    if (phy_data & PHY_INIT_FAIL) {
        log(LOG_ERR, "%s(): PHY initialization failed", __FUNCTION__);
        return -1;
    }

    phy_data = mx6q_mii_read(mx6q, phy_addr, EXT_CONTROL_REG);
    phy_data |= CONFIG_ENABLE;
    mx6q_mii_write(mx6q, phy_addr, EXT_CONTROL_REG, phy_data);
    nic_delay(2);

    phy_data = mx6q_mii_read(mx6q, phy_addr, CONFIG_REG_1);
    phy_data &= ~(MASTER_SLAVE | AUTO_OP | (MII_MODE << MII_MODE_SHIFT));
    if (mx6q->brmast) {
        phy_data |= MASTER_SLAVE;
	if (mx6q->cfg.verbose) {
	    log(LOG_INFO, "%s(): PHY set to master mode", __FUNCTION__);
	}
    } else {
	if (mx6q->cfg.verbose) {
	    log(LOG_INFO, "%s(): PHY set to slave mode", __FUNCTION__);
	}
    }
    mx6q_mii_write(mx6q, phy_addr, CONFIG_REG_1, phy_data);

    phy_data = mx6q_mii_read(mx6q, phy_addr, CONFIG_REG_2);
    phy_data |= JUMBO_ENABLE;
    mx6q_mii_write(mx6q, phy_addr, CONFIG_REG_2, phy_data);

    phy_data = ((NORMAL_MODE << POWER_MODE_SHIFT) | LINK_CONTROL |
		CONFIG_INH | WAKE_REQUEST);
    mx6q_mii_write(mx6q, phy_addr, EXT_CONTROL_REG, phy_data);
    nic_delay(5);
    return 0;
}

void mx6_broadreach_phy_init (mx6q_dev_t *mx6q)
{
    nic_config_t	*cfg;

    cfg = &mx6q->cfg;

    switch (mx6q->mdi->PhyData[cfg->phy_addr]->VendorOUI) {
    case BROADCOM2:
	switch(mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
	case BCM89810:
	    if (cfg->verbose > 3) {
		log(LOG_INFO, "Detected BCM89810");
	    }
	    mx6_broadreach_bcm89810_phy_init(mx6q);
	    break;
	default:
	    break;
	}
	break;
    case BROADCOM3:
	switch(mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
	case BCM89811:
	    if (cfg->verbose > 3) {
		log(LOG_INFO, "Detected BCM89811");
	    }
	    mx6_broadreach_bcm89811_phy_init(mx6q);
	    break;
	default:
	    break;
	}
	break;
    case NXP:
	switch(mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
	case TJA1100:
	case TJA1100_1:
            if (cfg->verbose > 3)
                log (LOG_INFO, "Detected NXP TJA1100 PHY");
            mx6_broadreach_nxp_phy_init (mx6q);
            break;
	default:
	    break;
	}
	break;
    default:
	break;
    }
}

static void mmd_write_reg(mx6q_dev_t *mx6q, int device, int reg, int val)
{
        nic_config_t *cfg = &mx6q->cfg;
        int     phy_idx = cfg->phy_addr;

        mx6q_mii_write(mx6q, phy_idx,  0x0d, device);
        mx6q_mii_write(mx6q, phy_idx, 0x0e, reg);
        mx6q_mii_write(mx6q, phy_idx, 0x0d, (1 << 14) | device);
        mx6q_mii_write(mx6q, phy_idx, 0x0e, val);
}

int mx6_ksz9031_phy_init(mx6q_dev_t *mx6q)
{
        nic_config_t    *cfg    = &mx6q->cfg;
        int             phy_idx = cfg->phy_addr;

        // master mode (when possible), 1000 Base-T capable
        mx6q_mii_write(mx6q, phy_idx, 0x9, 0x0f00);


        /*
         * min rx data delay, max rx/tx clock delay,
         * min rx/tx control delay
         */
        mmd_write_reg(mx6q, 2, 4, 0);
        mmd_write_reg(mx6q, 2, 5, 0);
        mmd_write_reg(mx6q, 2, 8, 0x003ff);

        return 0;
}

int mx6_sabrelite_phy_init(mx6q_dev_t *mx6q)
{
    nic_config_t    *cfg    = &mx6q->cfg;
    int     phy_idx = cfg->phy_addr;

    // 1000 Base-T full duplex capable, single port device
    mx6q_mii_write(mx6q, phy_idx, 0x9, 0x0600);

    // min rx data delay
    mx6q_mii_write(mx6q, phy_idx, 0xb, 0x8105);
    mx6q_mii_write(mx6q, phy_idx, 0xc, 0x0000);

    // max rx/tx clock delay, min rx/tx control delay
    mx6q_mii_write(mx6q, phy_idx, 0xb, 0x8104);
    mx6q_mii_write(mx6q, phy_idx, 0xc, 0xf0f0);
    mx6q_mii_write(mx6q, phy_idx, 0xb, 0x104);

    return 0;
}

int mx6_sabreauto_rework(mx6q_dev_t *mx6q)
{
    nic_config_t        *cfg        = &mx6q->cfg;
    int                 phy_idx     = cfg->phy_addr;
    unsigned short val;

    /* To enable AR8031 ouput a 125MHz clk from CLK_25M */
    mx6q_mii_write(mx6q, phy_idx, 0xd, 0x7);
    mx6q_mii_write(mx6q, phy_idx, 0xe, 0x8016);
    mx6q_mii_write(mx6q, phy_idx, 0xd, 0x4007);
    val = mx6q_mii_read(mx6q, phy_idx, 0xe);

    val &= 0xffe3;
    val |= 0x18;
    mx6q_mii_write(mx6q, phy_idx, 0xe, val);

    /* introduce tx clock delay */
    mx6q_mii_write(mx6q, phy_idx, 0x1d, 0x5);
    val = mx6q_mii_read(mx6q, phy_idx, 0x1e);
    val |= 0x0100;
    mx6q_mii_write(mx6q, phy_idx, 0x1e, val);

    /*
     * Disable SmartEEE
     * The Tx delay can mean late pause and bad timestamps.
     */
    mx6q_mii_write(mx6q, phy_idx, 0xd, 0x3);
    mx6q_mii_write(mx6q, phy_idx, 0xe, 0x805d);
    mx6q_mii_write(mx6q, phy_idx, 0xd, 0x4003);
    val = mx6q_mii_read(mx6q, phy_idx, 0xe);
    val &= ~(1 << 8);
    mx6q_mii_write(mx6q, phy_idx, 0xe, val);

    /* As above for EEE (802.3az) */
    mx6q_mii_write(mx6q, phy_idx, 0xd, 0x7);
    mx6q_mii_write(mx6q, phy_idx, 0xe, 0x3c);
    mx6q_mii_write(mx6q, phy_idx, 0xd, 0x4007);
    mx6q_mii_write(mx6q, phy_idx, 0xd,0);

    return 0;
}

int
mx6q_init_phy(mx6q_dev_t *mx6q)
{
    int			status;
    nic_config_t	*cfg;
    struct ifnet	*ifp;

    cfg = &mx6q->cfg;
    ifp = &mx6q->ecom.ec_if;

    // register callbacks with MII managment library
    callout_init(&mx6q->mii_callout);
    status = MDI_Register_Extended (mx6q, mx6q_mii_write, mx6q_mii_read,
				mx6q_mii_callback,
				(mdi_t **)&mx6q->mdi, NULL, 0, 0);
    if (status != MDI_SUCCESS) {
        log(LOG_ERR, "%s(): MDI_Register_Extended() failed: %d",
            __FUNCTION__, status);
        return -1;
    }

    // Register callbacks with SQI calculation
    callout_init(&mx6q->sqi_callout);

    // get PHY address
    if (cfg->phy_addr == -1) {
        cfg->phy_addr = mx6_get_phy_addr(mx6q);
	if (cfg->phy_addr == -1) {
	    // Failure already logged in mx6_get_phy_addr()
	    return -1;
	}
    }

    status = MDI_InitPhy(mx6q->mdi, cfg->phy_addr);
    if (status != MDI_SUCCESS) {
        log(LOG_ERR, "%s(): Failed to init the PHY: %d",
            __FUNCTION__, status);
        return -1;
    }

    if (mx6_is_br_phy(mx6q)) {
        mx6_broadreach_phy_init(mx6q);
    } else {
        status = MDI_ResetPhy(mx6q->mdi, cfg->phy_addr, WaitBusy);
        if (status != MDI_SUCCESS) {
            log(LOG_ERR, "%s(): Failed to reset the PHY: %d",
                __FUNCTION__, status);
            return -1;
        }

        switch (mx6q->mdi->PhyData[cfg->phy_addr]->VendorOUI) {
        case ATHEROS:
            switch (mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
            case AR8031:
                if (cfg->verbose > 3)
                    log(LOG_INFO, "Detected Atheros AR8031 PHY");
                mx6_sabreauto_rework(mx6q);
                break;
            default:
                break;
            }
            break;

        case KENDIN:
            switch (mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
            case KSZ9021:
                if (cfg->verbose > 3)
                    log(LOG_INFO, "Detected Micrel KSZ9021 PHY");
                mx6_sabrelite_phy_init(mx6q);
                break;
            case KSZ9031:
                if (cfg->verbose > 3)
                    log (LOG_INFO, "Detected Micrel KSZ9031 PHY");
                mx6_ksz9031_phy_init (mx6q);
                break;
            case KSZ8081:
                if (cfg->verbose > 3)
                    log (LOG_INFO, "Detected Micrel KSZ8081 PHY");
            default:
                break;
            }
            break;

        case BROADCOM2:
            switch (mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
            case BCM54616:
                if (cfg->verbose > 3) {
                    log(LOG_INFO, "Detected Broadcom BCM54616 PHY");
                }
                mx6_paves3_phy_init(mx6q);
                break;
            case BCM89810:
                if (cfg->verbose > 3) {
                    log(LOG_INFO, "Detected Broadcom BCM89810 PHY");
                }
                break;
            default:
                break;
            }
            break;

        case MARVELL:
            switch (mx6q->mdi->PhyData[cfg->phy_addr]->Model) {
            case ALASKA:
                if (cfg->verbose > 3)
                    log(LOG_INFO, "Detected Marvell PHY");
                mx6_paves3_twelve_inch_phy_init(mx6q);
                break;
            default:
                break;
            }
            break;

        default:
            break;
        }
	MDI_PowerdownPhy(mx6q->mdi, cfg->phy_addr);
    }

    cfg->flags |= NIC_FLAG_LINK_DOWN;
    if_link_state_change(ifp, LINK_STATE_DOWN);

    return 0;
}

void
mx6q_fini_phy(mx6q_dev_t *mx6q)
{
    MDI_DeRegister(&mx6q->mdi);
    mx6q->mdi = NULL;
}


//
// periodically called by stack to probe sqi
//
void
mx6q_BRCM_SQI_Monitor (void *arg)
{
    mx6q_dev_t      *mx6q   = arg;
    nic_config_t        *cfg    = &mx6q->cfg;

    if (cfg->flags & NIC_FLAG_LINK_DOWN) {
        if (cfg->verbose) {
            log(LOG_INFO, "%s(): link down, clear sqi sample buffer:  \n", __FUNCTION__);
        }
        mx6_clear_sample();
    }
    else {
        mx6_mii_sqi(mx6q);
        // restart timer to call us again in MX6Q_SQI_SAMPLING_INTERVAL seconds
        callout_msec(&mx6q->sqi_callout, MX6Q_SQI_SAMPLING_INTERVAL * 1000, mx6q_BRCM_SQI_Monitor, mx6q);
    }
}

/*read the SQI register*/
static void mx6_mii_sqi_bcm89810(mx6q_dev_t *mx6q)
{
    /*
    follow the 2 steps to read SQI register.
    1. Set up register access:
        Write 0C00h to register 18h
        Write 0002h to register 17h
    2. Read register 15h

    Refer to the datasheet, April 20, 2012 - 89810-DS03-R - Table 10, page 50
    */
    unsigned short val = 0;
    nic_config_t    *cfg = &mx6q->cfg;
    int     phy_idx = cfg->phy_addr;

	mx6q_mii_write(mx6q, phy_idx, 0x18, 0x0C00);
	mx6q_mii_write(mx6q, phy_idx, 0x17, 0x0002);
	val = mx6q_mii_read(mx6q, phy_idx, 0x15);
	if (cfg->verbose > 5) {
		log(LOG_INFO, "%s(): SQI read val:  %d\n", __FUNCTION__, val);
	}

	if (val > 0)
		mx6q->sqi = mx6_calculate_sqi(val);

	if (cfg->verbose > 5) {
		log(LOG_INFO, "%s(): sqi = :  %d\n",  __FUNCTION__, mx6q->sqi);
	}
}

/*read the SQI register*/
static void mx6_mii_sqi_bcm89811(mx6q_dev_t *mx6q)
{
    /*
    follow the 2 steps to read SQI register.
    1. Set up register access:
        read-modify-write to set bit 11 = 1 in RDB register 0x028
    2. Read RDB register 0x108

    Refer to the datasheet, April 20, 2012 - 89810-DS03-R - Table 10, page 50
    */
    unsigned short val = 0;
    nic_config_t    *cfg = &mx6q->cfg;
    int     phy_idx = cfg->phy_addr;

    //read RDB register 0x28
    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x28);
    val = mx6q_mii_read(mx6q, phy_idx, 0x1F);
    //write RDB register with bit 11=1
    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x28);
    mx6q_mii_write(mx6q, phy_idx, 0x1F, val|0x800);

    //read RDB register 0x108
    mx6q_mii_write(mx6q, phy_idx, 0x1E, 0x108);
    val = mx6q_mii_read(mx6q, phy_idx, 0x1F);

    if (cfg->verbose > 5) {
        log(LOG_INFO, "%s(): SQI read val:  %d\n", __FUNCTION__, val);
    }

    if (val > 0)
        mx6q->sqi = mx6_calculate_sqi(val);

    if (cfg->verbose > 5) {
        log(LOG_INFO, "%s(): sqi = :  %d\n",  __FUNCTION__, mx6q->sqi);
    }
}

/*read the SQI register*/
void mx6_mii_sqi(mx6q_dev_t *mx6q)
{
	nic_config_t       *cfg = &mx6q->cfg;

	if ((mx6q->mdi->PhyData[cfg->phy_addr]->VendorOUI == BROADCOM2) &&
		(mx6q->mdi->PhyData[cfg->phy_addr]->Model == BCM89810)) {
			mx6_mii_sqi_bcm89810(mx6q);
	}
	else if ((mx6q->mdi->PhyData[cfg->phy_addr]->VendorOUI == BROADCOM3) &&
	    (mx6q->mdi->PhyData[cfg->phy_addr]->Model == BCM89811)){
			mx6_mii_sqi_bcm89811(mx6q);
	}
	else
		log(LOG_ERR, "%s(): unknown PHY, SQI not availiable.  \n", __FUNCTION__);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/mx6x/mii.c $ $Rev: 816496 $")
#endif

/*
 * $QNXLicenseC: 
 * Copyright 2010, QNX Software Systems.  
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

#include <mx6q.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <device_qnx.h>


//
// this is a callback, made by the bsd media code.  We passed
// a pointer to this function during the ifmedia_init() call
// in bsd_mii_initmedia()
//
void
bsd_mii_mediastatus (struct ifnet *ifp, struct ifmediareq *ifmr)
{
    mx6q_dev_t	*mx6q;

    mx6q = ifp->if_softc;

    mx6q->bsd_mii.mii_media_active = IFM_ETHER;
    mx6q->bsd_mii.mii_media_status = IFM_AVALID;

    if ((mx6q->cfg.flags & NIC_FLAG_LINK_DOWN) != 0) {
	mx6q->bsd_mii.mii_media_active |= IFM_NONE;
    } else {
	mx6q->bsd_mii.mii_media_status |= IFM_ACTIVE;

	switch (mx6q->cfg.media_rate) {

	case 10000L:
	    mx6q->bsd_mii.mii_media_active |= IFM_10_T;
	    break;

	case 100000L:
	    mx6q->bsd_mii.mii_media_active |= IFM_100_TX;
	    break;

	case 1000000L:
	    mx6q->bsd_mii.mii_media_active |= IFM_1000_T;
	    break;

	default:
	log(LOG_ERR, "mx6q: Unknown media, forcing none");
	/* Fallthrough */
	case 0:
	    mx6q->bsd_mii.mii_media_active |= IFM_NONE;
	    break;
	}

	if (mx6q->cfg.duplex) {
	    mx6q->bsd_mii.mii_media_active |= IFM_FDX;
	}

	switch (mx6q->flow_status) {
	case MX6_FLOW_RX:
	    mx6q->bsd_mii.mii_media_active |= IFM_ETH_RXPAUSE;
	    break;

	case MX6_FLOW_TX:
	    mx6q->bsd_mii.mii_media_active |= IFM_ETH_TXPAUSE;
	    break;

	case MX6_FLOW_BOTH:
	    mx6q->bsd_mii.mii_media_active |= IFM_FLOW;
	    break;

	default:
	    /* No flow control */
	    break;
	}
    }

    /* Return the data */
    ifmr->ifm_status = mx6q->bsd_mii.mii_media_status;
    ifmr->ifm_active = mx6q->bsd_mii.mii_media_active;
}

//
// this is a callback, made by the bsd media code.  We passed
// a pointer to this function during the ifmedia_init() call
// in bsd_mii_initmedia().  This function is called when
// someone makes an ioctl into us, we call into the generic
// ifmedia source, and it make this callback to actually 
// force the speed and duplex, just as if the user had
// set the cmd line options
//
int
bsd_mii_mediachange (struct ifnet *ifp)
{
    mx6q_dev_t		*mx6q;
    struct ifmedia	*ifm;
    nic_config_t	*cfg;
    int			an_media;

    mx6q = ifp->if_softc;
    cfg = &mx6q->cfg;
    ifm = &mx6q->bsd_mii.mii_media;

    if (!(ifp->if_flags & IFF_UP)) {
	slogf( _SLOGC_NETWORK, _SLOG_WARNING,
	       "%s(): mx6q interface isn't up, ioctl ignored", __FUNCTION__);
	return 0;
    }

    switch (ifm->ifm_media & IFM_TMASK) {
    case IFM_NONE:
	mx6q->set_speed = 0;
	mx6q->set_duplex = 0;
	mx6q->set_flow = MX6_FLOW_NONE;

	/* Special case, shut down the PHY and bail out */
	callout_stop(&mx6q->mii_callout);
	MDI_DisableMonitor(mx6q->mdi);
	MDI_PowerdownPhy(mx6q->mdi, cfg->phy_addr);
	cfg->flags |= NIC_FLAG_LINK_DOWN;
	if_link_state_change(ifp, LINK_STATE_DOWN);
	return 0;

    case IFM_AUTO:
	mx6q->set_speed = -1;
	mx6q->set_duplex = -1;
	mx6q->set_flow = MX6_FLOW_AUTO;

	MDI_GetMediaCapable(mx6q->mdi, cfg->phy_addr, &an_media);
	if ((mx6q->mdi->PhyData[cfg->phy_addr]->VendorOUI == KENDIN) &&
	    ((mx6q->mdi->PhyData[cfg->phy_addr]->Model == KSZ9021) ||
	     (mx6q->mdi->PhyData[cfg->phy_addr]->Model == KSZ9031))) {
	    /* Fails to autoneg with ASYM */
	    an_media |= MDI_FLOW;
	} else {
	    an_media |= MDI_FLOW | MDI_FLOW_ASYM;
	}
	break;

    case IFM_10_T:
	mx6q->set_speed = 10000L;
	mx6q->set_flow = MX6_FLOW_NONE;

	if ((ifm->ifm_media & IFM_FDX) == 0) {
	    mx6q->set_duplex = 0;
	    an_media = MDI_10bT;
	} else {
	    mx6q->set_duplex = 1;
	    an_media = MDI_10bTFD;
	}
	break;

    case IFM_100_TX:
	mx6q->set_speed = 100000L;
	mx6q->set_flow = MX6_FLOW_NONE;

	if ((ifm->ifm_media & IFM_FDX) == 0) {
	    mx6q->set_duplex = 0;
	    an_media = MDI_100bT;
	} else {
	    mx6q->set_duplex = 1;
	    an_media = MDI_100bTFD;
	}
	break;

    case IFM_1000_T:
	mx6q->set_speed = 1000000L;
	mx6q->set_duplex = 1;
	mx6q->set_flow = MX6_FLOW_NONE;
	an_media = MDI_1000bTFD;
	break;

    default:			// should never happen
	slogf( _SLOGC_NETWORK, _SLOG_WARNING,
	       "%s(): mx6q interface - unknown media: 0x%X", 
	       __FUNCTION__, ifm->ifm_media);
	return 0;
	break;
    }

    if (ifm->ifm_media & IFM_FLOW) {
	mx6q->set_flow = MX6_FLOW_BOTH;
	an_media |= MDI_FLOW;
    } else if (ifm->ifm_media & IFM_ETH_TXPAUSE) {
	mx6q->set_flow = MX6_FLOW_TX;
	an_media |= MDI_FLOW_ASYM;
    } else if (ifm->ifm_media & IFM_ETH_RXPAUSE) {
	mx6q->set_flow = MX6_FLOW_RX;
	an_media |= MDI_FLOW_ASYM;
    }

    /* Force down as renegotiating and poll for it to come up */
    callout_stop(&mx6q->mii_callout);
    cfg->flags |= NIC_FLAG_LINK_DOWN;
    if_link_state_change(ifp, LINK_STATE_DOWN);

    MDI_PowerupPhy(mx6q->mdi, cfg->phy_addr);
    MDI_EnableMonitor(mx6q->mdi, 0);
    /*
     * Micrel KSZ9021 takes a while to settle after powering up and
     * autoneg fails if done immediately. Start the polling now as
     * the MDIO reads the polling does give it enough time to settle
     * before configuring the autoneg.
     */
    mx6q_MDI_MonitorPhy(mx6q);
    MDI_SetAdvert(mx6q->mdi, cfg->phy_addr, an_media);
    MDI_AutoNegotiate(mx6q->mdi, cfg->phy_addr, NoWait);


    return 0;
}


//
// called from mx6q_create_instance() in detect.c to hook up
// to the bsd media structure.
//
// NOTE: Must call bsd_mii_finimedia() to free resources.
//
void
bsd_mii_initmedia(mx6q_dev_t *mx6q)
{
    nic_config_t	*cfg;
    struct ifmedia	*ifm;

    cfg = &mx6q->cfg;
    ifm = &mx6q->bsd_mii.mii_media;

    mx6q->bsd_mii.mii_ifp = &mx6q->ecom.ec_if;

    ifmedia_init(ifm, IFM_IMASK, bsd_mii_mediachange, bsd_mii_mediastatus);

    ifmedia_add(ifm, IFM_ETHER|IFM_NONE, 0, NULL);
    ifmedia_add(ifm, IFM_ETHER|IFM_AUTO, 0, NULL);

    ifmedia_add(ifm, IFM_ETHER|IFM_10_T, 0, NULL);
    ifmedia_add(ifm, IFM_ETHER|IFM_10_T|IFM_FDX, 0, NULL);
    ifmedia_add(ifm, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_FLOW, 0, NULL);
    /* Kendin PHY bug, fails to autoneg with MDI_FLOW_ASYM */
    if (!((mx6q->mdi->PhyData[cfg->phy_addr]->VendorOUI == KENDIN) &&
	  ((mx6q->mdi->PhyData[cfg->phy_addr]->Model == KSZ9021) ||
	   (mx6q->mdi->PhyData[cfg->phy_addr]->Model == KSZ9031)))) {

	ifmedia_add(ifm, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_ETH_TXPAUSE,
		    0, NULL);
	ifmedia_add(ifm, IFM_ETHER|IFM_10_T|IFM_FDX|IFM_ETH_RXPAUSE,
		    0, NULL);
    }

    ifmedia_add(ifm, IFM_ETHER|IFM_100_TX, 0, NULL);
    ifmedia_add(ifm, IFM_ETHER|IFM_100_TX|IFM_FDX, 0, NULL);
    ifmedia_add(ifm, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_FLOW, 0, NULL);
    /* Kendin PHY bug, fails to autoneg with MDI_FLOW_ASYM */
    if (!((mx6q->mdi->PhyData[cfg->phy_addr]->VendorOUI == KENDIN) &&
	  ((mx6q->mdi->PhyData[cfg->phy_addr]->Model == KSZ9021) ||
	   (mx6q->mdi->PhyData[cfg->phy_addr]->Model == KSZ9031)))) {

	ifmedia_add(ifm, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_ETH_TXPAUSE,
		    0, NULL);
	ifmedia_add(ifm, IFM_ETHER|IFM_100_TX|IFM_FDX|IFM_ETH_RXPAUSE,
		    0, NULL);
    }

    if (!(mx6q->rmii || mx6q->mii)) {
	ifmedia_add(ifm, IFM_ETHER|IFM_1000_T|IFM_FDX, 0, NULL);
	ifmedia_add(ifm, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_FLOW, 0, NULL);
	/* Kendin PHY bug, fails to autoneg with MDI_FLOW_ASYM */
	if (!((mx6q->mdi->PhyData[cfg->phy_addr]->VendorOUI == KENDIN) &&
	      ((mx6q->mdi->PhyData[cfg->phy_addr]->Model == KSZ9021) ||
	       (mx6q->mdi->PhyData[cfg->phy_addr]->Model == KSZ9031)))) {

	    ifmedia_add(ifm, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_ETH_TXPAUSE,
			0, NULL);
	    ifmedia_add(ifm, IFM_ETHER|IFM_1000_T|IFM_FDX|IFM_ETH_RXPAUSE,
			0, NULL);
	}
    }

    /*
     * nic_parse_options() sets speed / duplex in cfg but those are for
     * reporting state. Copy them across to the right place.
     */
    mx6q->set_speed = cfg->media_rate;
    mx6q->set_duplex = cfg->duplex;
    cfg->media_rate = 0;
    cfg->duplex = 0;

    switch (mx6q->set_speed) {
    case -1:
	ifm->ifm_media = IFM_ETHER | IFM_AUTO;
	break;

    case 10000L:
	ifm->ifm_media = IFM_ETHER | IFM_10_T;
	break;

    case 100000L:
	ifm->ifm_media = IFM_ETHER | IFM_100_TX;
	break;

    case 1000000L:
	ifm->ifm_media = IFM_ETHER | IFM_1000_T;
	break;

    default:
	slogf( _SLOGC_NETWORK, _SLOG_WARNING,
		  "%s(): mx6q Unknown initial media speed %d, forcing none",
		  __FUNCTION__, mx6q->set_speed);
	/* Fallthrough */

    case 0:
	ifm->ifm_media = IFM_ETHER | IFM_NONE;
	break;
    }

    if ((ifm->ifm_media & (IFM_NONE | IFM_AUTO)) == 0) {
	if (mx6q->set_duplex == 1) {
	    ifm->ifm_media |= IFM_FDX;

	    switch(mx6q->set_flow) {
	    case MX6_FLOW_BOTH:
		ifm->ifm_media |= IFM_FLOW;
		break;
	    case MX6_FLOW_TX:
		ifm->ifm_media |= IFM_ETH_TXPAUSE;
		break;
	    case MX6_FLOW_RX:
		ifm->ifm_media |= IFM_ETH_RXPAUSE;
		break;
	    default:
		/* No flow control */
		break;
	    }
	}
    }

    ifmedia_set(ifm, ifm->ifm_media);
}

// Free any memory associated with the bsd mii.
// ifmedia_add() allocates memory and must be freed by
// ifmedia_delete_instance().
//
void bsd_mii_finimedia(mx6q_dev_t *mx6q)
{
	ifmedia_delete_instance(&mx6q->bsd_mii.mii_media, IFM_INST_ANY);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/devnp/mx6x/bsd_media.c $ $Rev: 806262 $")
#endif

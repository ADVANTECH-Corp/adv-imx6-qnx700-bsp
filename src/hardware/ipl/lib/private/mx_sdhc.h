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

#ifndef _MX_SDHC_INCLUDED
#define _MX_SDHC_INCLUDED

#include "sdmmc.h"

/* this speed meets the requirement that during ident the clock speed
 * should be lower tha 400k
 */
#define MX_SDMMC_CLK_DEFAULT 198000000

/*
 * SDMMC card power up wait loop
 */
#define MX_POWER_UP_WAIT               100000

//
// SDHC Register Offsets and Bitmasks
//
#define MX_MMCDMASYSADD                 0x00

#define MX_MMCBLATR                     0x04
  #define MMCBLATR_BLCNTSHIFT               16
  #define MMCBLATR_BLCNTMASK        0xFFFF0000
  #define MMCBLATR_BLSIZESHIFT               0
  #define MMCBLATR_BLSIZEMASK           0x1FFF

#define MX_MMCCMDARG                    0x08

#define MX_MMCCMDXTYPE                  0x0C
  #define MMCCMDXTYPE_CMDINXSHIFT           24
  #define MMCCMDXTYPE_CMDINXMASK    0x3F000000
  #define MMCCMDXTYPE_CMDTYPABRT     (3 << 22)
  #define MMCCMDXTYPE_CMDTYPRESUME   (2 << 22)
  #define MMCCMDXTYPE_CMDTYPSUSPEND  (1 << 22)
  #define MMCCMDXTYPE_CMDTYPNORMAL  ~(3 << 22)
  #define MMCCMDXTYPE_DPSEL          (1 << 21)
  #define MMCCMDXTYPE_CICEN          (1 << 20)
  #define MMCCMDXTYPE_CCCEN          (1 << 19)
  #define MMCCMDXTYPE_RSPTYPMASK     (3 << 16)
  #define MMCCMDXTYPE_RSPTYPL136     (1 << 16)
  #define MMCCMDXTYPE_RSPTYPL48      (2 << 16)
  #define MMCCMDXTYPE_RSPTYPL48B     (3 << 16)

#define MX_MMCCMDRSP0                   0x10
#define MX_MMCCMDRSP1                   0x14
#define MX_MMCCMDRSP2                   0x18
#define MX_MMCCMDRSP3                   0x1C
#define MX_MMCDATAPORT                  0x20

#define MX_MMCPRSNTST                   0x24
  #define MMCPRSNTST_DLSLSHIFT              24
  #define MMCPRSNTST_DLSLMASK       0xFF000000
  #define MMCPRSNTST_DL0SL           (1 << 24) //Data line 0 level. Checked for card's busy state after write transaction
  #define MMCPRSNTST_CLSL            (1 << 23)
  #define MMCPRSNTST_WPSPL           (1 << 19)
  #define MMCPRSNTST_CDPL            (1 << 18)
  #define MMCPRSNTST_CINS            (1 << 16)
  #define MMCPRSNTST_BREN            (1 << 11)
  #define MMCPRSNTST_BWEN            (1 << 10)
  #define MMCPRSNTST_RTA             (1 <<  9)
  #define MMCPRSNTST_WTA             (1 <<  8)
  #define MMCPRSNTST_SDOFF           (1 <<  7)
  #define MMCPRSNTST_PEROFF          (1 <<  6)
  #define MMCPRSNTST_HCKOFF          (1 <<  5)
  #define MMCPRSNTST_IPGOFF          (1 <<  4)
  #define MMCPRSNTST_SDSTB           (1 <<  3)
  #define MMCPRSNTST_DLA             (1 <<  2)
  #define MMCPRSNTST_CDIHB           (1 <<  1)
  #define MMCPRSNTST_CIHB            (1 <<  0)

#define MX_MMCPROTOCTL                  0x28
  #define MMCPROTOCTL_WECRM          (1 << 26)
  #define MMCPROTOCTL_WECINS         (1 << 25)
  #define MMCPROTOCTL_WECINT         (1 << 24)
  #define MMCPROTOCTL_IABG           (1 << 19)
  #define MMCPROTOCTL_RWCTL          (1 << 18)
  #define MMCPROTOCTL_CREQ           (1 << 17)
  #define MMCPROTOCTL_SABGREQ        (1 << 16)
  #define MMCPROTOCTL_DMASNSDMA     ~(1 <<  8)
  #define MMCPROTOCTL_DMASADMA       (1 <<  8)
  #define MMCPROTOCTL_CDSS           (1 <<  7)
  #define MMCPROTOCTL_CDTL           (1 <<  6)
  #define MMCPROTOCTL_EMODBE        ~(3 <<  4)
  #define MMCPROTOCTL_EMODHWBE       (1 <<  4)
  #define MMCPROTOCTL_EMODLE         (2 <<  4)
  #define MMCPROTOCTL_D3CD           (1 <<  3)
  #define MMCPROTOCTL_DTW8BIT        (2 <<  1)
  #define MMCPROTOCTL_DTW4BIT        (1 <<  1)
  #define MMCPROTOCTL_DTW1BIT       ~(3 <<  1)
  #define MMCPROTOCTL_LCTL           (1 <<  0)

#define MX_MMCSYSCTL                    0x2C
  #define MMCSYSCTL_INITA            (1 << 27)
  #define MMCSYSCTL_RSTD             (1 << 26)
  #define MMCSYSCTL_RSTC             (1 << 25)
  #define MMCSYSCTL_RSTA             (1 << 24)
  #define MMCSYSCTL_DTOCVSHIFT              16
  #define MMCSYSCTL_DTOCVMASK          0xF0000
  #define MMCSYSCTL_SDCLKFSSHIFT             8
  #define MMCSYSCTL_SDCLKFSMASK         0xFF00
  #define MMCSYSCTL_DVSSHIFT                 4
  #define MMCSYSCTL_DVSMASK               0xF0
  #define MMCSYSCTL_SDCLKEN          (1 <<  3)
  #define MMCSYSCTL_PEREN            (1 <<  2)
  #define MMCSYSCTL_HCKEN            (1 <<  1)
  #define MMCSYSCTL_IPGEN            (1 <<  0)

#define MX_MMCINTRST                    0x30
#define MX_MMCINTRSTEN                  0x34

#define MX_MMCINTRSIGEN                 0x38
//bit description common for above three registers
  #define MMCINTR_DMAE             (1 << 28)
  #define MMCINTR_AC12E            (1 << 24)
  #define MMCINTR_DEBE             (1 << 22)
  #define MMCINTR_DCE              (1 << 21)
  #define MMCINTR_DTOE             (1 << 20)
  #define MMCINTR_CIE              (1 << 19)
  #define MMCINTR_CEBE             (1 << 18)
  #define MMCINTR_CCE              (1 << 17)
  #define MMCINTR_CTOE             (1 << 16)
  #define MMCINTR_CINT             (1 <<  8)
  #define MMCINTR_CRM              (1 <<  7)
  #define MMCINTR_CINS             (1 <<  6)
  #define MMCINTR_BRR              (1 <<  5)
  #define MMCINTR_BWR              (1 <<  4)
  #define MMCINTR_DINT             (1 <<  3)
  #define MMCINTR_BGE              (1 <<  2)
  #define MMCINTR_TC               (1 <<  1)
  #define MMCINTR_CC               (1 <<  0)
  #define MMCINTR_ERRI             (MMCINTR_AC12E | MMCINTR_DEBE | MMCINTR_DCE | MMCINTR_DTOE | MMCINTR_CIE | MMCINTR_CEBE | MMCINTR_CCE | MMCINTR_CTOE)

#define MX_MMCAUTOCMD12ST               0x3C
  #define MMCAUTOCMD12ST_CNIBAC12E   (1 <<  7)
  #define MMCAUTOCMD12ST_AC12IE      (1 <<  4)
  #define MMCAUTOCMD12ST_AC12CE      (1 <<  3)
  #define MMCAUTOCMD12ST_AC12EBE     (1 <<  2)
  #define MMCAUTOCMD12ST_AC12TOE     (1 <<  1)
  #define MMCAUTOCMD12ST_AC12NE      (1 <<  0)

#define MX_MMCHCTLRCAP                  0x40
  #define MMCHCTLRCAP_VS18           (1 << 26)
  #define MMCHCTLRCAP_VS30           (1 << 25)
  #define MMCHCTLRCAP_VS33           (1 << 24)
  #define MMCHCTLRCAP_SRAS           (1 << 23)
  #define MMCHCTLRCAP_DMAS           (1 << 22)
  #define MMCHCTLRCAP_HSS            (1 << 21)
  #define MMCHCTLRCAP_ADMAS          (1 << 20)
  #define MMCHCTLRCAP_MBL512         (0 << 16)
  #define MMCHCTLRCAP_MBL1024        (1 << 16)
  #define MMCHCTLRCAP_MBL2048        (2 << 16)
  #define MMCHCTLRCAP_MBL4096        (3 << 16)

#define MX_MMCWATML                     0x44
  #define MMCWATML_WRBRSTLENSHIFT           24
  #define MMCWATML_WRBRSTLENMASK    0x1F000000
  #define MMCWATML_WRWMLSHIFT               16
  #define MMCWATML_WRWMLMASK          0xFF0000
  #define MMCWATML_RDBRSTLENSHIFT            8
  #define MMCWATML_RDBRSTLENMASK        0x1F00
  #define MMCWATML_RDWMLSHIFT                0
  #define MMCWATML_RDWMLMASK              0xFF

#define MX_MMCMIXCTRL                  0x48
  #define MMCMIXCTRL_FBCLKSEL       (1 << 25)
  #define MMCMIXCTRL_AUTOTUNEEN     (1 << 24)
  #define MMCMIXCTRL_SMPCLKSEL      (1 << 23)
  #define MMCMIXCTRL_EXETUNE        (1 << 22)
  #define MMCMIXCTRL_MSBSEL         (1 <<  5)
  #define MMCMIXCTRL_DTDSEL         (1 <<  4)
  #define MMCMIXCTRL_DDR_EN         (1 <<  3)
  #define MMCMIXCTRL_AC12EN         (1 <<  2)
  #define MMCMIXCTRL_BCEN           (1 <<  1)
  #define MMCMIXCTRL_DMAEN          (1 <<  0)

#define MX_MMCFORCEEVENT                0x50
#define MX_MMCADMAERRST                 0x54
#define MX_MMCADMASYSADD                0x58
#define MX_MMCHCTLRVER                  0xFC

extern void mx_sdmmc_init_hc(sdmmc_t *sdmmc, unsigned base, unsigned clock, int verbose);
extern int mx_sdmmc_fini(sdmmc_t *sdmmc);

#endif /* #ifndef _MX_SDHC_INCLUDED */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/private/mx_sdhc.h $ $Rev: 795037 $")
#endif


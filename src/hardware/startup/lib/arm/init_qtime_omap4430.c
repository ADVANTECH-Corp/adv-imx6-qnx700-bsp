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

/*
 * TI OMAP4430 specific timer support.
 * GPT1 is used.
 */

#include "startup.h"

#define OMAP44XX_NUM_INTERNAL_IRQS       32
#define OMAP44XX_GPT1_IRQ               (37+OMAP44XX_NUM_INTERNAL_IRQS)

#define OMAP44XX_GPT_OFF_TIOCP_CFG 0x10
#define OMAP44XX_GPT_OFF_TISR      0x18
#define OMAP44XX_GPT_OFF_TIER      0x1c
#define OMAP44XX_GPT_OFF_TCLR      0x24
#define OMAP44XX_GPT_OFF_TCRR      0x28
#define OMAP44XX_GPT_OFF_TLDR      0x2c
#define OMAP44XX_GPT_OFF_TSICR     0x40
#define OMAP44XX_GPT_SIZE         0x100

#define OMAP44XX_GPT1MS_TIOCP_CFG_SOFT_RESET 0x2

#define OMAP44XX_GPTIMER1_GPT_TIDR 0x4a318000

#define OMAP44XX_GPT_TCLR_CE              0x40
#define OMAP44XX_GPT_TCLR_AR              0x02
#define OMAP44XX_GPT_TCLR_ST              0x01
#define OMAP44XX_GPT_TISR_MATCH           0x01

#define OMAP44XX_CM_SYS_CLKSEL 0x4a306110
#define OMAP44XX_CM_SYS_CLKSEL_MASK          0x7
#define OMAP44XX_CM_SYS_CLKSEL_UNINITIALIZED   0

#define OMAP44XX_CM_WKUP_CLKSTCTRL 0x4a307800
#define OMAP44XX_CM_WKUP_CLKSTCTRL_CLKTRCTRL               0x3

#define OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL 0x4a307840
#define OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL_CLKSEL        (1<<24)
#define OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL_MODULEMODE       0x3
#define OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL_MODULEMODE_ON    0x2
#define OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL_IDLEST        0x3000

static uintptr_t timer_base;
#define TIMER_FIELD(_fn) \
  (timer_base + (OMAP44XX_GPT_OFF_ ## _fn))

static const struct callout_slot timer_callouts[] = {
  { CALLOUT_SLOT(timer_load, _omap4430) },
  { CALLOUT_SLOT(timer_value, _omap4430) },
  { CALLOUT_SLOT(timer_reload, _omap4430) },
};

/*
 * These functions are used to calibrate the inline delay loop functions.
 * They aren't used after the kernel comes up.
 */
static unsigned
timer_start_omap4430()
{
  out32(TIMER_FIELD(TCLR), in32(TIMER_FIELD(TCLR)) | OMAP44XX_GPT_TCLR_ST);

  return in32(TIMER_FIELD(TCRR));
}

static unsigned
timer_diff_omap4430(unsigned start)
{
  unsigned now = in32(TIMER_FIELD(TCRR));

  return (now - start);
}

extern const uint32_t clkin_freqs[];
#define  OMAP_CLOCK_SCALE  -15
// these calculated as round(1/clkin_freq[i]*10^-OMAP_CLOCK_SCALE)
const uint32_t clkin_rates[] =
{
  0,          // OMAP44XX_CM_SYS_CLKSEL_UNINITIALIZED
  83333333UL, // OMAP44XX_CM_SYS_CLKSEL_12M
  76923077UL, // OMAP44XX_CM_SYS_CLKSEL_13M
  59523810UL, // OMAP44XX_CM_SYS_CLKSEL_16P8M
  52083333UL, // OMAP44XX_CM_SYS_CLKSEL_19P2M
  38461538UL, // OMAP44XX_CM_SYS_CLKSEL_26M
  37037037UL, // OMAP44XX_CM_SYS_CLKSEL_27M
  26041667UL  // OMAP44XX_CM_SYS_CLKSEL_38P4M
};

void
init_qtime_omap4430()
{
  unsigned sys_clkin_sel;
  struct qtime_entry  *qtime = alloc_qtime();

  // prevent sleep
  out32(OMAP44XX_CM_WKUP_CLKSTCTRL,
        in32(OMAP44XX_CM_WKUP_CLKSTCTRL)&~OMAP44XX_CM_WKUP_CLKSTCTRL_CLKTRCTRL);

  // select sys_clk and enable GPT1
  out32(OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL,
        (in32(OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL) & 
         ~(OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL_CLKSEL |
           OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL_MODULEMODE)) |
         OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL_MODULEMODE_ON);

  // wait for module to wake up
  while ((in32(OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL) &
          OMAP44XX_CM_WKUP_GPTIMER1_CLKCTRL_IDLEST));

  /*
  * Map GPT1 registers
  */
  timer_base = startup_io_map(OMAP44XX_GPT_SIZE, OMAP44XX_GPTIMER1_GPT_TIDR);

  /* Reset the timer */
  out32(TIMER_FIELD(TIOCP_CFG), OMAP44XX_GPT1MS_TIOCP_CFG_SOFT_RESET);

  /* Wait for reset to complete */
  while (in32(TIMER_FIELD(TIOCP_CFG)) & OMAP44XX_GPT1MS_TIOCP_CFG_SOFT_RESET);

  /* Set posted mode */
  out32(TIMER_FIELD(TSICR), 0);

  /* Clear timer count and reload count */
  out32(TIMER_FIELD(TLDR), 0);
  out32(TIMER_FIELD(TCRR), 0);

  /* Enable interrupts */
  out32(TIMER_FIELD(TIER), OMAP44XX_GPT_TISR_MATCH);

  /*
  * Setup GPT1
  * Enable Auto-reload
  * Prescaler disable
  * Enable timer
  */
  out32(TIMER_FIELD(TCLR), OMAP44XX_GPT_TCLR_AR | OMAP44XX_GPT_TCLR_ST );

  timer_start = timer_start_omap4430;
  timer_diff  = timer_diff_omap4430;

  qtime->intr = OMAP44XX_GPT1_IRQ;  /* GPT1 irq */

  sys_clkin_sel = in32(OMAP44XX_CM_SYS_CLKSEL) & OMAP44XX_CM_SYS_CLKSEL_MASK;
  if (sys_clkin_sel == OMAP44XX_CM_SYS_CLKSEL_UNINITIALIZED)
  {
    crash("Error: CM_SYS_CLKSEL is not configured.\n");
  }
  qtime->timer_rate = clkin_rates[sys_clkin_sel];
  qtime->timer_scale = OMAP_CLOCK_SCALE;
  qtime->cycles_per_sec = (uint64_t)clkin_freqs[sys_clkin_sel];

#ifdef	FIXME
  arm_add_clock_cycles(&clock_cycles_omap4xx, 32);
#endif
   
  add_callout_array(timer_callouts, sizeof(timer_callouts));
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/lib/arm/init_qtime_omap4430.c $ $Rev: 781278 $")
#endif

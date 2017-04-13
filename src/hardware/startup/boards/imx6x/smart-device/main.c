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


/*
 * i.MX6Q Sabre-Lite board with Cortex-A9 MPCore
 */

#include "startup.h"
#include <time.h>
#include "board.h"
#include <stdbool.h>

/* callout prototype */
extern struct callout_rtn reboot_mx6x;

extern int dcache_enable;
extern int enable_TZAC;

int enable_norspi;
int enable_bt;

/* additional setup for certain generic peripherals */
extern void mx6q_usb_otg_host_init(void);
extern void mx6q_usb_host1_init(void);
extern void init_sata(void);
extern void init_audio(void);

/* iomux configuration for different peripherals */
extern void mx6q_init_ecspi(void);
extern void mx6q_init_i2c(void);
extern void mx6q_init_usdhc(void);
extern void mx6s_init_emmcNandFlash(void);
extern void mx6q_init_displays(void);
extern void mx6q_init_lvds(void);
extern void mx6q_init_lcd_panel(void);
extern void mx6q_init_sata(void);
extern void mx6q_init_audmux_pins(void);
extern void mx6q_init_enet(void);
extern void mx6q_init_mac(void);
extern void mx6q_init_pcie(void);
/* weilun@adv - begin */
//extern void mx6q_init_sensors(void);
//extern void mx6q_init_external_power_rails(void);
//extern unsigned mx6x_init_lvds_clock(void);
/* weilun@adv - end */
extern void mx6q_init_wifi(void);
extern void mx6q_init_bt(void);
extern void init_ram(unsigned gpu_start, unsigned ram_size);

/* weilun@adv debug - begin */
extern void mx6q_init_debug();
/* weilun@adv debug - end */

const struct callout_slot callouts[] = {
	{ CALLOUT_SLOT( reboot, _mx6x) },
};

/* The USB-Serial is on UART_1 */
const struct debug_device debug_devices[] = {
	{	"mx1",
#if defined(CONFIG_MACH_MX6Q_DMS_BA16)
		{"0x021F0000^0.115200.80000000.16",
#else
		{"0x02020000^0.115200.80000000.16",
#endif
		},
		init_mx1,
		put_mx1,
		{	&display_char_mx1,
			&poll_key_mx1,
			&break_detect_mx1,
		}
	},
};


unsigned mx6x_per_clock = 0;
uint32_t uart_clock;


/*
 * main()
 *	Startup program executing out of RAM
 *
 * 1. It gathers information about the system and places it in a structure
 *    called the system page. The kernel references this structure to
 *    determine everything it needs to know about the system. This structure
 *    is also available to user programs (read only if protection is on)
 *    via _syspage->.
 *
 * 2. It (optionally) turns on the MMU and starts the next program
 *    in the image file system.
 */
int
main(int argc, char **argv, char **envv)
{
	int		opt, options = 0;
	bool		imx6_quad_dual;
	bool		imx6_quad_plus;

	/*
	/ Note: The last 256 MB of SDRAM is reserved for the GPU when TrustZone is enabled
	/ gpu_reserved_start: Start address of reserved SDRAM for GPU usage
	*/
	unsigned gpu_reserved_start = MX6X_SDRAM_64_BIT_BASE + MEG(MX6X_SDRAM_SIZE - GPU_SDRAM_SIZE);

	/*
	 * Initialise debugging output
	 */
	select_debug(debug_devices, sizeof(debug_devices));

	add_callout_array(callouts, sizeof(callouts));

	enable_norspi = FALSE;
	enable_bt = FALSE;
	enable_TZAC = FALSE;

	/* identify the imx6 processor we're running on */
	imx6_quad_dual = (get_mx6_chip_type()==MX6_CHIP_TYPE_QUAD_OR_DUAL) ? TRUE:FALSE;
	imx6_quad_plus = (imx6_quad_dual && ( get_mx6_chip_rev()==MX6_CHIP_REV_2_0)) ? TRUE:FALSE;

	// common options that should be avoided are:
	// "AD:F:f:I:i:K:M:N:o:P:R:S:Tvr:j:Z"
	while ((opt = getopt(argc, argv, COMMON_OPTIONS_STRING "bmnstW")) != -1) {
		switch (opt) {
			case 'b':
				enable_bt = TRUE;
				break;
			case 'm':
				dcache_enable = TRUE;
				break;
			case 'n':
				enable_norspi = TRUE;
				break;
			case 't':
				enable_TZAC = TRUE;
				break;
			case 'W':
				options |= MX6X_WDOG_ENABLE;
				break;
			default:
				handle_common_option(opt);
				break;
		}
	}

	if (options & MX6X_WDOG_ENABLE) {
		/*
		* Enable WDT
		*/
		mx6x_wdg_reload();
		mx6x_wdg_enable();
	}

	/* Make sure to not allocate anything in the SDRAM which is in GPU SDRAM TrustZone Area */
	if (enable_TZAC)
		avoid_ram(gpu_reserved_start, MEG(GPU_SDRAM_SIZE));

	/*
	 * Collect information on all free RAM in the system
	 */
	init_ram(gpu_reserved_start, MX6X_SDRAM_SIZE);

	/*
	 * set CPU frequency, currently max stable CPU freq is 792MHz
	 */
	if (cpu_freq == 0)
		cpu_freq = mx6x_get_cpu_clk();

	/*
	 * Remove RAM used by modules in the image
	 */
	alloc_ram(shdr->ram_paddr, shdr->ram_size, 1);

	/*
	  * Initialize SMP
	  */
	init_smp();

	/*
	  * Initialize MMU
	  */
	if (shdr->flags1 & STARTUP_HDR_FLAGS1_VIRTUAL)
		init_mmu();

	/* Initialize the Interrupts related Information */
	init_intrinfo();

	/* Initialize the Timer related information */
	mx6x_init_qtime();

	/* Init Clocks (must happen after timer is initialized */
	mx6x_init_clocks();

	/* Init L2 Cache Controller */
	init_cacheattr();

	/* Initialize the CPU related information */
	init_cpuinfo();

	/* Initialize the Hwinfo section of the Syspage */
	init_hwinfo();

	if(imx6_quad_dual)
	{
		if(imx6_quad_plus)
		{
			add_typed_string(_CS_MACHINE, "i.MX6 QuadPlus Sabre-Smart Board");
		}
		else
		{
			add_typed_string(_CS_MACHINE, "i.MX6 QuadorDual Sabre-Smart Board");
		}
	}

	/* AIPSTZ init */
	mx6x_init_aipstz();

	/* weilun@adv - begin */
#if 0
	/* turn on external power domains */
	mx6q_init_external_power_rails();

	/* Interrupt GPIO lines from SabreSmart sensors */
	mx6q_init_sensors();
#endif
	/* weilun@adv - end */

	/* Configure PIN MUX to enable i2c */
	mx6q_init_i2c();

	/* Init audio to support WM8962 codec */
	mx6q_init_audmux_pins();
	init_audio();

	/* Test if LDO_PU is gated (responsible for VPU/GPU power domain) */
	if (pmu_get_voltage(LDO_PU) == CPU_POWER_GATED_OFF_VOLTAGE)
	{
		pmu_power_up_gpu();
	}

	/* Set GPU3D clocks */
	mx6x_init_gpu3D();

	/*
	 * If using an LVDS display mx6x_init_lvds_clock() should be called which will select
	 * PLL5 as the LDB clock source.
	 */
	/* weilun@adv fix screen:win_get_property: invalid property name 103 - begin */
#if 0
	mx6x_init_lvds_clock();
#else
	if (debug_flag)
		kprintf("weilun@adv don't call mx6x_init_lvds_clock()\n");
#endif
	/* weilun@adv fix screen:win_get_property: invalid property name 103 - end */

	/* Configure PIN MUX to enable SD */
	mx6q_init_usdhc();

	/* Init USB OTG, H1 */
	mx6q_usb_otg_host_init();
	mx6q_usb_host1_init();

	/*
	 * NORSPI conflicts with BT AND WIFI. Note that NOR SPI chip is by default not populated
	 * and WiFi/BT requires an external adapter using SD slot 2 and the J13 20 pin connector.
	 *
	 * BT conflicts with PCIe.
	 */

	if (enable_norspi && enable_bt)
	{
		kprintf("Cannot enable both NOR SPI and BT! Disabling NOR SPI.");
		enable_norspi = FALSE;
	}

	if (enable_norspi)
	{
		mx6q_init_ecspi();
		mx6q_init_pcie();
		if (debug_flag)
			kprintf("Enabling NOR SPI and PCIe, BT/WiFi will be unusable!\n");
	}
	else
	{
		mx6q_init_wifi();
		if (enable_bt)
		{
			mx6q_init_bt();
			if (debug_flag)
				kprintf("Enabling BT and WiFi, NOR SPI and PCIe will be disabled!\n");
		}
		else
		{
			mx6q_init_pcie();
			if (debug_flag)
				kprintf("Enabling PCIe and WiFi, BT will be disabled!\n");
		}
	}

	/* Configure pins for LVDS, LCD display*/
	mx6q_init_displays();

	/* ENET */
	mx6q_init_mac();
	mx6q_init_enet();

	/* SATA */
	mx6x_init_sata(ANATOP_PLL8_ENET_REF_ENET_50M);



	/* Report peripheral clock frequencies */
	mx6x_dump_clocks();

	/*
	 * Load bootstrap executables in the image file system and Initialise
	 * various syspage pointers. This must be the _last_ initialisation done
	 * before transferring control to the next program.
	 */
	init_system_private();

	/*
	 * This is handy for debugging a new version of the startup program.
	 * Commenting this line out will save a great deal of code.
	 */
	
	/* weilun@adv debug - begin */
	mx6q_init_debug();
#if defined(CONFIG_MACH_MX6Q_DMS_BA16)
	/* FIXME */
	mx6q_init_ecspi();
#endif
	/* weilun@adv debug - end */

	print_syspage();
	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/imx6x/smart-device/main.c $ $Rev: 823211 $")
#endif

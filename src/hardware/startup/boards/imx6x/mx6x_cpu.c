/*
 * $QNXLicenseC: 
 * Copyright 2012 QNX Software Systems.  
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

#include "startup.h"
#include "mx6x_startup.h"
#include <arm/mx6x.h>

#define CHIP_STRING_SIZE 30

uint32_t get_mx6_chip_rev()
{
	uint32_t chip_rev = 0;
	uint32_t dig_prog = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_CHIP_INFO);

	/* Check the major field */
	if ( ((dig_prog >> 8) & 0xFF) == 0)
	{
		/* 
		   1.x series.
		   DEPRECATED: This method of checking the chip revision ignores the 
		   major field (it was '0' anyways. Keeping it here for backwards compatability 
		*/
		chip_rev = dig_prog & 0xFF;
	}
	else
	{
		/* 
			>= 2.x series 
		   Encode major and minor chip rev fields into one byte. 
		   Upper nibble is major, lower is minor 
		*/
		uint32_t major = ((dig_prog & 0xFF00) >> 8) + 1;
		uint32_t minor = dig_prog & 0xFF;
		chip_rev = (major<< 4) | (minor);
	}

	return chip_rev;
}

uint32_t get_mx6_chip_type()
{
	uint32_t chip_id = in32(MX6X_ANATOP_BASE + MX6X_ANADIG_CHIP_INFO);
	chip_id >>= 16;
	return chip_id;
}
void print_chip_info()
{
	uint32_t chip_type = get_mx6_chip_type();
	uint32_t chip_rev = get_mx6_chip_rev();

	char chip_type_str[CHIP_STRING_SIZE];
	char chip_rev_str[CHIP_STRING_SIZE];

	switch(chip_type)
	{
		case MX6_CHIP_TYPE_QUAD_OR_DUAL:
			strcpy(chip_type_str, "Dual/Quad");
			break;

		case MX6_CHIP_TYPE_DUAL_LITE_OR_SOLO:
			strcpy(chip_type_str, "Solo/DualLite");
			break;
		default:
			strcpy(chip_type_str, "Unknown Variant");
			break;
	}

	switch(chip_rev)
	{
		case MX6_CHIP_REV_1_0:
			strcpy(chip_rev_str, "TO1.0");
			break;
		case MX6_CHIP_REV_1_1:
			strcpy(chip_rev_str, "TO1.1");
			break;
		case MX6_CHIP_REV_1_2:
			strcpy(chip_rev_str, "TO1.2");
			break;
		case MX6_CHIP_REV_1_3:
			strcpy(chip_rev_str, "TO1.3");
			break;
		case MX6_CHIP_REV_1_4:
			strcpy(chip_rev_str, "TO1.4");
			break;
		case MX6_CHIP_REV_1_5:
			/*
			 * i.MX6DQ TO1.5 is defined as Rev 1.3 in Data Sheet, marked
			 * as 'D' in Part Number last character.
			 */
			strcpy(chip_rev_str, "TO1.5");
			break;
		case MX6_CHIP_REV_2_0:
			strcpy(chip_rev_str, "TO2.0");
			break;
		default:
			strcpy(chip_rev_str, "Unknown Revision");
			kprintf("unknown revision: 0x%x\n", chip_rev);
			break;
	}
	kprintf("Detected i.MX6 %s, revision %s\n", chip_type_str, chip_rev_str);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/startup/boards/imx6x/mx6x_cpu.c $ $Rev: 783761 $")
#endif

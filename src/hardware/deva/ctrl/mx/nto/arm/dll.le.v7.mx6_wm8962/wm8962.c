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

#include <audio_driver.h>
#include <string.h>

#include <hw/i2c.h>
#include <stdint.h>
#include "mxssi.h"
#include "wm8962.h"

/* One playback device, one capture device. */
static int32_t pcm_devices[1] = {
    0
};

static snd_mixer_voice_t stereo_voices[2] = {
    {SND_MIXER_VOICE_LEFT, 0},
    {SND_MIXER_VOICE_RIGHT, 0}
};

/* mindB and maxdB values are all x100 */
static struct snd_mixer_element_volume1_range spk_range[2] = {
    {0, 0x4A, -68000, 6000},
    {0, 0x4A, -68000, 6000},
};

/* mindB and maxdB values are all x100 */
static struct snd_mixer_element_volume1_range hp_range[2] = {
    {0, 0x4A, -68000, 6000},
    {0, 0x4A, -68000, 6000},
};

/* DAC Master Range */
static struct snd_mixer_element_volume1_range master_range[2] = {
    {0, 0xFE, -71625, 23625},
    {0, 0xFE, -71625, 23625},
};

/* ADC Range */
static struct snd_mixer_element_volume1_range adc_range[2] = {
    {0, 0xFE, -71625, 23625},
    {0, 0xFE, -71625, 23625},
};

/***********************************************
 * read/write interface to wm8962 codec *
 ***********************************************/
/* read codec */
static uint32_t
codec_read (MIXER_CONTEXT_T * mixer, uint32_t reg)
{
    struct {
        i2c_send_t      hdr;
        uint16_t        reg;
    } msgreg;
    struct {
        i2c_recv_t      hdr;
        uint16_t        val;
    } msgval;
    int rbytes;

    msgreg.hdr.slave.addr = WM8962_SLAVE_ADDR;
    msgreg.hdr.slave.fmt  = I2C_ADDRFMT_7BIT;
    msgreg.hdr.len        = 2;
    msgreg.hdr.stop       = 1;
    msgreg.reg            = ((reg & 0xff) << 8) | ((reg & 0xff00) >> 8);

    int ret = devctl(mixer->i2c_fd, DCMD_I2C_SEND, &msgreg,
            sizeof(msgreg), NULL);

    if (ret) {
        ado_error ("WM8962: I2C_READ ADDR failed: %d", ret);
        return (-1);
    }

    msgval.hdr.slave.addr = WM8962_SLAVE_ADDR;
    msgval.hdr.slave.fmt  = I2C_ADDRFMT_7BIT;
    msgval.hdr.len        = 2;
    msgval.hdr.stop       = 1;

    if (devctl(mixer->i2c_fd, DCMD_I2C_RECV, &msgval,
                        sizeof(msgval), &rbytes)) {
        ado_error ("WM8962: I2C_READ DATA failed.");
        return (-1);
    }

    msgval.val = ((msgval.val & 0xff) << 8) | ((msgval.val & 0xff00) >> 8);

    return (msgval.val);
}

/* write codec */
static int
codec_write(MIXER_CONTEXT_T * mixer, uint32_t reg, uint32_t val)
{
    struct {
        i2c_send_t      hdr;
        uint16_t        reg;
        uint16_t        val;
    } msg;

    msg.hdr.slave.addr = WM8962_SLAVE_ADDR;
    msg.hdr.slave.fmt  = I2C_ADDRFMT_7BIT;
    msg.hdr.len        = 4;
    msg.hdr.stop       = 1;
    msg.reg            = ((reg & 0xff) << 8) | ((reg & 0xff00) >> 8);
    msg.val            = ((val & 0xff) << 8) | ((val & 0xff00) >> 8);

    if (devctl (mixer->i2c_fd, DCMD_I2C_SEND, &msg, sizeof(msg), NULL))
    {
        ado_error ("WM8962: I2C_WRITE failed.");
    }

    return (EOK);
}

/*
 * Provides a non-destructive way of modifying register contents.
 */
static int codec_update(MIXER_CONTEXT_T * wm8962, uint32_t reg, uint32_t mask, uint32_t val)
{
    uint32_t current_value = codec_read(wm8962, reg);
    current_value &= ~mask;

    return codec_write(wm8962, reg, current_value | val);
}

/*************************************
 *  wm8962 codec internal control  *
 *************************************/
/* Speaker mute control . */

static int32_t
wm8962_spk_mute_control (MIXER_CONTEXT_T * wm8962,
    ado_mixer_delement_t * element, uint8_t set, uint32_t * vol,
    void *instance_data)
{
	int32_t altered = 0;
	if (set)
	{
		altered = (wm8962->spk_mute != (vol[0] & (SND_MIXER_CHN_MASK_FRONT_LEFT |
				SND_MIXER_CHN_MASK_FRONT_RIGHT))) ? 1 : 0;
		if (altered)
		{
			if (vol[0] & SND_MIXER_CHN_MASK_FRONT_LEFT)
				wm8962->spk_mute |= SND_MIXER_CHN_MASK_FRONT_LEFT;
			else
				wm8962->spk_mute &= ~SND_MIXER_CHN_MASK_FRONT_LEFT;

			if (vol[0] & SND_MIXER_CHN_MASK_FRONT_RIGHT)
				wm8962->spk_mute |= SND_MIXER_CHN_MASK_FRONT_RIGHT;
			else
				wm8962->spk_mute &= ~SND_MIXER_CHN_MASK_FRONT_RIGHT;

			/* Apply mute update */
			codec_update(wm8962, SPKOUTL_VOLUME, SPKOUTL_VU, 0);
			codec_update(wm8962, CLASSDCTL_1, (SPKOUTR_PGA_MUTE | SPKOUTL_PGA_MUTE), wm8962->spk_mute);
			/*
			* The VU bits for the speakers are in a different register to the mute
			* bits and only take effect on the PGA if it is actually powered.
			*/
			codec_update(wm8962, SPKOUTL_VOLUME, SPKOUTL_VU, SPKOUTL_VU);
		}
	}
	else
	{
		vol[0] = wm8962->spk_mute;
	}

	return (altered);
}

static int32_t
wm8962_spk_vol_control (MIXER_CONTEXT_T * wm8962,
    ado_mixer_delement_t * element, uint8_t set, uint32_t * vol,
    void *instance_data)
{
    int32_t altered = 0;
    uint16_t spkoutl_vol, spkoutr_vol;

    spkoutl_vol = codec_read (wm8962, SPKOUTL_VOLUME);
    spkoutr_vol = codec_read (wm8962, SPKOUTR_VOLUME);

    if (set)
    {
        // This offset is used since spk_range was shifted to start from 0
        vol[0] += 0x35;
        vol[1] += 0x35;

        altered = ((vol[0] != (spkoutl_vol & SPKOUTL_VOL_MASK)) || (vol[1] !=  (spkoutr_vol & SPKOUTR_VOL_MASK)));

        if (altered)
        {
            codec_update(wm8962, SPKOUTL_VOLUME, SPKOUTL_VU, 0);
            codec_update(wm8962, SPKOUTL_VOLUME, SPKOUTL_VOL_MASK, SPKOUTL_VOL(vol[0]));
            codec_update(wm8962, SPKOUTR_VOLUME, SPKOUTR_VOL_MASK, SPKOUTR_VOL(vol[1]));
            codec_update(wm8962, SPKOUTL_VOLUME, SPKOUTL_VU, SPKOUTL_VU);
        }
    }
    else
    {
        // This offset is used since spk_range was shifted to start from 0
        vol[0] = (spkoutl_vol & SPKOUTL_VOL_MASK) - 0x35;
        vol[1] = (spkoutr_vol & SPKOUTR_VOL_MASK) - 0x35;
    }

    return (altered);
}

/* Headphone mute control. */
static int32_t
wm8962_hp_mute_control (MIXER_CONTEXT_T * wm8962,
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol,
	void *instance_data)
{
	int32_t altered = 0;

	if (set)
	{
		altered = (wm8962->hp_mute != (vol[0] & (SND_MIXER_CHN_MASK_FRONT_LEFT |
				SND_MIXER_CHN_MASK_FRONT_RIGHT))) ? 1 : 0;

		if (altered)
		{
			codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VU, 0);

			if (vol[0] & SND_MIXER_CHN_MASK_FRONT_LEFT)
			{
				wm8962->hp_mute |= SND_MIXER_CHN_MASK_FRONT_LEFT;
				codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VOL_MASK, 0x0);	/* Mute */
			}
			else
			{
				wm8962->hp_mute &= ~SND_MIXER_CHN_MASK_FRONT_LEFT;
				codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VOL_MASK, HPOUTL_VOL(wm8962->hp_vol[0]));
			}

			if (vol[0] & SND_MIXER_CHN_MASK_FRONT_RIGHT)
			{
				wm8962->hp_mute |= SND_MIXER_CHN_MASK_FRONT_RIGHT;
				codec_update(wm8962, HPOUTR_VOLUME, HPOUTR_VOL_MASK, 0x0);	/* Mute */
			}
			else
			{
				wm8962->hp_mute &= ~SND_MIXER_CHN_MASK_FRONT_RIGHT;
				codec_update(wm8962, HPOUTR_VOLUME, HPOUTR_VOL_MASK, HPOUTR_VOL(wm8962->hp_vol[1]));
			}
			codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VU, HPOUTL_VU);	/* Apply volume update */
		}
	}
	else
	{
		vol[0] = wm8962->hp_mute;
	}

	return (altered);
}

static int32_t
wm8962_hp_vol_control (MIXER_CONTEXT_T * wm8962,
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol,
	void *instance_data)
{
	int32_t altered = 0;

	if (set)
	{
		// This offset is used since spk_range was shifted to start from 0
		vol[0] += 0x35;
		vol[1] += 0x35;

		altered = ((vol[0] != wm8962->hp_vol[0])	|| (vol[1] != wm8962->hp_vol[1]));

		if (altered)
		{
			wm8962->hp_vol[0] = vol[0];
			wm8962->hp_vol[1] = vol[1];

			codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VU, 0);
			if (!(wm8962->hp_mute & SND_MIXER_CHN_MASK_FRONT_LEFT))
				codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VOL_MASK, HPOUTL_VOL(wm8962->hp_vol[0]));
			if (!(wm8962->hp_mute & SND_MIXER_CHN_MASK_FRONT_RIGHT))
				codec_update(wm8962, HPOUTR_VOLUME, HPOUTR_VOL_MASK, HPOUTR_VOL(wm8962->hp_vol[1]));
			codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VU, HPOUTL_VU);
		}
	}
	else
	{
		vol[0] = wm8962->hp_vol[0] - 0x35;
		vol[1] = wm8962->hp_vol[1] - 0x35;
	}

	return (altered);
}

/* DAC Master mute control. */
static int32_t
wm8962_dac_mute_control (MIXER_CONTEXT_T * wm8962,
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol,
	void *instance_data)
{
	int32_t altered = 0;

	if (set)
	{
		if (vol[0] > wm8962->dac_mute)
		{
			wm8962->dac_mute = SND_MIXER_CHN_MASK_FRONT_LEFT | SND_MIXER_CHN_MASK_FRONT_RIGHT;
			altered = 1;
		}
		else if (vol[0] < wm8962->dac_mute)
		{
			wm8962->dac_mute = 0;
			altered = 1;
		}

		if (altered)
		{
			/* Apply mute update */
			codec_update(wm8962, LDAC_VOLUME, DACL_VU, 0);
			//R5_ADC&DAC control1 register bit 3: DACMUTE bit applies to both left and right channels
			codec_update(wm8962, ADCDACTL1, DAC_MUTE, wm8962->dac_mute ? DAC_MUTE : 0);
			codec_update(wm8962, LDAC_VOLUME, DACL_VU, DACL_VU);
		}
	}
	else
	{
		vol[0] = wm8962->dac_mute;
	}

	return (altered);
}

static int32_t
wm8962_dac_vol_control (MIXER_CONTEXT_T * wm8962,
	ado_mixer_delement_t * element, uint8_t set, uint32_t * vol,
	void *instance_data)
{
	int32_t altered = 0;
	uint16_t dacoutl_vol, dacoutr_vol;

	dacoutl_vol = codec_read (wm8962, LDAC_VOLUME);
	dacoutr_vol = codec_read (wm8962, RDAC_VOLUME);

	if (set)
	{
		// This offset is used since spk_range was shifted to start from 0
		vol[0] += 1;
		vol[1] += 1;

		altered = ((vol[0] != ((dacoutl_vol) & DACL_VOL_MASK))
				|| (vol[1] !=  ((dacoutr_vol) & DACR_VOL_MASK)));

		if (altered)
		{
			codec_update(wm8962, LDAC_VOLUME, DACL_VU, 0);
			codec_update(wm8962, LDAC_VOLUME, DACL_VOL_MASK, DACL_VOL(vol[0]));
			codec_update(wm8962, RDAC_VOLUME, DACR_VOL_MASK, DACR_VOL(vol[1]));
			codec_update(wm8962, LDAC_VOLUME, DACL_VU, DACL_VU);
		}
	}
	else
	{
		vol[0] = (dacoutl_vol & DACL_VOL_MASK) - 1;
		vol[1] = (dacoutr_vol & DACR_VOL_MASK) - 1;
	}

	return (altered);
}

/*
 * ADC mute control - used for MIC_IN.
 */
static int32_t
wm8962_adc_mute_control (MIXER_CONTEXT_T * wm8962,
    ado_mixer_delement_t * element, uint8_t set, uint32_t * vol,
    void *instance_data)
{
	int32_t altered = 0;

	if (set)
	{
		uint16_t lrswap = 0;

		/* Check if we are configured left and right channels swap */
		lrswap = codec_read(wm8962, AUDIO_INTERFACE_0);
		lrswap &= ADC_LRSWAP;

		altered = (wm8962->adc_mute != (vol[0] & (SND_MIXER_CHN_MASK_FRONT_LEFT |
				SND_MIXER_CHN_MASK_FRONT_RIGHT))) ? 1 : 0;
		if (altered)
		{
			codec_update(wm8962, ADCR_VOLUME, ADCR_VU, 0);

			if (vol[0] & SND_MIXER_CHN_MASK_FRONT_LEFT)
			{
				wm8962->adc_mute |= SND_MIXER_CHN_MASK_FRONT_LEFT;
				if (lrswap)
					codec_update(wm8962, ADCR_VOLUME, ADCR_VOL_MASK, 0x0);	/* Mute */
				else
					codec_update(wm8962, ADCL_VOLUME, ADCL_VOL_MASK, 0x0);	/* Mute */
			}
			else
			{
				wm8962->adc_mute &= ~SND_MIXER_CHN_MASK_FRONT_LEFT;
				if (lrswap)
					codec_update(wm8962, ADCR_VOLUME, ADCR_VOL_MASK, wm8962->adc_vol[0]);
				else
					codec_update(wm8962, ADCL_VOLUME, ADCL_VOL_MASK, wm8962->adc_vol[0]);
			}

			if (vol[0] & SND_MIXER_CHN_MASK_FRONT_RIGHT)
			{
				wm8962->adc_mute |= SND_MIXER_CHN_MASK_FRONT_RIGHT;
				if (lrswap)
					codec_update(wm8962, ADCL_VOLUME, ADCL_VOL_MASK, 0x0);	/* Mute */
				else
					codec_update(wm8962, ADCR_VOLUME, ADCR_VOL_MASK, 0x0);	/* Mute */
			}
			else
			{
				wm8962->adc_mute &= ~SND_MIXER_CHN_MASK_FRONT_RIGHT;
				if (lrswap)
					codec_update(wm8962, ADCL_VOLUME, ADCL_VOL_MASK, wm8962->adc_vol[1]);
				else
					codec_update(wm8962, ADCR_VOLUME, ADCR_VOL_MASK, wm8962->adc_vol[1]);
			}
			codec_update(wm8962, ADCR_VOLUME, ADCR_VU, ADCR_VU);	/* Apply volume update */
		}
	}
	else
	{
		vol[0] = wm8962->adc_mute;
	}

	return (altered);
}

/*
 * Set ADV volume, used for MIC_IN.
 */
static int32_t
wm8962_adc_vol_control (MIXER_CONTEXT_T * wm8962,
    ado_mixer_delement_t * element, uint8_t set, uint32_t * vol,
    void *instance_data)
{
    int32_t altered = 0;

    if (set)
    {
        uint16_t lrswap = 0;

        /* Check if we are configured left and right channels swap */
        lrswap = codec_read(wm8962, AUDIO_INTERFACE_0);
        lrswap &= ADC_LRSWAP;

        // This offset is used since spk_range was shifted to start from 0
        vol[0] += 1;
        vol[1] += 1;

        altered = ((vol[0] != wm8962->adc_vol[0]) || (vol[1] != wm8962->adc_vol[1]));

        wm8962->adc_vol[0] = vol[0];
        wm8962->adc_vol[1] = vol[1];

        if (altered)
        {
            codec_update(wm8962, ADCR_VOLUME, ADCR_VU, 0);
            if (!(wm8962->adc_mute & SND_MIXER_CHN_MASK_FRONT_LEFT))
            {
                if (lrswap)
                    codec_update(wm8962, ADCR_VOLUME, ADCR_VOL_MASK, wm8962->adc_vol[0]);
                else
                    codec_update(wm8962, ADCL_VOLUME, ADCL_VOL_MASK, wm8962->adc_vol[0]);
            }
            if (!(wm8962->adc_mute & SND_MIXER_CHN_MASK_FRONT_RIGHT))
            {
                if (lrswap)
                    codec_update(wm8962, ADCL_VOLUME, ADCL_VOL_MASK, wm8962->adc_vol[1]);
                else
                    codec_update(wm8962, ADCR_VOLUME, ADCR_VOL_MASK, wm8962->adc_vol[1]);
            }
            codec_update(wm8962, ADCR_VOLUME, ADCR_VU, ADCR_VU);
        }
    }
    else
    {
        vol[0] = wm8962->adc_vol[0] - 1;
        vol[1] = wm8962->adc_vol[1] - 1;
    }

    return (altered);
}

/* Determine if the mic-bias is enabled. */
static int32_t
wm8962_micbias_get (MIXER_CONTEXT_T * wm8962,
    ado_dswitch_t * dswitch, snd_switch_t * cswitch,
    void *instance_data)
{
    uint32_t data;

    data = (codec_read (wm8962, PWR_MGMT_1) >> 1) & 1;
    cswitch->type = SND_SW_TYPE_BOOLEAN;
    cswitch->value.enable = data ? 1 : 0;

    return (0);
}

/* Set the micbias to a new value if that has been requested. */
static int32_t
wm8962_micbias_set (MIXER_CONTEXT_T * wm8962,
    ado_dswitch_t * dswitch, snd_switch_t * cswitch,
    void *instance_data)
{
    uint32_t data, bias;
    int32_t altered = 0;

    data = codec_read (wm8962, PWR_MGMT_1);
    bias = (data >> 1) & 1;
    altered = (cswitch->value.enable != (bias ? 1 : 0));
    if (altered)
        codec_update(wm8962, PWR_MGMT_1, MICBIAS_ENA, MICBIAS_SET(cswitch->value.enable));

    return (altered);
}

/* Wait for a write sequence to finish. */
int wm8962_waitfor(MIXER_CONTEXT_T * wm8962, uint32_t reg, uint32_t mask)
{
    int i;
    int val;

    for(i = 0; i < MAX_WAIT_TIME; i++)
    {
        val = codec_read(wm8962, reg);

        /* If the bits specified in the mask are no longer high. */
        if(!(val & mask))
        {
            ado_debug(DB_LVL_MIXER, "WM8962: waited %dms for write sequence.\n", i*WAIT_DELAY);
            return EOK;
        }
        delay(WAIT_DELAY);
    }
    return -1;
}

static int wm8962_init_hp(MIXER_CONTEXT_T * wm8962)
{
    /* Enable and Configure headphone Output Paths. */
    codec_write(wm8962, WRITE_SEQ_CONTROL_2, WSEQ_START | WSEQ_HP_POWER_UP);
    wm8962_waitfor(wm8962, WRITE_SEQ_CONTROL_3, WSEQ_BUSY );

    /* Enable HPMIX */
    codec_update(wm8962, MIXERENABLE, HPMIXL_ENA | HPMIXR_ENA, HPMIXL_ENA | HPMIXR_ENA);

    /* Init HPOUT Volume */
    wm8962->hp_vol[0] = wm8962->hp_vol[1] = HPOUT_VOL_0DB_ATT;
    codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VU, 0x0);
    codec_update(wm8962, HPOUTL_VOLUME, (HPOUTL_VOL_MASK | HPOUTL_ZC), (HPOUTL_ZC | HPOUTL_VOL(wm8962->hp_vol[0])));
    codec_update(wm8962, HPOUTR_VOLUME, (HPOUTR_VOL_MASK | HPOUTR_ZC), (HPOUTR_ZC | HPOUTR_VOL(wm8962->hp_vol[1])));
    codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VU, HPOUTL_VU);

    /* Route DAC through HPMIX */
    codec_write(wm8962, HEADPHONE_MIXER_1, HPMIXL_TO_HPOUTL_PGA_MASK | DACL_TO_HPMIXL);
    codec_write(wm8962, HEADPHONE_MIXER_2, HPMIXR_TO_HPOUTR_PGA_MASK | DACR_TO_HPMIXR);

    /* Un-Mute HPMIX Output */
    codec_update(wm8962, HEADPHONE_MIXER_3, HPMIXL_MUTE, 0x00 );
    codec_update(wm8962, HEADPHONE_MIXER_4, HPMIXR_MUTE, 0x00 );

    /* Un-Mute HPOUT */
    wm8962->hp_mute = 0x0;
    codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VU, 0);
    codec_update (wm8962, PWR_MGMT_2, (HPOUTR_PGA_MUTE | HPOUTL_PGA_MUTE), wm8962->hp_mute);
    codec_update(wm8962, HPOUTL_VOLUME, HPOUTL_VU, HPOUTL_VU);

    return (EOK);
}

static int wm8962_init_speaker(MIXER_CONTEXT_T * wm8962)
{
    /* Enable and Configure speaker Output Paths. */
    codec_write(wm8962,  WRITE_SEQ_CONTROL_2, WSEQ_START | WSEQ_SPK_WAKE_UP);
    wm8962_waitfor(wm8962, WRITE_SEQ_CONTROL_3, WSEQ_BUSY );

    //  VMID_SEL = 01, BIAS_ENA = 1,
    codec_update(wm8962, PWR_MGMT_1, VMID_SEL_MASK | BIAS_ENA, VMID_SEL_50K_DIV | BIAS_ENA);
    //  CLASSD_VOL = 111 (+12dB),
    codec_update(wm8962, CLASSDCTL_2, CLASSD_VOL_MASK, CLASSD_VOL(CLASSD_VOL_12DB));

    codec_update(wm8962, PWR_MGMT_2, DACL_ENA, DACL_ENA);
    codec_update(wm8962, PWR_MGMT_2, DACR_ENA, DACR_ENA);
    codec_update(wm8962, PWR_MGMT_2, SPKOUTL_PGA_ENA, SPKOUTL_PGA_ENA);
    codec_update(wm8962, PWR_MGMT_2, SPKOUTR_PGA_ENA, SPKOUTR_PGA_ENA);

    codec_update(wm8962, MIXERENABLE, SPKMIXL_ENA | SPKMIXR_ENA, SPKMIXL_ENA | SPKMIXR_ENA);

    codec_update(wm8962, SPAKER_MIXER_3, SPKMIXL_MUTE, 0x00 );
    codec_update(wm8962, SPAKER_MIXER_4, SPKMIXR_MUTE, 0x00 );
    codec_update(wm8962, SPAKER_MIXER_1, SPKMIXL_TO_SPKOUTL_PGA, 0);
    codec_update(wm8962, SPAKER_MIXER_2, SPKMIXR_TO_SPKOUTR_PGA, 0);

    /* Latch volume update bits */
    codec_update(wm8962, LDAC_VOLUME, (DACL_VOL_MASK | DACL_VU), (DAC_VOL_0DB | DACL_VU));
    codec_update(wm8962, RDAC_VOLUME, (DACR_VOL_MASK | DACR_VU), (DAC_VOL_0DB | DACR_VU));

    codec_update(wm8962, SPKOUTL_VOLUME, SPKOUTL_VU , 0);
    codec_update(wm8962, SPKOUTL_VOLUME, SPKOUTL_VOL_MASK, SPKOUT_VOL_0DB_ATT);
    codec_update(wm8962, SPKOUTR_VOLUME, SPKOUTR_VOL_MASK, SPKOUT_VOL_0DB_ATT);
    codec_update(wm8962, SPKOUTL_VOLUME, SPKOUTL_VU, SPKOUTL_VU);

    codec_update(wm8962, CLASSDCTL_1, SPKOUTR_PGA_MUTE | SPKOUTL_PGA_MUTE, 0x00);

    return (EOK);
}

/* Soft reset wm8962 codec */
static int
wm8962_reset (MIXER_CONTEXT_T * wm8962)
{
    /* Data written to reset registers isn't really important. */
    codec_write(wm8962, SOFTWARE_RESET, 0x6243);
    codec_write(wm8962, SW_RESET_PLL, 0x00);

    return (0);
}

/*
 * Configure the WM8962 internall PLL.
 */
static int
wm8962_setpll (MIXER_CONTEXT_T * wm8962)
{
    uint32_t fref = wm8962->mclk;
    uint32_t fout = SYSCLK;
    uint32_t pll_outdiv = 4;
    uint32_t pll_n, pll_k_int;
    float nk, fvco, pll_k;

    /* Calculate N.K */
    fvco = fout * pll_outdiv;

    /* Ensure Fvco is in the correct range. */
    if((fvco < FVCO_MINVAL) || (fvco > FVCO_MAXVAL))
    {
        pll_outdiv = 2;
        fvco = fout * pll_outdiv;

        if((fvco < FVCO_MINVAL) || (fvco > FVCO_MAXVAL))
        {
            ado_error ("WM8962: Fvco not in 90-100MHz range! Adjust PLL_OUTDIV or SYSCLK.");
            return EINVAL;
        }
    }

    nk = (fvco * 2) / fref;
    pll_n = (int) nk;
    pll_k = nk - pll_n;
    pll_k_int = pll_k * 16777216; /* Magic number from datasheet, convert K into integer format. */

    /* As we're using MCLK as the fref, we need to disable SEQ_ENA. */
    codec_update(wm8962, PLL_DLL, SEQ_ENA_MASK, 0x00);

    /* Configure integer portion. */
    codec_update(wm8962, PLL_13, (PLL3_FRAC_MASK | PLL3_N_MASK), (PLL3_FRAC | (pll_n << 0)));

    /* Configure fractional portion. */
    codec_update(wm8962, PLL_14, PLL3_K_23_16_MASK, ((pll_k_int >> 16) & 0xFF));
    codec_update(wm8962, PLL_15, PLL3_K_15_8_MASK, ((pll_k_int >> 8) & 0xFF));
    codec_update(wm8962, PLL_16, PLL3_K_7_0_MASK, ((pll_k_int) & 0xFF));

    /* Route MCLK to PLL3 */
    codec_update(wm8962, PLL_4, (PLL_CLK_SRC_MASK | FLL_TO_PLL3_MASK), (PLL_CLK_SRC_MCLK | FLL_TO_PLL3_MCLK));
    codec_update(wm8962, ANALOGUE_CLOCKING_4, (OSC_MCLK_SRC_MASK), OSC_MCLK_SRC_MCLK);

    /* Configure appropriate divisors 
     * Note: pll_outdiv can only be 2 or 4.
     */
    codec_update(wm8962, ANALOGUE_CLOCKING_2, (PLL3_OUTDIV_MASK | PLL_SYSCLK_DIV_MASK), (PLL3_OUTDIV(pll_outdiv) | PLL_SYSCLK_DIV_2));

    /* Enable the PLL now that we've configured everything. */
    codec_update(wm8962, PLL_2, (PLL3_ENA_MASK), PLL3_ENA);

    return (EOK);
}

static void wm8962_init_mic(MIXER_CONTEXT_T *wm8962)
{
    /* Note: The SabreSmart board has a mono mic conncted to INR3 */

    codec_update(wm8962, PWR_MGMT_1, (DMIC_ENA | INR_ENA | ADCL_ENA | ADCR_ENA | MICBIAS_ENA),
                (INR_ENA | ADCR_ENA | MICBIAS_ENA));

    /* Enable Input Path from IN3R to the Input PGA - Mono Mic connected to IN3R */
    codec_update(wm8962, RINPUT_PGA_CONTROL, (INPGAR_ENA | IN3R_TO_INPGAR | IN1R_TO_INPGAR | IN2R_TO_INPGAR),
            (INPGAR_ENA | IN3R_TO_INPGAR));

    /* Enable and un-mute the Right input Mixer */
    codec_update(wm8962, INPUT_MIXER_CTRL_1, (MIXINR_ENA | MIXINL_ENA | MIXINR_MUTE), MIXINR_ENA);

    /* Route Right input PGA to Right input Mixer */
    codec_write(wm8962, INPGAR_TO_MIXINR, INPGAR_TO_MIXINR);

    codec_update(wm8962, INPUT_MIXER_CTRL_2,
            (INPGAR_TO_MIXINR | IN3R_TO_MIXINR | IN2R_TO_MIXINR | INPGAL_TO_MIXINL | IN3L_TO_MIXINL | IN2L_TO_MIXINL),
            INPGAR_TO_MIXINR | IN3R_TO_MIXINR);

    /* Configure PGA->Mixer boost to 29dB */
    codec_update(wm8962, RIGHT_INPUT_MIX_VOL, INPGAR_MIXINR_VOL_MASK , INPGAR_MIXINR_VOL_29DB);

    /* Enable Mix Bias */
    codec_update(wm8962, PWR_MGMT_1, MICBIAS_ENA, MICBIAS_ENA);

    /* Configure volume of Right Input PGA - Un-mute, 0dB */
    codec_update(wm8962, RINPUT_VOLUME, INR_VU, 0x0);
    codec_update(wm8962, RINPUT_VOLUME, (INPGAR_MUTE | INR_VOL_MASK), INR_VOL(0x1f));
    codec_update(wm8962, RINPUT_VOLUME, INR_VU, INR_VU);

    /* Only the right ADC is connceted to an input so only configure its volume */
    codec_update(wm8962, ADCR_VOLUME, ADCR_VU, 0x0);
    wm8962->adc_vol[0] = wm8962->adc_vol[1] = ADC_VOL_0DB;
    codec_update(wm8962, ADCR_VOLUME, ADCR_VOL_MASK, ADCR_VOL(wm8962->adc_vol[1]));
    codec_update(wm8962, ADCR_VOLUME, ADCR_VU, ADCR_VU);

    /* Put the Right ADC data in the Left channel position on the I2S lines */
    codec_update(wm8962, AUDIO_INTERFACE_0, ADC_LRSWAP, ADC_LRSWAP);
}

/*
 * Power on parts of the codec required for normal operation.
 */
static int wm8962_init_subsystems(HW_CONTEXT_T * mx, MIXER_CONTEXT_T * wm8962)
{
    /* Configure GPIO for testing on TP6. */
    codec_update(wm8962, PWR_MGMT_1, OPCLK_ENA, 0x00);
    codec_update(wm8962, GPIO_2, GPIO2_FN_MASK, GPIO2_FN_OPCLK);
    codec_update(wm8962, PWR_MGMT_1, OPCLK_ENA, OPCLK_ENA);

    codec_update(wm8962, WRITE_SEQ_CONTROL_1, WSEQ_ENA, WSEQ_ENA);

    wm8962_init_hp(wm8962);
    wm8962_init_speaker(wm8962);
    wm8962->dac_mute = 0x0;
    codec_update(wm8962, ADCDACTL1, DAC_MUTE, wm8962->dac_mute);

    wm8962_init_mic(wm8962);

    /* Configure WM8962 as I2S master/slave. */
    if (mx->clk_mode & CLK_MODE_MASTER)
        codec_update(wm8962, AUDIO_INTERFACE_0, MSTR_MASK | FMT_MASK | WL_MASK,  SLAVE | FMT_I2S | WL_16);
    else
        codec_update(wm8962, AUDIO_INTERFACE_0, MSTR_MASK | FMT_MASK | WL_MASK,  MSTR | FMT_I2S | WL_16);

    return EOK;
}

static int wm8962_init_clocks(MIXER_CONTEXT_T * wm8962, int sample_rate, int sample_mode)
{
    /* Turn off sysclk so we can safely write to registers. */
    codec_update(wm8962, CLOCKING_2, (SYSCLK_ENA_MASK), (0x00));

    /* Ensure we can control all registers. */
    codec_update(wm8962, CLOCKING_2, (CLKREG_OVD_MASK), (CLKREG_OVD));

    /* Turn off the OSC_ENA bit set by CLKREG_OVD, and disable the PLLs. */
    codec_update(wm8962, PLL_2, (OSC_ENA_MASK | PLL2_ENA_MASK | PLL3_ENA_MASK), 0x00);

    /* Set up path to allow observation of SYSCLK at TP6. */
    codec_update(wm8962, ANALOGUE_CLOCKING_1, (CLKOUT2_SEL_MASK), (CLKOUT2_SEL_GPIO2));
    codec_update(wm8962, ANALOGUE_CLOCKING_3, (CLKOUT5_OE | CLKOUT2_OE), CLKOUT2_OE);

    /* Configure, then enable the PLL */
    wm8962_setpll(wm8962);

    /* Indicate which input clk to derive sysclk from. */
   codec_update(wm8962, CLOCKING_2, (MCLK_SRC_MASK), MCLK_SRC_PLL);

    /* Configure GPIO clock speed. */
    codec_update(wm8962, CLOCKING_3, (OPCLK_DIV_MASK), (OPCLK_DIV_1));

    /* Configure sample rate */
    codec_update(wm8962, ADDITIONAL_CTRL_3, (SAMPLE_MODE_MASK | SAMPLE_RATE_MASK), (sample_mode | sample_rate));

    /* Set LRCLK rate for 16 bit audio. */
    codec_update(wm8962, AUDIO_INTERFACE_2, AIF_RATE_MASK, AIF_RATE_32);

    /* Turn on sysclk so as we're done configuring. */
    codec_update(wm8962, CLOCKING_2, (SYSCLK_ENA_MASK | BCLK_DIV_MASK), (SYSCLK_ENA | BCLK_DIV_8));

    return EOK;
}

static int
wm8962_destroy (MIXER_CONTEXT_T * wm8962)
{
    /* Power down the codec if we're no longer using it. */
    codec_write(wm8962, WRITE_SEQ_CONTROL_2, WSEQ_START | WSEQ_POWER_DOWN);

    /* disconnect connect between SoC and codec */
    if (wm8962->i2c_fd != -1)
        close (wm8962->i2c_fd);

    ado_debug (DB_LVL_MIXER, "Destroying WM8962 mixer");
    ado_free (wm8962);

    return (0);
}

static int32_t
build_wm8962_mixer (MIXER_CONTEXT_T * wm8962, ado_mixer_t * mixer)
{
    /* Speaker Output elements */
    ado_mixer_delement_t *spk_mute = NULL, *spk_vol = NULL, *spk_io = NULL, *spk_pcm;
    ado_mixer_dgroup_t *spk_grp = NULL;

    /* headphone Output elements */
    ado_mixer_delement_t *hp_mute = NULL, *hp_vol = NULL, *hp_io = NULL, *hp_pcm;
    ado_mixer_dgroup_t *hp_grp = NULL;

    /* DAC Master Output elements */
    ado_mixer_delement_t *master_mute = NULL, *master_vol = NULL, *master_io = NULL, *master_pcm;
    ado_mixer_dgroup_t *master_grp = NULL;

    /* Input elements */
    ado_mixer_delement_t *adc_vol = NULL, *adc_mute = NULL, *adc_pcm = NULL, *adc_io = NULL;
    ado_mixer_dgroup_t *adc_grp = NULL;

    /* ################# */
    /*  PLAYBACK GROUPS  */
    /* ################# */
    /* WM8962: Line Out Output */
    if ((spk_pcm = ado_mixer_element_pcm1 (mixer, SND_MIXER_ELEMENT_PLAYBACK,
                SND_MIXER_ETYPE_PLAYBACK1, 1, &pcm_devices[0])) == NULL)
        return (-1);

    if ((spk_mute = ado_mixer_element_sw2 (mixer, "Line_Out Mute",
                (void *)wm8962_spk_mute_control, NULL, NULL)) == NULL)
        return (-1);

    if (ado_mixer_element_route_add (mixer, spk_pcm, spk_mute) != 0)
        return (-1);

    if ((spk_vol = ado_mixer_element_volume1 (mixer, "Line_Out Volume",
                2, spk_range, (void *)wm8962_spk_vol_control, NULL, NULL)) == NULL)
        return (-1);

    if (ado_mixer_element_route_add (mixer, spk_mute, spk_vol) != 0)
        return (-1);

    if ((spk_io = ado_mixer_element_io (mixer, "Line_Out", SND_MIXER_ETYPE_OUTPUT,
                0, 2, stereo_voices)) == NULL)
            return (-1);

    if (ado_mixer_element_route_add (mixer, spk_vol, spk_io) != 0)
        return (-1);

    if ((spk_grp = ado_mixer_playback_group_create (mixer, SND_MIXER_LINE_OUT,
                SND_MIXER_CHN_MASK_STEREO, spk_vol, spk_mute)) == NULL)
        return (-1);

    /* WM8962: Headphone Output */
    if ((hp_pcm = ado_mixer_element_pcm1 (mixer, SND_MIXER_ELEMENT_PLAYBACK,
                SND_MIXER_ETYPE_PLAYBACK1, 1, &pcm_devices[0])) == NULL)
        return (-1);

    if ((hp_mute = ado_mixer_element_sw1 (mixer, "HP Mute", 2,
                       (void *)wm8962_hp_mute_control, NULL, NULL)) == NULL)

        return (-1);

    if (ado_mixer_element_route_add (mixer, hp_pcm, hp_mute) != 0)
        return (-1);

    if ((hp_vol = ado_mixer_element_volume1 (mixer, "HP Volume", 2,
                      hp_range, (void *)wm8962_hp_vol_control, NULL, NULL)) == NULL)
        return (-1);

    if (ado_mixer_element_route_add (mixer, hp_mute, hp_vol) != 0)
        return (-1);

    if ((hp_io = ado_mixer_element_io (mixer, "HP OUT", SND_MIXER_ETYPE_OUTPUT, 0, 2, stereo_voices)) == NULL)
        return (-1);

    if (ado_mixer_element_route_add (mixer, hp_vol, hp_io) != 0)
        return (-1);

    if ((hp_grp = ado_mixer_playback_group_create (mixer, SND_MIXER_HEADPHONE_OUT,
                SND_MIXER_CHN_MASK_STEREO, hp_vol, hp_mute)) == NULL)
        return (-1);

    /* WM8962: DAC master Output */
    if ((master_pcm = ado_mixer_element_pcm1 (mixer, SND_MIXER_ELEMENT_PLAYBACK,
                SND_MIXER_ETYPE_PLAYBACK1, 1, &pcm_devices[0])) == NULL)
        return (-1);

    if ((master_mute = ado_mixer_element_sw2 (mixer, "Master Mute",
                (void *)wm8962_dac_mute_control, NULL, NULL)) == NULL)
        return (-1);

    if (ado_mixer_element_route_add (mixer, master_pcm, master_mute) != 0)
        return (-1);

    if ((master_vol = ado_mixer_element_volume1 (mixer, "Master Volume",
                2, master_range, (void *)wm8962_dac_vol_control, NULL, NULL)) == NULL)
        return (-1);

    if (ado_mixer_element_route_add (mixer, master_mute, master_vol) != 0)
        return (-1);

    if ((master_io = ado_mixer_element_io (mixer, "Master OUT", SND_MIXER_ETYPE_OUTPUT, 0, 2, stereo_voices)) == NULL)
        return (-1);

    if (ado_mixer_element_route_add (mixer, master_vol, master_io) != 0)
        return (-1);

    if ((master_grp = ado_mixer_playback_group_create (mixer, SND_MIXER_MASTER_OUT,
                SND_MIXER_CHN_MASK_STEREO, master_vol, master_mute)) == NULL)
        return (-1);

    /* ################ */
    /*  CAPTURE GROUPS  */
    /* ################ */
    /* WM8962: ADC Gain Control */
    if ((adc_io = ado_mixer_element_io (mixer, "MICIN", SND_MIXER_ETYPE_INPUT,
                0, 2, stereo_voices)) == NULL)
            return (-1);

    if ((adc_vol = ado_mixer_element_volume1 (mixer,
                "Mic Volume", 2, adc_range,
                (void *)wm8962_adc_vol_control, NULL, NULL)) == NULL)
        return (-1);

    if (ado_mixer_element_route_add (mixer, adc_io, adc_vol) != 0)
        return (-1);

    if ((adc_mute = ado_mixer_element_sw1 (mixer, "Mic Mute", 2,
                (void *)wm8962_adc_mute_control, NULL, NULL)) == NULL)
        return (-1);

    if (ado_mixer_element_route_add (mixer, adc_vol, adc_mute) != 0)
        return (-1);

    if ((adc_pcm = ado_mixer_element_pcm1 (mixer, SND_MIXER_ELEMENT_CAPTURE,
                SND_MIXER_ETYPE_CAPTURE1, 1, &pcm_devices[0])) == NULL)
        return (-1);

    if (ado_mixer_element_route_add (mixer, adc_mute, adc_pcm) != 0)
        return (-1);

    if ((adc_grp = ado_mixer_capture_group_create (mixer, SND_MIXER_GRP_IGAIN,
                SND_MIXER_CHN_MASK_STEREO, adc_vol, adc_mute, NULL, NULL)) == NULL)
        return (-1);

    /* ####################### */
    /* SWITCHES                */
    /* ####################### */
    if (ado_mixer_switch_new (mixer, "MIC BIAS", SND_SW_TYPE_BYTE,
            0, (void *)wm8962_micbias_get, (void *)wm8962_micbias_set, NULL, NULL) == NULL)
        return (-1);

    return (0);
}

static char *wm8962_opts[] = {
#define I2CDEV       0
    "i2cdev",
#define ADR0CS       1
    "adr0cs",
#define MCLK         2
    "mclk",
#define SAMPLERATE   3
    "samplerate",
#define INFO         4
    "info",     /* info must be last in the option list */
    NULL
};

static char *opt_help[] = {
    "=[#]                 : i2c device number, default 0 -> /dev/i2c0",
    "=[#]                 : adr0cs pin selection, 0->i2c addr 0xa, 1->i2c addr 0x2a, default 0",
    "=[#]                 : external SYS_MCLK, default 16500000",
    "=[#]                 : samplerate of codec, default 48000",
    "                     : display the mixer options"
};

/* Parse mixer options */
static int
wm8962_parse_options (MIXER_CONTEXT_T * wm8962, char * args)
{
    char * value = NULL;
    int opt;
    int idx;
    char *p;

    wm8962->i2c_num = 1;
    wm8962->adr0cs = 0;
    wm8962->mclk = MCLK_REF;
    wm8962->samplerate = 48000;

    while ((p = strchr(args, ':')) != NULL)
        *p = ',';

    while (*args != '\0')
    {
        opt = getsubopt (&args, wm8962_opts, &value);
        switch (opt) {
            case I2CDEV:
                if (value != NULL)
                    wm8962->i2c_num = strtoul (value, NULL, 0);
                break;
            case ADR0CS:
                if (value != NULL)
                    wm8962->adr0cs = strtoul (value, NULL, 0);
                break;
            case MCLK:
                if (value != NULL)
                    wm8962->mclk = strtoul (value, NULL, 0);
                break;
            case SAMPLERATE:
                if (value == NULL)
                    ado_error ("WM8962: Must pass a value to samplerate");
                else
                    wm8962->samplerate = strtoul (value, NULL, 0);
                break;
            case INFO:
                idx = 0;
                printf ("WM8962 mixer options\n");

                /* display all the mixer options before "info" */
                while (idx <= opt)
                {
                    printf("%10s%s\n", wm8962_opts[idx], opt_help[idx]);
                    idx++;
                }
                printf("\nExample: io-audio -d [driver] mixer=i2cdev=1:adr0cs=0:mclk=1650000\n");
                break;
            default:
                ado_error ("WM8962: Unrecognized option \"%s\"", value);
                break;
        }
    }

    return (EOK);
}

/* Called by audio i2s controller */
int
codec_mixer (ado_card_t * card, HW_CONTEXT_T * mx)
{
    wm8962_context_t *wm8962;
    int32_t status;
    int rate_set;
    int sample_mode = SAMPLE_INT_MODE;
    char i2c_dev[_POSIX_PATH_MAX];

    if ((wm8962 = (wm8962_context_t *)
        ado_calloc (1, sizeof (wm8962_context_t))) == NULL)
    {
        ado_error ("WM8962: no memory (%s)", strerror (errno));
        return (-1);
    }

    if ((status = ado_mixer_create (card, "wm8962", &mx->mixer, (void *)wm8962)) != EOK)
    {
        ado_error ("WM8962: Failed to create mixer", strerror (errno));
        ado_free (wm8962);
        return (status);
    }

    if (wm8962_parse_options (wm8962, mx->mixeropts)!=EOK)
    {
        ado_error ("WM8962: Failed to parse mixer options");
        ado_free (wm8962);
        return (-1);
    }

    /* check samplerate option */
    switch (wm8962->samplerate) {
        case 32000:
            rate_set = SAMPLE_RATE_32KHZ;
            break;
        case 44100:
            rate_set = SAMPLE_RATE_44KHZ;
            sample_mode = SAMPLE_FRAC_MODE;
            break;
        case 48000:
            rate_set = SAMPLE_RATE_48KHZ;
            break;
        case 96000:
            rate_set = SAMPLE_RATE_96KHZ;
            break;
        default:
            slogf (_SLOG_SETCODE (_SLOGC_AUDIO, 0), _SLOG_ERROR,
                "WM8962: Unsupported audio sample rate. This codec supports 32000, 44100, 48000, 96000 sample rates. Using the default (48000).");
            rate_set = SAMPLE_RATE_48KHZ;
            wm8962->samplerate = SAMPLE_RATE_48KHZ;
    }

    /* establish connection between SoC and wm8962 via I2C resource manager */
    sprintf(i2c_dev, "/dev/i2c%d", wm8962->i2c_num);
    wm8962->i2c_fd = open (i2c_dev, O_RDWR);
    if (wm8962->i2c_fd == -1)
    {
        ado_error ("WM8962 I2C_INIT: Failed to open I2C device %s", i2c_dev);
        ado_free (wm8962);
        return (-1);
    }

    if (build_wm8962_mixer (wm8962, mx->mixer))
    {
        ado_error ("WM8962: Failed to build mixer");
        close (wm8962->i2c_fd);
        ado_free (wm8962);
        return (-1);
    }

    wm8962_reset (wm8962);
    wm8962_init_clocks(wm8962, rate_set, sample_mode);
    wm8962_init_subsystems(mx, wm8962);

    ado_mixer_set_reset_func (mx->mixer, (void *)wm8962_reset);
    ado_mixer_set_destroy_func (mx->mixer, (void *)wm8962_destroy);

    return (0);
}

void
codec_set_default_group( ado_pcm_t *pcm, ado_mixer_t *mixer, int channel, int index )
{

    switch (channel)
    {
        case ADO_PCM_CHANNEL_PLAYBACK:
            ado_pcm_chn_mixer (pcm, ADO_PCM_CHANNEL_PLAYBACK, mixer,
                ado_mixer_find_element (mixer, SND_MIXER_ETYPE_PLAYBACK1,
                    SND_MIXER_ELEMENT_PLAYBACK, index), ado_mixer_find_group (mixer,
                    SND_MIXER_MASTER_OUT, index));
            break;
        case ADO_PCM_CHANNEL_CAPTURE:
            ado_pcm_chn_mixer (pcm, ADO_PCM_CHANNEL_CAPTURE, mixer,
                ado_mixer_find_element (mixer, SND_MIXER_ETYPE_CAPTURE1,
                    SND_MIXER_ELEMENT_CAPTURE, index), ado_mixer_find_group (mixer,
                    SND_MIXER_GRP_IGAIN, index));
            break;
        default:
            break;
    }
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/deva/ctrl/mx/nto/arm/dll.le.v7.mx6_wm8962/wm8962.c $ $Rev: 816773 $")
#endif

/*
 * $QNXLicenseC:
 * Copyright 2014 QNX Software Systems.
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

#ifndef __MIXER_H
#define __MIXER_H

/* Default clock values. */
#define MCLK_REF                16500000 /* This value assumes that mx6q_init_audmux_pins() in startup is configured for 16.5MHz */
#define FVCO_MAXVAL             100000000 /* Maximum value for Fvco: 100Mhz */
#define FVCO_MINVAL             90000000 /* Minimum value for Fvco: 90Mhz */
#define SYSCLK                  24576000 /* The value we want the PLL to produce for a given MCLK_REF. */

/* I2C 7-bit slave address */
#define WM8962_SLAVE_ADDR       (0x34 >> 1) /* Device ID from data sheet. */

/*
 * WM8962 CODEC Registers
 */
#define SOFTWARE_RESET          0x0F
#define SW_RESET_PLL            0x7F

#define LINPUT_VOLUME           0x00 // R0
#define RINPUT_VOLUME           0x01 // R1
#define HPOUTL_VOLUME           0x02 // R2
#define HPOUTR_VOLUME           0x03 // R3
#define ADCDACTL1               0x05 // R5
#define CLOCKING_1              0x04 // R4
#define AUDIO_INTERFACE_0       0x07 // R7
#define CLOCKING_2              0x08 // R8
#define LDAC_VOLUME             0x0A // R10
#define RDAC_VOLUME             0x0B // R11
#define AUDIO_INTERFACE_2       0x0E // R14
#define ADCL_VOLUME             0x15 // R21
#define ADCR_VOLUME             0x16 // R22
#define PWR_MGMT_1              0x19 // R25
#define PWR_MGMT_2              0x1A // R26
#define ADDITIONAL_CTRL_3       0x1B // R27
#define CLOCKING_3              0x1E // R30
#define INPUT_MIXER_CTRL_1      0x1F // R31
#define RIGHT_INPUT_MIX_VOL     0x21 // R33
#define INPUT_MIXER_CTRL_2      0x22 // R34
#define LINPUT_PGA_CONTROL      0x25 // R37
#define RINPUT_PGA_CONTROL      0x26 // R38
#define SPKOUTL_VOLUME          0x28 // R40
#define SPKOUTR_VOLUME          0x29 // R41
#define ADDITIONAL_CTL          0x30 // R48
#define CLASSDCTL_1             0x31 // R49
#define CLASSDCTL_2             0x33 // R51
#define ANALOG_HP_0             0x45 // R69
#define SPAKER_MIXER_1          0x69 // R105
#define SPAKER_MIXER_2          0x6A // R105
#define SPAKER_MIXER_3          0x6B // R107
#define SPAKER_MIXER_4          0x6C // R108
#define MIXERENABLE             0x63 // R99
#define WRITE_SEQ_CONTROL_1     0x57 // R87
#define WRITE_SEQ_CONTROL_2     0x5a // R90
#define WRITE_SEQ_CONTROL_3     0x5d // R93
#define HEADPHONE_MIXER_1       0x64 // R100
#define HEADPHONE_MIXER_2       0x65 // R101
#define HEADPHONE_MIXER_3       0x66 // R102
#define HEADPHONE_MIXER_4       0x67 // R103
#define ANALOGUE_CLOCKING_1     0x7C // R124
#define ANALOGUE_CLOCKING_2     0x7D // R125
#define ANALOGUE_CLOCKING_3     0x7E // R126
#define PLL_2                   0x81 // R129
#define PLL_4                   0x83 // R131
#define PLL_13                  0x8C // R140
#define PLL_14                  0x8D // R141
#define PLL_15                  0x8E // R142
#define PLL_16                  0x8F // R142
#define PLL_DLL                 0x96 // R150
#define ANALOGUE_CLOCKING_4     0x98 // R152 (also called "ANALOGUE CONTROL 4" in datasheet.)
#define GPIO_2                  0x201 // R513
#define SOUNDSTAGE_ENABLE       0x4005 //R16389

#define MAX_WAIT_TIME           500   /* Total sleep time is (WAIT_DELAY * MAX_WAIT_TIME) */
#define WAIT_DELAY              25    /* Time to wait in miliseconds. */

struct wm8962_context;

#define MIXER_CONTEXT_T struct wm8962_context

typedef struct wm8962_context
{
    ado_mixer_delement_t *mic;
    ado_mixer_delement_t *line;

    char                 i2c_num;   /* I2C bus number */
    int                  i2c_fd;    /* I2C device handle */
    int                  mclk;      /* external SYS_MCLK */
    int                  adr0cs;    /* wm8962 slave address select pin */
    int                  samplerate;
    int                  input_mux;
    uint16_t             dac_mute;
    uint16_t             adc_mute;
    uint16_t             adc_vol[2];
    uint16_t             hp_mute;
    uint16_t             hp_vol[2];
    uint16_t             spk_mute;
}
wm8962_context_t;

/*
 * R0 (0x00) - Left Input volume
 */
#define INL_VU                           (1<<8)              /* IN_VU */
#define INPGAL_MUTE                      (1<<7)              /* INPGAL_MUTE */
#define INL_ZC                           (1<<6)              /* INL_ZC */
#define INL_VOL_MASK                     0x003F              /* INL_VOL - [5:0] */
#define INL_VOL(x)                       (x & INL_VOL_MASK)  /* INL_VOL - [5:0] */

/*
 * R1 (0x01) - Right Input volume
 */
#define INR_VU                           INL_VU              /* IN_VU */
#define INPGAR_MUTE                      INPGAL_MUTE         /* INPGAR_MUTE */
#define INR_ZC                           INL_ZC              /* INR_ZC */
#define INR_VOL_MASK                     0x003F              /* INR_VOL - [5:0] */
#define INR_VOL(x)                       (x & INR_VOL_MASK)  /* INR_VOL - [5:0] */

/*
 * R2 (0x02) - HPOUTL volume
 */
#define HPOUTL_VU                 (1<<8)  /* HPOUT_VU */
#define HPOUTL_ZC                 (1<<7)  /* HPOUTL_ZC */
#define HPOUTL_VOL_MASK           0x007F  /* HPOUTL_VOL - [6:0] */
#define HPOUTL_VOL(x)             (x & HPOUTL_VOL_MASK)  /* HPOUTL_VOL - [6:0] */

/*
 * R3 (0x03) - HPOUTR volume
*/
#define HPOUTR_VU                 HPOUTL_VU  /* HPOUT_VU */
#define HPOUTR_ZC                 (1<<7)     /* HPOUTL_ZC */
#define HPOUTR_VOL_MASK           0x007F     /* HPOUTL_VOL - [6:0] */
#define HPOUTR_VOL(x)             (x & HPOUTR_VOL_MASK)  /* HPOUTL_VOL - [6:0] */

#define HPOUT_VOL_0DB_ATT         (0x79 << 0) /* 0dB volume */

/*
 * R5 (0x05) - ADC & DAC Control 1
 */
#define ADCR_DAT_INV              (1<<6)   /* ADCR_DAT_INV */
#define ADCL_DAT_INV              (1<<5)   /* ADCL_DAT_INV */
#define DAC_MUTE_RAMP             (1<<4)   /* DAC_MUTE_RAMP */
#define DAC_MUTE                  (1<<3)   /* DAC_MUTE */
#define DAC_DEEMP_MASK            (0x6<<1) /* DAC_DEEMP - [2:1] */
#define DAC_DEEMP(x)              ((x<<1) & DAC_DEEMP_MASK) /* DAC_DEEMP - [2:1] */
#define ADC_HPF_DIS               (1<<0)   /* ADC_HPF_DIS */

/*
 * R7 (0x07) Audio Interface 0
 */
#define ADC_LRSWAP              (0x1 << 8)
#define FMT_I2S                 (0x2 << 0)
#define MSTR                    (0x1 << 6)
#define SLAVE                   (0x0 << 6)
#define WL_16                   (0x00)

#define WL_MASK                 (0x3 << 2)
#define MSTR_MASK               (0x1 << 6)
#define FMT_MASK                (0x3 << 0)

/*
 * R8 (0x08)  Clocking 2
 */
#define CLKREG_OVD              (0x1 << 11)
#define MCLK_SRC_MCLK           (0x0 << 9)
#define MCLK_SRC_FLL            (0x1 << 9)
#define MCLK_SRC_PLL            (0x2 << 9)
#define SYSCLK_ENA              (0x1 << 5)

#define SYSCLK_ENA_MASK         (0x1 << 5)
#define MCLK_SRC_MASK           (0x3 << 9)
#define CLKREG_OVD_MASK         (0x1 << 11)

/*
 * R10 (0x0A) - Left DAC volume
 */
#define DACL_VU                 (1<<8)  /* DAC_VU */
#define DACL_VOL_MASK           0x00FF  /* DACL_VOL - [7:0] */
#define DACL_VOL(x)             ((x<<0) & DACL_VOL_MASK)  /* DACL_VOL - [7:0] */

/*
 * R11 (0x0B) - Right DAC volume
 */
#define DACR_VU                 DACL_VU              /* DAC_VU */
#define DACR_VOL_MASK           0x00FF               /* DACR_VOL - [7:0] */
#define DACR_VOL(x)             (x & DACR_VOL_MASK)  /* DACR_VOL - [7:0] */

#define DAC_VOL_0DB             (0xC0)

/*
 * R14 (0xE) - Audio Interface (2)
 */
#define AIF_RATE_32             (0x20)
#define AIF_RATE_MASK           (0x7FF<<0)

/*
 * R21 (0x15) - Left ADC volume
*/
#define ADCL_VU                 (1<<8)               /* ADC_VU */
#define ADCL_VOL_MASK           0x00FF               /* ADCL_VOL - [7:0] */
#define ADCL_VOL(x)             (x & ADCL_VOL_MASK)  /* ADCL_VOL - [7:0] */

/*
  * R22 (0x16) - Right ADC volume
  */
#define ADCR_VU                 ADCL_VU              /* ADC_VU */
#define ADCR_VOL_MASK           ADCL_VOL_MASK        /* ADCR_VOL - [7:0] */
#define ADCR_VOL(x)             (x & ADCR_VOL_MASK)  /* ADCR_VOL - [7:0] */

#define ADC_VOL_0DB             (0xC0)
#define ADC_VOL_23DB            (0xff)               /* 23.625dB */

/*
 * R25 (0x19) - Pwr Mgmt (1)
 */
#define MICBIAS_ENA             (0x1<<1)
#define MICBIAS_SET(x)          ((x)<<1)
#define ADCR_ENA                (0x1<<2)
#define ADCL_ENA                (0x1<<3)
#define INR_ENA                 (0x1<<4)
#define BIAS_ENA                (0x1<<6)
#define VMID_SEL_MASK           (0x3<<7)
#define VMID_SEL_50K_DIV        (0x1<<7)
#define OPCLK_ENA               (0x1<<9)
#define DMIC_ENA                (0x1<<10)

/*
 * R26 (0x1A) - Pwr Mgmt (2)
*/
#define DACL_ENA                (1<<8)  /* DACL_ENA */
#define DACR_ENA                (1<<7)  /* DACR_ENA */
#define HPOUTL_PGA_ENA          (1<<6)  /* HPOUTL_PGA_ENA */
#define HPOUTR_PGA_ENA          (1<<5)  /* HPOUTR_PGA_ENA */
#define SPKOUTL_PGA_ENA         (1<<4)  /* SPKOUTL_PGA_ENA */
#define SPKOUTR_PGA_ENA         (1<<3)  /* SPKOUTR_PGA_ENA */
#define HPOUTL_PGA_MUTE         (1<<1)  /* HPOUTL_PGA_MUTE */
#define HPOUTR_PGA_MUTE         (1<<0)  /* HPOUTR_PGA_MUTE */

/*
 * R27 (0x1B) - Additional Control (3)
 */
#define SAMPLE_MODE_MASK        (0x1<<4)
#define SAMPLE_INT_MODE         (1<<4)
#define SAMPLE_FRAC_MODE        (0<<4)
#define SAMPLE_RATE_MASK        (0x7<<0)
#define SAMPLE_RATE_32KHZ       (0x1) // Mode 1 (INT)
#define SAMPLE_RATE_44KHZ       (0x0) // Mode 0 (FRACTIONAL) (44.1Khz)
#define SAMPLE_RATE_48KHZ       (0x0) // Mode 1 (INT)
#define SAMPLE_RATE_96KHZ       (0x6) // Mode 1 (INT)

/*
 * R30 (0x1E) - Clocking 3
 */
#define OPCLK_DIV_MASK          (0x7<<10)
#define OPCLK_DIV_1             (0x0<<10)
#define BCLK_DIV_MASK           (0xF<<0)
#define BCLK_DIV_8              (0x7<<0)

/*
 * R31 (0x1F) - Input Mixer Control (1)
 */
#define MIXINR_ENA              (1<<0)
#define MIXINL_ENA              (1<<1)
#define MIXINR_MUTE             (1<<2)
#define MIXINL_MUTE             (1<<3)

/*
 * R33 (0x21) - Right Input Mixer Volume
 */
#define INPGAR_MIXINR_VOL_MASK  (0x7<<3)
#define INPGAR_MIXING_VOL(x)    ((x<<3) & INPGAR_MIXINR_VOL_MASK)
#define IN3R_MIXINR_VOL_MASK    (0x7<<0)
#define IN3R_MIXINR_VOL(x)      ((x<<0) & IN3R_MIXINR_VOL_MASK)

#define IN3R_MIXINR_VOL_0DB     (0x5<<0)
#define INPGAR_MIXINR_VOL_0DB   (0x0<<3)
#define INPGAR_MIXINR_VOL_29DB  (0x7<<3)

/*
 * R34 (0x22) - Input Mixer Control 2
 */
#define INPGAR_TO_MIXINR        (1<<0)
#define IN3R_TO_MIXINR          (1<<1)
#define IN2R_TO_MIXINR          (1<<2)
#define INPGAL_TO_MIXINL        (1<<3)
#define IN3L_TO_MIXINL          (1<<4)
#define IN2L_TO_MIXINL          (1<<5)

/*
 * R37 (0x25) - Left input PGA control
 */
#define INPGAL_ENA              (1<<4)  /* INPGAL_ENA */
#define IN1L_TO_INPGAL          (1<<3)  /* IN1L_TO_INPGAL */
#define IN2L_TO_INPGAL          (1<<2)  /* IN2L_TO_INPGAL */
#define IN3L_TO_INPGAL          (1<<1)  /* IN3L_TO_INPGAL */
#define IN4L_TO_INPGAL          (1<<0)  /* IN4L_TO_INPGAL */

/*
 * R38 (0x26) - Right input PGA control
 */
#define INPGAR_ENA              (1<<4)  /* INPGAR_ENA */
#define IN1R_TO_INPGAR          (1<<3)  /* IN1R_TO_INPGAR */
#define IN2R_TO_INPGAR          (1<<2)  /* IN2R_TO_INPGAR */
#define IN3R_TO_INPGAR          (1<<1)  /* IN3R_TO_INPGAR */
#define IN4R_TO_INPGAR          (1<<0)  /* IN4R_TO_INPGAR */

/*
 * R40 (0x28) - SPKOUTL volume
 */
#define SPKOUTL_VU              (1<<8)  /* SPKOUT_VU */
#define SPKOUTL_ZC              (1<<7)  /* SPKOUTR_ZC */
#define SPKOUTL_VOL_MASK        0x007F  /* SPKOUTR_VOL - [6:0] */
#define SPKOUTL_VOL(x)          (x & SPKOUTL_VOL_MASK) /* SPKOUTR_VOL - [6:0] */

/*
 * R41 (0x29) - SPKOUTR volume
 */
#define SPKOUTR_VU              SPKOUTL_VU  /* SPKOUT_VU */
#define SPKOUTR_ZC              (1<<7)      /* SPKOUTR_ZC */
#define SPKOUTR_VOL_MASK        0x007F      /* SPKOUTR_VOL - [6:0] */
#define SPKOUTR_VOL(x)          (x & SPKOUTR_VOL_MASK) /* SPKOUTR_VOL - [6:0] */

#define SPKOUT_VOL_0DB_ATT      (0x79 << 0) /* 0dB volume */

/*
 * R49 (0x31) - Class D Control 1
 */
#define SPKOUTR_ENA             (1<<7)  /* SPKOUTR_ENA */
#define SPKOUTL_ENA             (1<<6)  /* SPKOUTL_ENA */
#define SPKOUTL_PGA_MUTE        (1<<1)  /* SPKOUTL_PGA_MUTE */
#define SPKOUTR_PGA_MUTE        (1<<0)  /* SPKOUTR_PGA_MUTE */
/*
 * R51 (0x33) - Class D Control 2
 */
#define SPK_MONO                (1<<6)  /* SPK_MONO */
#define CLASSD_VOL_MASK         0x0007  /* CLASSD_VOL - [2:0] */
#define CLASSD_VOL(x)           (x & CLASSD_VOL_MASK)  /* CLASSD_VOL - [2:0] */

#define CLASSD_VOL_12DB         0x7

/*
 * R64 (0x45) - ANALOG_HP_0
 */
#define HP1L_RMV_SHRT           (0x1<<7)
#define HP1L_ENA_OUTP           (0x1<<6)
#define HP1L_ENA_DLY            (0x1<<5)
#define HP1L_ENA                (0x1<<4)
#define HP1R_RMV_SHRT           (0x1<<3)
#define HP1R_ENA_OUTP           (0x1<<2)
#define HP1R_ENA_DLY            (0x1<<1)
#define HP1R_ENA                (0x1<<0)

/*
 * R87 (0x57) - Write Sequencer Control (1)
 */
#define WSEQ_ENA                (1<<5)

/*
 * R88 (0x58) - Write Sequencer Control (2)
 */
#define WSEQ_ABORT              (1<<8)
#define WSEQ_START              (1<<7)

#define WSEQ_HP_POWER_UP        (0x00)
#define WSEQ_INPUT_POWER_UP     (0x12)
#define WSEQ_POWER_DOWN         (0x1B)
#define WSEQ_SPK_WAKE_UP        (0x68)

/*
 * R89 (0x59) - Write Sequencer Control (3)
 */
#define WSEQ_BUSY               (1<<0)

/*
 * R99 (0x63) - Mixer Enables
 */
#define HPMIXL_ENA                       (1<<3)  /* HPMIXL_ENA */
#define HPMIXR_ENA                       (1<<2)  /* HPMIXR_ENA */
#define SPKMIXL_ENA                      (1<<1)  /* SPKMIXL_ENA */
#define SPKMIXR_ENA                      (1<<0)  /* SPKMIXR_ENA */

/*
 * R100 (0x64) - Headphone Mixer (1)
 */
#define HPMIXL_TO_HPOUTL_PGA_MASK        (1<<7)
#define DACL_TO_HPMIXL                   (1<<5)

/*
 * R101 (0x65) - Headphone Mixer (2)
 */
#define HPMIXR_TO_HPOUTR_PGA_MASK        (1<<7)
#define DACR_TO_HPMIXR                   (1<<4)

/*
 * R102 (0x65) - Headphone Mixer (3)
 */
#define HPMIXL_MUTE                      (1<<8)

/*
 * R103 (0x67) - Headphone Mixer (4)
 */
#define HPMIXR_MUTE                      (1<<8)

/*
 * R105 (0x69) - Speaker Mixer (1)
 */
#define SPKMIXL_TO_SPKOUTL_PGA           (1<<7)  /* SPKMIXL_TO_SPKOUTL_PGA */
#define DACL_TO_SPKMIXL                  (1<<5)  /* DACL_TO_SPKMIXL */
#define DACR_TO_SPKMIXL                  (1<<4)  /* DACR_TO_SPKMIXL */
#define MIXINL_TO_SPKMIXL                (1<<3)  /* MIXINL_TO_SPKMIXL */
#define MIXINR_TO_SPKMIXL                (1<<2)  /* MIXINR_TO_SPKMIXL */
#define IN4L_TO_SPKMIXL                  (1<<1)  /* IN4L_TO_SPKMIXL */
#define IN4R_TO_SPKMIXL                  (1<<0)  /* IN4R_TO_SPKMIXL */

/*
 * R106 (0x6A) - Speaker Mixer (2)
 */
#define SPKMIXR_TO_SPKOUTR_PGA           (1<<7)  /* SPKMIXR_TO_SPKOUTR_PGA */
#define DACL_TO_SPKMIXR                  (1<<5)  /* DACL_TO_SPKMIXR */
#define DACR_TO_SPKMIXR                  (1<<4)  /* DACR_TO_SPKMIXR */
#define MIXINL_TO_SPKMIXR                (1<<3)  /* MIXINL_TO_SPKMIXR */
#define MIXINR_TO_SPKMIXR                (1<<2)  /* MIXINR_TO_SPKMIXR */
#define IN4L_TO_SPKMIXR                  (1<<1)  /* IN4L_TO_SPKMIXR */
#define IN4R_TO_SPKMIXR                  (1<<0)  /* IN4R_TO_SPKMIXR */

/*
 * R107 (0x6B) - Speaker Mixer (3)
 */
#define SPKMIXL_MUTE                     (1<<8)  /* SPKMIXL_MUTE */
#define MIXINL_SPKMIXL_VOL               (1<<7)  /* MIXINL_SPKMIXL_VOL */
#define MIXINR_SPKMIXL_VOL               (1<<6)  /* MIXINR_SPKMIXL_VOL */
#define IN4L_SPKMIXL_VOL_MASK            0x0038  /* IN4L_SPKMIXL_VOL - [5:3] */
#define IN4L_SPKMIXL_VOL(x)              ((x<<3) & IN4L_SPKMIXL_VOL_MASK)  /* IN4L_SPKMIXL_VOL - [5:3] */
#define IN4R_SPKMIXL_VOL_MASK            0x0007  /* IN4R_SPKMIXL_VOL - [2:0] */
#define IN4R_SPKMIXL_VOL(x)              (x & IN4R_SPKMIXL_VOL_MASK)  /* IN4R_SPKMIXL_VOL - [2:0] */

/*
 * R108 (0x6C) - Speaker Mixer (4)
 */
#define SPKMIXR_MUTE                     (1<<8)  /* SPKMIXR_MUTE */
#define MIXINL_SPKMIXR_VOL_MASK          0x0080  /* MIXINL_SPKMIXR_VOL */
#define MIXINL_SPKMIXR_VOL(x)            ((x<<7) & MIXINL_SPKMIXR_VOL_MASK)  /* MIXINL_SPKMIXR_VOL */
#define MIXINR_SPKMIXR_VOL_MASK          0x0040  /* MIXINR_SPKMIXR_VOL */
#define MIXINR_SPKMIXR_VOL(x)            ((x<<6) & MIXINR_SPKMIXR_VOL_MASK)  /* MIXINR_SPKMIXR_VOL */
#define IN4L_SPKMIXR_VOL_MASK            0x0038  /* IN4L_SPKMIXR_VOL - [5:3] */
#define IN4L_SPKMIXR_VOL(x)              ((x<<3) & IN4L_SPKMIXR_VOL_MASK)  /* IN4L_SPKMIXR_VOL - [5:3] */
#define IN4R_SPKMIXR_VOL_MASK            0x0007  /* IN4R_SPKMIXR_VOL - [2:0] */
#define IN4R_SPKMIXR_VOL(x)              ((x<<0) & IN4R_SPKMIXR_VOL_MASK)  /* IN4R_SPKMIXR_VOL - [2:0] */

/*
 * R109 (0x6D) - Speaker Mixer (5)
 */
#define DACL_SPKMIXL_VOL_MASK            0x0080  /* DACL_SPKMIXL_VOL */
#define DACL_SPKMIXL_VOL(x)              ((x<<7) & DACL_SPKMIXL_VOL_MASK)  /* DACL_SPKMIXL_VOL */
#define DACR_SPKMIXL_VOL_MASK            0x0040  /* DACR_SPKMIXL_VOL */
#define DACR_SPKMIXL_VOL(x)              ((x<<6) & DACR_SPKMIXL_VOL_MASK)  /* DACR_SPKMIXL_VOL */
#define DACL_SPKMIXR_VOL_MASK            0x0020  /* DACL_SPKMIXR_VOL */
#define DACL_SPKMIXR_VOL(x)              ((x<<5) & DACL_SPKMIXR_VOL_MASK)  /* DACL_SPKMIXR_VOL */
#define DACR_SPKMIXR_VOL_MASK            0x0010  /* DACR_SPKMIXR_VOL */
#define DACR_SPKMIXR_VOL(x)              ((x<<4) & DACR_SPKMIXR_VOL_MASK)  /* DACR_SPKMIXR_VOL */

/*
 * R124 - (0x7C) - Analogue Clocking (1)
 */
#define CLKOUT2_SEL_MASK        (0x3<<5)
#define CLKOUT2_SEL_GPIO2       (0x1<<5)

/*
 * R125 - (0x7D) - Analogue Clocking (2)
 */
#define PLL_SYSCLK_DIV_MASK     (0x3<<3)
#define PLL_SYSCLK_DIV_1        (0x0<<3)
#define PLL_SYSCLK_DIV_2        (0x1<<3)
#define PLL_SYSCLK_DIV_4        (0x2<<3)
#define PLL3_OUTDIV_MASK        (0x1<<6)
#define PLL3_OUTDIV(x)          (((x)>>2)<<6) /* x Should only ever be 2 or 4. */

/*
 * R126 - (0x7E) - Analogue Clocking (3)
 */
#define CLKOUT5_OE              (1<<0)
#define CLKOUT2_OE              (1<<3)

/* PLL Registers */
#define PLL3_ENA                (0x1<<4)

#define PLL_CLK_SRC_MCLK        (0x1<<1)
#define FLL_TO_PLL3_MCLK        (0x0<<0)

#define PLL3_FRAC               (0x1<<6)

#define OSC_MCLK_SRC_MCLK       (0x00)
#define OSC_MCLK_SRC_OSC        (0x01<<4)

#define SEQ_ENA_MASK            (0x1<<1)

#define OSC_ENA_MASK            (0x1<<7)
#define PLL2_ENA_MASK           (0x1<<5)
#define PLL3_ENA_MASK           (0x1<<4)

#define PLL_CLK_SRC_MASK        (0x1<<1)
#define FLL_TO_PLL3_MASK        (0x1<<0)

#define PLL3_FRAC_MASK          (0x1<<6)
#define PLL3_N_MASK             (0x1F<<0)

#define PLL3_K_23_16_MASK       (0xFF<<0)
#define PLL3_K_15_8_MASK        (0xFF<<0)
#define PLL3_K_7_0_MASK         (0xFF<<0)

#define OSC_MCLK_SRC_MASK       (0x1<<4)

/*
 * R513 (0x201) - GPIO (2)
 */
#define GPIO2_FN_MASK           (0x1F<<0)
#define GPIO2_FN_OPCLK          (0x12<<0)

#endif /* __MIXER_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/deva/ctrl/mx/nto/arm/dll.le.v7.mx6_wm8962/wm8962.h $ $Rev: 793675 $")
#endif

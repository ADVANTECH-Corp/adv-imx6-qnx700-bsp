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

#include <wfdqnx/wfdcfg.h>
#include <wfdqnx/wfdcfg_imx6x.h>
#include <stddef.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>  // NULL
#include <string.h>

struct wfdcfg_device {
	const struct wfdcfg_keyval *ext_list;
};

struct wfdcfg_port {
	int id;
	const struct wfdcfg_keyval *ext_list;
};

struct wfdcfg_mode_list {
	const struct mode *first_mode;
};

// Internal structure to keep a mode and its associated extension(s).
struct mode {
	const struct wfdcfg_timing timing;
	const struct wfdcfg_keyval *ext;
};


/* Helper functions */

static const struct mode*
cast_timing_to_mode(const struct wfdcfg_timing *timing)
{
	char *p = (char*)timing;
	if (p) {
		p -= offsetof(struct mode, timing);
	}
	return (const struct mode*)p;
}

static const struct wfdcfg_keyval*
get_ext_from_list(const struct wfdcfg_keyval *ext_list, const char *key)
{
	while (ext_list) {
		if (!ext_list->key) {
			ext_list = NULL;
			break;
		} else if (strcmp(ext_list->key, key) == 0) {
			return ext_list;
		}
		++ext_list;
	}
	return NULL;
}


static const struct wfdcfg_keyval device_ext[] = {
	{ NULL }  // marks end of list
};

static const struct wfdcfg_keyval port_ext[] = {
	{ NULL }  // marks end of list
};

#define MODELINE(mhz, h1, h2, h3, h4, v1, v2, v3, v4, f) \
	{ .pixel_clock_kHz = (mhz) * 1000, \
	.hpixels = (h1), .hfp = (h2) - (h1), .hsw = (h3) - (h2), .hbp = (h4) - (h3), \
	.vlines  = (v1), .vfp = (v2) - (v1), .vsw = (v3) - (v2), .vbp = (v4) - (v3), \
	.flags = (f), } \

static const struct mode modes[] = {
	/* A combination of the 800x600 and 640x480 modelines */
	{
		.timing = MODELINE(40.0, 800, 840, 968, 1056, 480, 481, 484, 500, 0),
	},

	/* These are pulled from the EDID of a nearby monitor */

	/* 1920x1200 (0xb4)  154.0MHz +HSync -VSync +preferred
		h: width  1920 start 1968 end 2000 total 2080 skew    0 clock   74.0KHz
		v: height 1200 start 1203 end 1209 total 1235           clock   60.0Hz */
	{
		.timing = MODELINE(154.0, 1920, 1968, 2000, 2080, 1200, 1203, 1209, 1235, WFDCFG_INVERT_VSYNC),
	},

	/* 1920x1080 (0xb6)  148.5MHz +HSync +VSync +preferred
		h: width  1920 start 2008 end 2052 total 2200 skew    0 clock   67.5KHz
		v: height 1080 start 1084 end 1089 total 1125           clock   60.0Hz */
	{
		.timing = MODELINE(148.5, 1920, 2008, 2052, 2200, 1080, 1084, 1089, 1125, 0),
	},

	/* 1920x720 @ 60Hz 97.34MHz +HSync +VSync
		h: width  1920 start 1980 end 2000 total 2080 skew    0
		v: height  720 start  722 end  723 total  780           */
	{
		.timing = MODELINE(97.34, 1920, 1980, 2000, 2080, 720, 722, 723, 780, 0),
	},

	/* 1280x1024 (0xb7)  135.0MHz +HSync +VSync
		h: width  1280 start 1296 end 1440 total 1688 skew    0 clock   80.0KHz
		v: height 1024 start 1025 end 1028 total 1066           clock   75.0Hz */
	{
		.timing = MODELINE(135.0, 1280, 1296, 1440, 1688, 1024, 1025, 1028, 1066, 0),
	},

	/* 1280x1024 (0xb8)  108.0MHz +HSync +VSync
		h: width  1280 start 1328 end 1440 total 1688 skew    0 clock   64.0KHz
		v: height 1024 start 1025 end 1028 total 1066           clock   60.0Hz */
	{
		.timing = MODELINE(135.0, 1280, 1328, 1440, 1688, 1024, 1025, 1028, 1066, 0),
	},

	/* 1152x864 (0xb9)  108.0MHz +HSync +VSync
		h: width  1152 start 1216 end 1344 total 1600 skew    0 clock   67.5KHz
		v: height  864 start  865 end  868 total  900           clock   75.0Hz */
	{
		.timing = MODELINE(108.0, 1152, 1216, 1344, 1600, 864, 865, 868, 900, 0),
	},

	/* 1024x768 (0xba)   78.8MHz +HSync +VSync
		h: width  1024 start 1040 end 1136 total 1312 skew    0 clock   60.1KHz
		v: height  768 start  769 end  772 total  800           clock   75.1Hz */
	{
		.timing = MODELINE(78.8, 1024, 1040, 1136, 1312, 768, 769, 772, 800, 0),
	},

	/* 1024x768 (0xbb)   65.0MHz -HSync -VSync
		h: width  1024 start 1048 end 1184 total 1344 skew    0 clock   48.4KHz
		v: height  768 start  771 end  777 total  806           clock   60.0Hz */
	{
		.timing = MODELINE(65.0, 1024, 1048, 1184, 1344, 768, 771, 777, 806, 0),
	},

	/* 800x600 (0xbc)   49.5MHz +HSync +VSync
		h: width   800 start  816 end  896 total 1056 skew    0 clock   46.9KHz
		v: height  600 start  601 end  604 total  625           clock   75.0Hz */
	{
		.timing = MODELINE(49.5, 800, 816, 896, 1056, 600, 601, 604, 625, 0),
	},

	/* 800x600 (0x45)   40.0MHz +HSync +VSync
		h: width   800 start  840 end  968 total 1056 skew    0 clock   37.9KHz
		v: height  600 start  601 end  605 total  628           clock   60.3Hz */
	{
		.timing = MODELINE(40.0, 800, 840, 968, 1056, 600, 601, 605, 628, 0),
	},

	/* 640x480 (0xbd)   31.5MHz -HSync -VSync
		h: width   640 start  656 end  720 total  840 skew    0 clock   37.5KHz
		v: height  480 start  481 end  484 total  500           clock   75.0Hz */
	{
		.timing = MODELINE(31.5, 640, 656, 720, 840, 480, 481, 484, 500, WFDCFG_INVERT_VSYNC | WFDCFG_INVERT_HSYNC),
	},

	/* 640x480 (0xbe)   25.2MHz -HSync -VSync
		h: width   640 start  656 end  752 total  800 skew    0 clock   31.5KHz
		v: height  480 start  490 end  492 total  525           clock   60.0Hz */
	{
		.timing = MODELINE(25.2, 640, 656, 752, 800, 480, 490, 492, 525, WFDCFG_INVERT_VSYNC | WFDCFG_INVERT_HSYNC),
	},

	/* 720x400 (0xbf)   28.3MHz -HSync +VSync
		h: width   720 start  738 end  846 total  900 skew    0 clock   31.5KHz
		v: height  400 start  412 end  414 total  449           clock   70.1Hz */
	{
		.timing = MODELINE(28.3, 720, 738, 846, 900, 400, 412, 414, 449, WFDCFG_INVERT_HSYNC),
	},

	// 1280x800 @ 50Hz  -Hsync -Vsync
	{
		.timing = MODELINE(68.00, 1280, 1336, 1464, 1648, 800, 803, 809, 826, WFDCFG_INVERT_VSYNC | WFDCFG_INVERT_HSYNC),
	},

	// 1280x480 @ 60 Hz  -Hsync -Vsync
	{
		.timing = MODELINE(48.0, 1280, 1320, 1440, 1600, 480, 483, 493, 500, WFDCFG_INVERT_VSYNC | WFDCFG_INVERT_HSYNC),
	},

	// 960x540 @ 30.00 Hz (CVT)  -Hsync +Vsync
	{
		.timing = {
			.pixel_clock_kHz = 19750,
			.hpixels =  960, .hfp= 24, .hsw= 96, .hbp=120,  // 1200 total
			.vlines  =  540, .vfp=  3, .vsw=  5, .vbp=  6,  //  554 total
			.flags = WFDCFG_INVERT_VSYNC,
		},
	},

	// 960x540 @ 60.00 Hz (CVT)  -Hsync +Vsync
	{
		.timing = {
			.pixel_clock_kHz = 40750,
			.hpixels =  960, .hfp= 32, .hsw= 96, .hbp=128,  // 1216 total
			.vlines  =  540, .vfp=  3, .vsw=  5, .vbp= 14,  //  562 total
			.flags = WFDCFG_INVERT_VSYNC,
		},
	},

	// 800x480 @ 30.00 Hz (CVT)  -Hsync +Vsync
	{
		.timing = {
			.pixel_clock_kHz = 14750,
			.hpixels =  800, .hfp= 24, .hsw= 72, .hbp= 96,  //  992 total
			.vlines  =  480, .vfp=  3, .vsw=  7, .vbp=  6,  //  496 total
			.flags = WFDCFG_INVERT_VSYNC,
		},
	},

	// 800x480 @ 60.00 Hz (CVT)  -Hsync +Vsync
	{
		.timing = {
			.pixel_clock_kHz =  29760,
			.hpixels =  800, .hfp= 24, .hsw= 72, .hbp= 96,  //  992 total
			.vlines  =  480, .vfp=  3, .vsw=  7, .vbp= 10,  //  500 total
			.flags = WFDCFG_INVERT_VSYNC,
		},
	},
	// 1280x720 @ 60 Hz
	{
		.timing = {
			.pixel_clock_kHz =  74250,
			.hpixels = 1280, .hfp=110, .hsw= 40, .hbp=220,  // 1650 total
			.vlines  =  720, .vfp=  5, .vsw=  5, .vbp= 20,  //  750 total
			.flags = 0,
		},
	},
	{
		// marks end of list
		.timing = {.pixel_clock_kHz = 0},
	},
};


int
wfdcfg_device_create(struct wfdcfg_device **device, int deviceid,
		const struct wfdcfg_keyval *opts)
{
	int err = EOK;
	struct wfdcfg_device *tmp_dev = NULL;
	(void)opts;

	switch(deviceid) {
		case 1:
			tmp_dev = malloc(sizeof(*tmp_dev));
			if (!tmp_dev) {
				err = ENOMEM;
				goto end;
			}

			tmp_dev->ext_list = device_ext;
			break;

		default:
			/* Invalid device id*/
			err = ENOENT;
			goto end;
	}

end:
	if (err) {
		free(tmp_dev);
	} else {
		*device = tmp_dev;
	}
	return err;
}

const struct wfdcfg_keyval*
wfdcfg_device_get_extension(const struct wfdcfg_device *device,
		const char *key)
{
	return get_ext_from_list(device->ext_list, key);
}

void
wfdcfg_device_destroy(struct wfdcfg_device *device)
{
	free(device);
}

int
wfdcfg_port_create(struct wfdcfg_port **port,
		const struct wfdcfg_device *device, int portid,
		const struct wfdcfg_keyval *opts)
{
	int err = EOK;
	struct wfdcfg_port *tmp_port = NULL;
	(void)opts;

	assert(device);

	switch(portid) {
		// Ignore the port ID as all ports are equal
		default:
			tmp_port = malloc(sizeof(*tmp_port));
			if (!tmp_port) {
				err = ENOMEM;
				goto end;
			}
			tmp_port->id = portid;
			tmp_port->ext_list = port_ext;
			break;
		case 0:  // invalid
			err = ENOENT;
			goto end;
	}

end:
	if (err) {
		free(tmp_port);
	} else {
		*port = tmp_port;
	}
	return err;
}

const struct wfdcfg_keyval*
wfdcfg_port_get_extension(const struct wfdcfg_port *port, const char *key)
{
	return get_ext_from_list(port->ext_list, key);
}

void
wfdcfg_port_destroy(struct wfdcfg_port *port)
{
	free(port);
}

int
wfdcfg_mode_list_create(struct wfdcfg_mode_list **list,
		const struct wfdcfg_port* port, const struct wfdcfg_keyval *opts)
{
	int err = 0;
	struct wfdcfg_mode_list *tmp_mode_list = NULL;

	(void)opts;
	assert(port); (void)port;

	tmp_mode_list = malloc(sizeof *tmp_mode_list);
	if (!tmp_mode_list) {
		err = ENOMEM;
		goto out;
	}
	tmp_mode_list->first_mode = &modes[0];
out:
	if (err) {
		free(tmp_mode_list);
	} else {
		*list = tmp_mode_list;
	}
	return err;
}

const struct wfdcfg_keyval*
wfdcfg_mode_list_get_extension(const struct wfdcfg_mode_list *list,
		const char *key)
{
	(void)list;
	(void)key;
	return NULL;
}

void
wfdcfg_mode_list_destroy(struct wfdcfg_mode_list *list)
{
	free(list);
}

const struct wfdcfg_timing*
wfdcfg_mode_list_get_next(const struct wfdcfg_mode_list *list,
		const struct wfdcfg_timing *prev_timing)
{
	assert(list);

	const struct mode *m = list->first_mode;
	if (prev_timing) {
		m = cast_timing_to_mode(prev_timing) + 1;
	}

	if (m->timing.pixel_clock_kHz == 0) {
		// end of list (this is not an error)
		m = NULL;
	}
	return &m->timing;
}

const struct wfdcfg_keyval*
wfdcfg_mode_get_extension(const struct wfdcfg_timing *timing,
		const char *key)
{
	const struct wfdcfg_keyval *ext = cast_timing_to_mode(timing)->ext;
	return get_ext_from_list(ext, key);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/wfd/imx6x/wfdcfg/imx6x-hdmi/hdmi.c $ $Rev: 778554 $")
#endif

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

//enum {
//	TIMING_EXTENSIONS = UINT32_C(1) << 30,   // 'ext' pointer is valid
//};


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

static const struct wfdcfg_keyval video_extensions[] = {
	{
#if 0
		.key = TIMING_LDB_OPTIONS,
		.i = TIMING_LDB_DATA_MAPPING_SPWG_18,
#else
		.key = TIMING_OUTPUT_FORMAT,
		.i = TIMING_OUTPUT_FORMAT_RGB888,
#endif
	},
	{ .key = NULL },
};

static const struct mode modes[] = {
	{
		.timing = {
#if 0
		/* These are from LTIB default settings */
		.pixel_clock_kHz = 33898,
		.hpixels = 640, .hbp = 96, .hsw = 72, .hfp = 24,
		.vlines  = 480,  .vbp = 3, .vsw =  7, .vfp = 10,
#else
		/* (10^12)/25200000 = 39682.5 */
		.pixel_clock_kHz = 39683,
		.hpixels = 640, .hbp = 142, .hsw = 2, .hfp = 16,
		.vlines  = 480, .vbp = 33, .vsw = 2, .vfp = 10,
#endif
		//.flags = TIMING_EXTENSIONS | WFDCFG_INVERT_CLOCK,
		},
		.ext = video_extensions,
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
__SRCVERSION("$URL: http://svn/product/branches/6.6.0/trunk/hardware/wfd/imx6x/wfdcfg/imx6x-hdmi/hdmi.c $ $Rev: 763554 $")
#endif

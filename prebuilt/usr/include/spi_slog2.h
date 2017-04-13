
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




#ifndef SPI_LOGGING_H
#define SPI_LOGGING_H

#include <dlfcn.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <sys/slog2.h>

#define LOGGING_NAME "spi"

int _spi_get_logging_level();
void _spi_set_logging_level(int level);
int _spi_init_logging(const char * name);


extern int (*_lib_slog2f)( slog2_buffer_t buffer, uint16_t code, uint8_t severity, const char* format, ... );
extern int (*_lib_slog2_register)( slog2_buffer_set_config_t *config, slog2_buffer_t *handles, uint32_t flags );
extern int (*_lib_slog2_get_verbosity)( slog2_buffer_t buffer );
extern int (*_lib_slog2_set_verbosity)( slog2_buffer_t buffer, uint8_t verbosity );

extern slog2_buffer_set_config_t _buffer_config;
extern slog2_buffer_t _spi_buffer_handle;


#define spi_debug(msg, ...) \
do { \
	if (_spi_buffer_handle) \
		_lib_slog2f(_spi_buffer_handle, 0, SLOG2_DEBUG1, msg, ## __VA_ARGS__); \
	else                    \
		fprintf(stderr,msg , ## __VA_ARGS__); \
} while (0)

#define spi_debug2(msg, ...) \
do { \
	if (_spi_buffer_handle) \
		_lib_slog2f(_spi_buffer_handle, 0, SLOG2_DEBUG2, msg, ## __VA_ARGS__); \
	else                    \
		fprintf(stderr,msg , ## __VA_ARGS__); \
} while (0)

#define spi_error(msg, ...) \
do { \
	if (_spi_buffer_handle) \
		_lib_slog2f(_spi_buffer_handle, 0, SLOG2_ERROR, msg, ## __VA_ARGS__); \
	else                    \
		fprintf(stderr,msg , ## __VA_ARGS__); \
} while (0)

#define spi_info(msg, ...) \
do { \
	if (_spi_buffer_handle) \
		_lib_slog2f(_spi_buffer_handle, 0, SLOG2_INFO, msg, ## __VA_ARGS__); \
	else                    \
		fprintf(stderr,msg , ## __VA_ARGS__); \
} while (0)

#define spi_warn(msg, ...) \
do { \
	if (_spi_buffer_handle) \
		_lib_slog2f(_spi_buffer_handle, 0, SLOG2_WARNING, msg, ## __VA_ARGS__); \
	else                    \
		fprintf(stderr,msg , ## __VA_ARGS__); \
} while (0)

#define spi_critical(msg, ...) \
do { \
	if (_spi_buffer_handle) \
		_lib_slog2f(_spi_buffer_handle, 0, SLOG2_CRITICAL, msg, ## __VA_ARGS__); \
	else                    \
		fprintf(stderr,msg , ## __VA_ARGS__); \
	} while (0)

#endif //spi_LOGGING_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/spi/include/spi_slog2.h $ $Rev: 772212 $")
#endif

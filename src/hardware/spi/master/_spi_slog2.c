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



#include <stdio.h>
#include "spi_slog2.h"

#define DEFAULT_LOGGING_LEVEL SLOG2_INFO
#define DATA_LOGGING_LEVEL    SLOG2_DEBUG2

slog2_buffer_set_config_t _buffer_config;
slog2_buffer_t _spi_buffer_handle;

static bool have_slog2_symbols = false;

int (*_lib_slog2f)( slog2_buffer_t buffer, uint16_t code, uint8_t severity, const char* format, ... );
int (*_lib_slog2_register)( slog2_buffer_set_config_t *config, slog2_buffer_t *handles, uint32_t flags );
int (*_lib_slog2_get_verbosity)( slog2_buffer_t buffer );
int (*_lib_slog2_set_verbosity)( slog2_buffer_t buffer, uint8_t verbosity );


/* function-like macro simplifies the code since no explicit casting is required */
#define GET_LIB_SYMBOL( __err, __handle, __sym )                        \
		if( __err == 0 ) {                                                  \
			_lib_ ## __sym = (typeof(_lib_##__sym))dlsym( __handle, #__sym ); \
			if( _lib_ ## __sym == NULL ) {                                   \
				__err |= -1;                                                \
			}                                                               \
		}

static int _get_slog2_symbols( void )
{
	int rc = -1;
	void *dll_handle = (void*)NULL;

	if( dlsym( RTLD_DEFAULT, "slog2f" ) ) {
		dll_handle = RTLD_DEFAULT;
	} else {
		/* libslog2.so is not loaded in the process' address space   */
		/* open it and make its symbols global to the entire process */
		dll_handle = dlopen( "libslog2.so.1", RTLD_LAZY | RTLD_GLOBAL );
	}

	if( dll_handle ) {
		rc = 0;
		GET_LIB_SYMBOL( rc, dll_handle, slog2_register );
		GET_LIB_SYMBOL( rc, dll_handle, slog2f );
		GET_LIB_SYMBOL( rc, dll_handle, slog2_get_verbosity );
		GET_LIB_SYMBOL( rc, dll_handle, slog2_set_verbosity );
	}

	return rc;
}



int
_spi_init_logging(const char * name)
{

	if(name == NULL) {
	    name = LOGGING_NAME;
	}

	if( false == have_slog2_symbols ) {
		if( 0 == _get_slog2_symbols() ) {
			have_slog2_symbols = true;
		} else {
			perror("Error loading slog2 dll symbols");
			return -1;
		}
	}

	_buffer_config.buffer_set_name = name;
	_buffer_config.num_buffers = 1;

	_buffer_config.verbosity_level = DEFAULT_LOGGING_LEVEL;

	_buffer_config.buffer_config[0].buffer_name = "normal";
	_buffer_config.buffer_config[0].num_pages = 2;

	if (-1 == _lib_slog2_register(&_buffer_config, &_spi_buffer_handle, 0)) {
		perror("Error registering slogger2 buffer");
		return -1;
	}

	return 0;
}

int
_spi_get_logging_level()
{
	return _lib_slog2_get_verbosity(_spi_buffer_handle);
}

void
_spi_set_logging_level(int level)
{
	_lib_slog2_set_verbosity(_spi_buffer_handle, level);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/spi/master/_spi_slog2.c $ $Rev: 772213 $")
#endif

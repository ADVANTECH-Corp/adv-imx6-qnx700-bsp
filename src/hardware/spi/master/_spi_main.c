/*
 * $QNXLicenseC: 
 * Copyright 2007, 2008, QNX Software Systems.  
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
 
#ifdef __USAGE
%C - SPI driver

Syntax:
%C [-u unit]

Options:
-u unit    Set spi unit number (default: 0).
-d         driver module name
-P         device file permissions (default: 0666)
#endif


#include <sys/procmgr.h>
#include <dlfcn.h>
#include "proto.h"

// globals
char     *UserParm;
unsigned devperm;
unsigned verb;

int main(int argc, char *argv[])
{
	spi_dev_t	*head = NULL, *tail = NULL, *dev;
	void		*drventry, *dlhdl;
	siginfo_t	info;
	sigset_t	set;
	int			i, c, devnum = 0;

	UserParm = NULL;
	const char * name = "spi_master";

 /* default permission for /dev/spi* entry */
	devperm = 0666;
	
	if (ThreadCtl(_NTO_TCTL_IO, 0) == -1) {
		spi_error("ThreadCtl");
		return (!EOK);
	}

	_spi_init_iofunc();

	if (_spi_init_logging(name) != 0){
		spi_error("spi-master logging failed");
		return 0;
	}
	verb = SLOG2_INFO;
	spi_info("Starting spi-master resource manager");

	while ((c = getopt(argc, argv, "u:U:P:v:d:")) != -1) {
		switch (c) {
			case 'u':
				devnum = strtol(optarg, NULL, 0);
				break;
			case 'U':
				UserParm = strdup(optarg);
				break;
			case 'P':
				if ( optarg ) {
					devperm = strtoul(optarg, NULL, 8);	// octal
				}
				break;
			case 'v':
				verb = strtol(optarg, NULL, 0);
				break;
			case 'd':
				if ((drventry = _spi_dlload(&dlhdl, optarg)) == NULL) {
					spi_error("spi_load_driver() failed");
					return (-1);
				}

				do {
					if ((dev = calloc(1, sizeof(spi_dev_t))) == NULL)
						goto cleanup;
	
					if (argv[optind] == NULL || *argv[optind] == '-')
						dev->opts = NULL;
					else
						dev->opts = strdup(argv[optind]);
					++optind;
					dev->funcs  = (spi_funcs_t *)drventry;
					dev->devnum = devnum++;
					dev->dlhdl  = dlhdl;

					i = _spi_create_instance(dev);

					if (i != EOK) {
						spi_error("spi_create_instance() failed");

						if (dev->opts)
							free(dev->opts);

						free(dev);
						goto cleanup;
					}

					if (head) {
						tail->next = dev;
						tail = dev;
					}
					else
						head = tail = dev;
				} while (optind < argc && *(optarg = argv[optind]) != '-'); 

				/* 
				 * Now we only support one dll
				 */
				goto start_spi;

				break;
		}
	}

start_spi:
	_spi_set_logging_level(verb);
	if (head) {



		/* background the process */
		procmgr_daemon(0, PROCMGR_DAEMON_NOCLOSE | PROCMGR_DAEMON_NODEVNULL);

		sigemptyset(&set);
		sigaddset(&set, SIGTERM);

		for (;;) {
			if (SignalWaitinfo(&set, &info) == -1)
				continue;

			if (info.si_signo == SIGTERM)
				break;
		}
	}

cleanup:
	dev=head;
	while (dev) {
		if (dev->ctp) {
			dispatch_unblock(dev->ctp);

		}

		if (dev->drvhdl) {
			resmgr_detach(dev->dpp, dev->id, _RESMGR_DETACH_ALL);
			dev->funcs->fini(dev->drvhdl);
		}

		if (dev->dpp) {
			dispatch_destroy(dev->dpp);
		}

		head = dev->next;

		if (dev->opts)
			free(dev->opts);

		free(dev);
		dev=head;
	}

	dlclose(dlhdl);

	return (EOK);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/spi/master/_spi_main.c $ $Rev: 809378 $")
#endif

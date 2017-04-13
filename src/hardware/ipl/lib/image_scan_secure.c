/*
 * $QNXLicenseC:
 * Copyright 2016, QNX Software Systems.
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
 *	image_scan_secure: Scan through memory looking for an image
 *                         and only boot if the signature is valid.
 */

#include <stdlib.h>
#include "ipl.h"
#include "uECC.h"
#include "sha2.h"
#include "pubkey.h"

#define BOOT_HEADER_LEN		8
#define uECC_256_SIG_BYTES	64

uint8_t			hash[DIGEST_LEN];
unsigned char		hscratch [256];
struct startup_header	startup_hdr;

//
//     Scan 1k boundaries for the image identifier byte and
//     then load the image only if the cryptographic signature
//     is valid.
//

unsigned long image_scan_secure (unsigned long start, unsigned long end) {
	unsigned long		lastaddr = -1;
	unsigned short		lastver = 0;
	const unsigned char	*sptr;
	unsigned		chunk_sz;
	unsigned		rem;
	sha256_t		shactx;
	int			rc;

	//
	// We assume that the images will all start on a 4 byte boundary
	//

	for (; start < end; start += 4) {

		copy ((unsigned long)(&startup_hdr), start, sizeof(startup_hdr));

		//
		//	No endian issues here since stored "naturally"
		//

		if (startup_hdr.signature != STARTUP_HDR_SIGNATURE)
			continue;

		//
		// Start secure boot (create a hash of the IFS and verify it against
		// the signature stored directly after the IFS.  The public key should
		// exist in pubkey.h
		//

		//
		// Creating sha256 hash of the IFS
		//
		ser_putstr("Creating IFS hash\n");
		sha256_init(&shactx);
		sptr = (const unsigned char*)(start - BOOT_HEADER_LEN);
		rem = startup_hdr.stored_size + BOOT_HEADER_LEN;
		while (rem > 0) {
			chunk_sz = min(sizeof(hscratch), rem);
			copy((unsigned long)hscratch, (unsigned long)sptr, chunk_sz);
			sha256_add(&shactx, hscratch, chunk_sz);
			rem  -= chunk_sz;
			sptr += chunk_sz;
		}
		sha256_done(&shactx, hash);

		//
		// The IFS signature should be directly after the image
		//
		copy((unsigned long)hscratch, (unsigned long)((const unsigned char*)(start + startup_hdr.stored_size)), uECC_256_SIG_BYTES);
		sptr = hscratch;

		//
		// Finally, verify the binary agaisnt the signature
		//
		ser_putstr( "\nVerifying signature\n" );
		rc = uECC_verify(pubkey, hash, sizeof(hash), sptr, uECC_secp256r1());
		if ( rc != 1 ) {
			ser_putstr( "Error verifying signature\n" );
			return (-1UL);
		}
		ser_putstr( "Signature Verified!!!\n\n" );

		//
		// Stash the version and address and continue looking
		// for something newer than we are (jump ahead by
		// startup_hdr.stored_size)
		//

		if (startup_hdr.version > lastver) {
			lastver = startup_hdr.version;
			lastaddr = start;
		}

		start += startup_hdr.stored_size - 4;
	}
	return(lastaddr);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/image_scan_secure.c $ $Rev: 811635 $")
#endif

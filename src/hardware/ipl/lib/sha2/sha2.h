/*
*
* Copyright (c) 2012, QNX Software Systems.
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
*/

// Author: Peter Williams (pwilliams@rim.com)

// sha2.h
//
//   definitions for implementation of the NIST SHS (secure hash standard)
//   implementation, straight from the PHUB 180-x spec
//

#ifndef __SHA2_H__
#define __SHA2_H__

#include <stdint.h>


// In this kernel code, we don't want all variations.  But we'll preserve them in the
// code in case we need them in the future.  These #defines are used to indicate which
// we implement

//#define IMPLEMENT_SHA1
//#define IMPLEMENT_SHA224
#define IMPLEMENT_SHA256
//#define IMPLEMENT_SHA384
//#define IMPLEMENT_SHA512


// constants

#define BLOCKSIZE_BITS_512  512
#define BLOCKSIZE_BITS_1024 1024

#define BLOCKSIZE_BYTES_512  (BLOCKSIZE_BITS_512  / 8)
#define BLOCKSIZE_BYTES_1024 (BLOCKSIZE_BITS_1024 / 8)

#define DIGEST_LEN  32

// 512 bit context for SHA224 and SHA256
struct sha512bitContext {
    uint64_t nbytes;                        // # bytes in the SHA hash
    uint32_t H[DIGEST_LEN];                 // intermediate hash values
    uint32_t M_nbytes;                      // # bytes in the message block
    unsigned char M[BLOCKSIZE_BYTES_512];   // the message block
};

// 1024 bit context for SHA384 and SHA512
struct sha1024bitContext {
    uint64_t  nbytes;                       // # bytes in the SHA hash
    uint64_t  H[DIGEST_LEN];                // intermediate hash values
    uint32_t  M_nbytes;                     // # bytes in the message block
    unsigned char M[BLOCKSIZE_BYTES_1024];  // the message block
};

#ifdef IMPLEMENT_SHA1
typedef struct sha512bitContext sha1_t;
void sha1_init( sha1_t *psha );
void sha1_add( sha1_t *psha, const void *data, uint32_t len );
void sha1_done( sha1_t *psha, unsigned char *digest );
#endif

#ifdef IMPLEMENT_SHA224
typedef struct sha512bitContext sha224_t;
void sha224_init( sha224_t *psha );
void sha224_add( sha224_t *psha, const void *data, uint32_t len );
void sha224_done( sha224_t *psha, unsigned char *digest );
#endif

#ifdef IMPLEMENT_SHA256
typedef struct sha512bitContext sha256_t;
void sha256_init( sha256_t *psha );
void sha256_add( sha256_t *psha, const void *data, uint32_t len );
void sha256_done( sha256_t *psha, unsigned char *digest );
#endif

#ifdef IMPLEMENT_SHA384
typedef struct sha1024bitContext sha384_t;
void sha384_init( sha384_t *psha );
void sha384_add( sha384_t *psha, const void *data, uint32_t len );
void sha384_done( sha384_t *psha, unsigned char *digest );
#endif

#ifdef IMPLEMENT_SHA512
typedef struct sha1024bitContext sha512_t;
void sha512_init( sha512_t *psha );
void sha512_add( sha512_t *psha, const void *data, uint32_t len );
void sha512_done( sha512_t *psha, unsigned char *digest );
#endif

#endif  // __SHA2_H__

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/sha2/sha2.h $ $Rev: 823226 $")
#endif

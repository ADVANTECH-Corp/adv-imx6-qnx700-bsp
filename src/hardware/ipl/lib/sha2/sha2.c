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

// sha256.c
//
//   SHS (secure hash standard) implementation, straight from the FIPS PUB 180-4 spec
//
//   Hashes covered are
//          SHA-1
//          SHA-224
//          SHA-256
//          SHA-384
//          SHA-512

// dependencies

#include <stdlib.h>
#include <string.h>
#include "sha2.h"


// If FAST_CODE is defined, we unroll the loops inherent in the SHA-256 algorithm.
// Much harder to read, but faster.  Doesn't apply to SHA-512.
#define FAST_CODE


// defines

//#define INCLUDE_TEST_CODE

// macros

#define SHR( x, n ) (x >> n)

#define Ch( x, y, z )  ((x & y) ^ ((~x) & z))
//#define Maj( x, y, z ) ((x & y) ^ (x & z) ^ (y & z))
// Note, the actual spec for Maj is defined above but the
// following is equivalent and will consumes less cpu cycles
#define Maj( x, y, z ) ((x & y) | ((x | y) & z))
#define Parity( x, y, z ) (x ^ y ^ z)

#define ROTL32( x, n ) ((x << n) | (x >> (32-n)))
#define MIN( x, y ) ((x) < (y) ? (x) : (y))

// SHA224 & SHA256 sigma & gamma functions
#define ROTR32( x, n ) ((x >> n) | (x << (32-n)))
#define Sigma256_0( x ) (ROTR32( x, 2 ) ^ ROTR32( x, 13 ) ^ ROTR32( x, 22 ))
#define Sigma256_1( x ) (ROTR32( x, 6 ) ^ ROTR32( x, 11 ) ^ ROTR32( x, 25 ))
#define Gamma256_0( x ) (ROTR32( x, 7 ) ^ ROTR32( x, 18 ) ^ SHR( x, 3  ))
#define Gamma256_1( x ) (ROTR32( x,17 ) ^ ROTR32( x, 19 ) ^ SHR( x, 10 ))

// SHA384 & SHA512 sigma & gamma functions
#define ROTR64( x, n ) ((x >> n) | (x << (64-n)))
#define Sigma512_0( x ) (ROTR64( x, 28 ) ^ ROTR64( x, 34 ) ^ ROTR64( x, 39 ))
#define Sigma512_1( x ) (ROTR64( x, 14 ) ^ ROTR64( x, 18 ) ^ ROTR64( x, 41 ))
#define Gamma512_0( x ) (ROTR64( x, 1  ) ^ ROTR64( x, 8  ) ^ SHR( x, 7 ))
#define Gamma512_1( x ) (ROTR64( x, 19 ) ^ ROTR64( x, 61 ) ^ SHR( x, 6 ))

// local data

#define K1 0x5a827999
#define K2 0x6ed9eba1
#define K3 0x8f1bbcdc
#define K4 0xca62c1d6

// SHA224 & SHA256 'K' values
static const uint32_t K[ 64 ] = {
   0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// SHA384 & SHA512 'K' values
static const uint64_t KL[ 80 ] = {
    UINT64_C( 0x428a2f98d728ae22 ), UINT64_C( 0x7137449123ef65cd ), UINT64_C( 0xb5c0fbcfec4d3b2f ), UINT64_C( 0xe9b5dba58189dbbc ),
    UINT64_C( 0x3956c25bf348b538 ), UINT64_C( 0x59f111f1b605d019 ), UINT64_C( 0x923f82a4af194f9b ), UINT64_C( 0xab1c5ed5da6d8118 ),
	UINT64_C( 0xd807aa98a3030242 ), UINT64_C( 0x12835b0145706fbe ), UINT64_C( 0x243185be4ee4b28c ), UINT64_C( 0x550c7dc3d5ffb4e2 ),
    UINT64_C( 0x72be5d74f27b896f ), UINT64_C( 0x80deb1fe3b1696b1 ), UINT64_C( 0x9bdc06a725c71235 ), UINT64_C( 0xc19bf174cf692694 ),
    UINT64_C( 0xe49b69c19ef14ad2 ), UINT64_C( 0xefbe4786384f25e3 ), UINT64_C( 0x0fc19dc68b8cd5b5 ), UINT64_C( 0x240ca1cc77ac9c65 ),
    UINT64_C( 0x2de92c6f592b0275 ), UINT64_C( 0x4a7484aa6ea6e483 ), UINT64_C( 0x5cb0a9dcbd41fbd4 ), UINT64_C( 0x76f988da831153b5 ),
    UINT64_C( 0x983e5152ee66dfab ), UINT64_C( 0xa831c66d2db43210 ), UINT64_C( 0xb00327c898fb213f ), UINT64_C( 0xbf597fc7beef0ee4 ),
    UINT64_C( 0xc6e00bf33da88fc2 ), UINT64_C( 0xd5a79147930aa725 ), UINT64_C( 0x06ca6351e003826f ), UINT64_C( 0x142929670a0e6e70 ),
    UINT64_C( 0x27b70a8546d22ffc ), UINT64_C( 0x2e1b21385c26c926 ), UINT64_C( 0x4d2c6dfc5ac42aed ), UINT64_C( 0x53380d139d95b3df ),
    UINT64_C( 0x650a73548baf63de ), UINT64_C( 0x766a0abb3c77b2a8 ), UINT64_C( 0x81c2c92e47edaee6 ), UINT64_C( 0x92722c851482353b ),
    UINT64_C( 0xa2bfe8a14cf10364 ), UINT64_C( 0xa81a664bbc423001 ), UINT64_C( 0xc24b8b70d0f89791 ), UINT64_C( 0xc76c51a30654be30 ),
    UINT64_C( 0xd192e819d6ef5218 ), UINT64_C( 0xd69906245565a910 ), UINT64_C( 0xf40e35855771202a ), UINT64_C( 0x106aa07032bbd1b8 ),
    UINT64_C( 0x19a4c116b8d2d0c8 ), UINT64_C( 0x1e376c085141ab53 ), UINT64_C( 0x2748774cdf8eeb99 ), UINT64_C( 0x34b0bcb5e19b48a8 ),
    UINT64_C( 0x391c0cb3c5c95a63 ), UINT64_C( 0x4ed8aa4ae3418acb ), UINT64_C( 0x5b9cca4f7763e373 ), UINT64_C( 0x682e6ff3d6b2b8a3 ),
    UINT64_C( 0x748f82ee5defb2fc ), UINT64_C( 0x78a5636f43172f60 ), UINT64_C( 0x84c87814a1f0ab72 ), UINT64_C( 0x8cc702081a6439ec ),
    UINT64_C( 0x90befffa23631e28 ), UINT64_C( 0xa4506cebde82bde9 ), UINT64_C( 0xbef9a3f7b2c67915 ), UINT64_C( 0xc67178f2e372532b ),
    UINT64_C( 0xca273eceea26619c ), UINT64_C( 0xd186b8c721c0c207 ), UINT64_C( 0xeada7dd6cde0eb1e ), UINT64_C( 0xf57d4f7fee6ed178 ),
    UINT64_C( 0x06f067aa72176fba ), UINT64_C( 0x0a637dc5a2c898a6 ), UINT64_C( 0x113f9804bef90dae ), UINT64_C( 0x1b710b35131c471b ),
    UINT64_C( 0x28db77f523047d84 ), UINT64_C( 0x32caab7b40c72493 ), UINT64_C( 0x3c9ebe0a15c9bebc ), UINT64_C( 0x431d67c49c100d4c ),
    UINT64_C( 0x4cc5d4becb3e42b6 ), UINT64_C( 0x597f299cfc657e2a ), UINT64_C( 0x5fcb6fab3ad6faec ), UINT64_C( 0x6c44198c4a475817 )
};


static inline uint32_t read32_highbytefirst( const unsigned char * const src )
{
    return ((uint32_t)src[0] << 24) | ((uint32_t)src[1] << 16) | ((uint32_t)src[2] << 8) | (uint32_t)src[3];
}

static inline uint64_t read64_highbytefirst( const unsigned char * const src )
{
    return (uint64_t)src[0] << 56 | (uint64_t)src[1] << 48 | (uint64_t)src[2] << 40 | (uint64_t)src[3] << 32
         | (uint64_t)src[4] << 24 | (uint64_t)src[5] << 16 | (uint64_t)src[6] << 8  | (uint64_t)src[7];
}

static inline void write32_highbytefirst( const uint32_t data, unsigned char * dst )
{
    *dst++ = (data >> 24) & 0xFFu;
    *dst++ = (data >> 16) & 0xFFu;
    *dst++ = (data >>  8) & 0xFFu;
    *dst   = (data)       & 0xFFu;
}

static inline void write64_highbytefirst( const uint64_t data, unsigned char *dst )
{
    *dst++ = (data >> 56) & 0xFFu;
    *dst++ = (data >> 48) & 0xFFu;
    *dst++ = (data >> 40) & 0xFFu;
    *dst++ = (data >> 32) & 0xFFu;
    *dst++ = (data >> 24) & 0xFFu;
    *dst++ = (data >> 16) & 0xFFu;
    *dst++ = (data >>  8) & 0xFFu;
    *dst   = (data)       & 0xFFu;
}


// process_512bit_message_block
//
//   this implements the SHA algoritm on a 512 bit message block, which is
//   used for SHA224 and SHA256 calculations. The only difference between
//   these two calculations is the size of the message digest
//

#define UNROLL( a, b, c, d, e, f, g, h, Wi, Ki )         \
		t1 = h + Sigma256_1( e ) + Ch( e, f, g ) + Ki + W[ Wi ]; \
		t2 = Sigma256_0( a ) + Maj( a, b, c );                   \
		d += t1;                                                 \
		h = t1 + t2;

static void process_512bit_message_block( struct sha512bitContext *psha, const unsigned char * const M )
{
    register uint32_t a, b, c, d, e, f, g, h;    // working hash variables
    uint32_t W[64];                              // the message schedule
    uint32_t t, t1, t2;                          // temp variables

    // prepare the message schedule
    for ( t = 0; t < 16; t++ ) {
        W[ t ] = read32_highbytefirst( M + (4 * t) );
    }
    for ( t = 16; t < 64; t++ ) {
        W[ t ] = Gamma256_1( W[ t-2 ] ) + W[ t-7 ] + Gamma256_0( W[ t-15 ] ) + W[ t-16 ];
    }

    // initialize working variables
    a = psha->H[ 0 ];
    b = psha->H[ 1 ];
    c = psha->H[ 2 ];
    d = psha->H[ 3 ];
    e = psha->H[ 4 ];
    f = psha->H[ 5 ];
    g = psha->H[ 6 ];
    h = psha->H[ 7 ];

    // process the message schedule
#if !defined( FAST_CODE )
    // this is the message block process straight from the spec
    for ( t = 0; t < 64; t++ ) {
        t1 = h + Sigma256_1( e ) + Ch( e, f, g ) + K[ t ] + W[ t ];
        t2 = Sigma256_0( a ) + Maj( a, b, c );
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
#else
    // this implements the message block process by unrolling the loop calls

    UNROLL( a, b, c, d, e, f, g, h, 0,  0x428a2f98 );
    UNROLL( h, a, b, c, d, e, f, g, 1,  0x71374491 );
    UNROLL( g, h, a, b, c, d, e, f, 2,  0xb5c0fbcf );
    UNROLL( f, g, h, a, b, c, d, e, 3,  0xe9b5dba5 );
    UNROLL( e, f, g, h, a, b, c, d, 4,  0x3956c25b );
    UNROLL( d, e, f, g, h, a, b, c, 5,  0x59f111f1 );
    UNROLL( c, d, e, f, g, h, a, b, 6,  0x923f82a4 );
    UNROLL( b, c, d, e, f, g, h, a, 7,  0xab1c5ed5 );
    UNROLL( a, b, c, d, e, f, g, h, 8,  0xd807aa98 );
    UNROLL( h, a, b, c, d, e, f, g, 9,  0x12835b01 );
    UNROLL( g, h, a, b, c, d, e, f, 10, 0x243185be );
    UNROLL( f, g, h, a, b, c, d, e, 11, 0x550c7dc3 );
    UNROLL( e, f, g, h, a, b, c, d, 12, 0x72be5d74 );
    UNROLL( d, e, f, g, h, a, b, c, 13, 0x80deb1fe );
    UNROLL( c, d, e, f, g, h, a, b, 14, 0x9bdc06a7 );
    UNROLL( b, c, d, e, f, g, h, a, 15, 0xc19bf174 );
    UNROLL( a, b, c, d, e, f, g, h, 16, 0xe49b69c1 );
    UNROLL( h, a, b, c, d, e, f, g, 17, 0xefbe4786 );
    UNROLL( g, h, a, b, c, d, e, f, 18, 0x0fc19dc6 );
    UNROLL( f, g, h, a, b, c, d, e, 19, 0x240ca1cc );
    UNROLL( e, f, g, h, a, b, c, d, 20, 0x2de92c6f );
    UNROLL( d, e, f, g, h, a, b, c, 21, 0x4a7484aa );
    UNROLL( c, d, e, f, g, h, a, b, 22, 0x5cb0a9dc );
    UNROLL( b, c, d, e, f, g, h, a, 23, 0x76f988da );
    UNROLL( a, b, c, d, e, f, g, h, 24, 0x983e5152 );
    UNROLL( h, a, b, c, d, e, f, g, 25, 0xa831c66d );
    UNROLL( g, h, a, b, c, d, e, f, 26, 0xb00327c8 );
    UNROLL( f, g, h, a, b, c, d, e, 27, 0xbf597fc7 );
    UNROLL( e, f, g, h, a, b, c, d, 28, 0xc6e00bf3 );
    UNROLL( d, e, f, g, h, a, b, c, 29, 0xd5a79147 );
    UNROLL( c, d, e, f, g, h, a, b, 30, 0x06ca6351 );
    UNROLL( b, c, d, e, f, g, h, a, 31, 0x14292967 );
    UNROLL( a, b, c, d, e, f, g, h, 32, 0x27b70a85 );
    UNROLL( h, a, b, c, d, e, f, g, 33, 0x2e1b2138 );
    UNROLL( g, h, a, b, c, d, e, f, 34, 0x4d2c6dfc );
    UNROLL( f, g, h, a, b, c, d, e, 35, 0x53380d13 );
    UNROLL( e, f, g, h, a, b, c, d, 36, 0x650a7354 );
    UNROLL( d, e, f, g, h, a, b, c, 37, 0x766a0abb );
    UNROLL( c, d, e, f, g, h, a, b, 38, 0x81c2c92e );
    UNROLL( b, c, d, e, f, g, h, a, 39, 0x92722c85 );
    UNROLL( a, b, c, d, e, f, g, h, 40, 0xa2bfe8a1 );
    UNROLL( h, a, b, c, d, e, f, g, 41, 0xa81a664b );
    UNROLL( g, h, a, b, c, d, e, f, 42, 0xc24b8b70 );
    UNROLL( f, g, h, a, b, c, d, e, 43, 0xc76c51a3 );
    UNROLL( e, f, g, h, a, b, c, d, 44, 0xd192e819 );
    UNROLL( d, e, f, g, h, a, b, c, 45, 0xd6990624 );
    UNROLL( c, d, e, f, g, h, a, b, 46, 0xf40e3585 );
    UNROLL( b, c, d, e, f, g, h, a, 47, 0x106aa070 );
    UNROLL( a, b, c, d, e, f, g, h, 48, 0x19a4c116 );
    UNROLL( h, a, b, c, d, e, f, g, 49, 0x1e376c08 );
    UNROLL( g, h, a, b, c, d, e, f, 50, 0x2748774c );
    UNROLL( f, g, h, a, b, c, d, e, 51, 0x34b0bcb5 );
    UNROLL( e, f, g, h, a, b, c, d, 52, 0x391c0cb3 );
    UNROLL( d, e, f, g, h, a, b, c, 53, 0x4ed8aa4a );
    UNROLL( c, d, e, f, g, h, a, b, 54, 0x5b9cca4f );
    UNROLL( b, c, d, e, f, g, h, a, 55, 0x682e6ff3 );
    UNROLL( a, b, c, d, e, f, g, h, 56, 0x748f82ee );
    UNROLL( h, a, b, c, d, e, f, g, 57, 0x78a5636f );
    UNROLL( g, h, a, b, c, d, e, f, 58, 0x84c87814 );
    UNROLL( f, g, h, a, b, c, d, e, 59, 0x8cc70208 );
    UNROLL( e, f, g, h, a, b, c, d, 60, 0x90befffa );
    UNROLL( d, e, f, g, h, a, b, c, 61, 0xa4506ceb );
    UNROLL( c, d, e, f, g, h, a, b, 62, 0xbef9a3f7 );
    UNROLL( b, c, d, e, f, g, h, a, 63, 0xc67178f2 );
#endif  // !FAST_CODE

    // write out the intermediate hash
    psha->H[0] += a;
    psha->H[1] += b;
    psha->H[2] += c;
    psha->H[3] += d;
    psha->H[4] += e;
    psha->H[5] += f;
    psha->H[6] += g;
    psha->H[7] += h;
}

#if defined(IMPLEMENT_SHA512) || defined(IMPLEMENT_SHA384)
// process_1024bit_message_block
//
//   this implements the SHA algoritm on a 1024 bit message block, which is
//   used for SHA384 and SHA512 calculations. The only difference between
//   these two calculations is the size of the message digest
//
static void process_1024bit_message_block( struct sha1024bitContext *psha, const unsigned char * const M )
{
    uint64_t W[80];
    uint64_t t, t1, t2;
    uint64_t a, b, c, d, e, f, g, h;

    // prepare the message schedule
    for ( t = 0; t < 16; t++ ) {
        W[ t ] = read64_highbytefirst( M + (8 * t) );
    }
    for ( t = 16; t < 80; t++ ) {
        W[ t ] = Gamma512_1( W[ t-2 ] ) + W[ t-7 ] + Gamma512_0( W[ t-15 ] ) + W[ t-16 ];
    }

    // initialize working variables
    a = psha->H[ 0 ];
    b = psha->H[ 1 ];
    c = psha->H[ 2 ];
    d = psha->H[ 3 ];
    e = psha->H[ 4 ];
    f = psha->H[ 5 ];
    g = psha->H[ 6 ];
    h = psha->H[ 7 ];

    // process the message schedule
    for ( t = 0; t < 80; t++ ) {
        t1 = h + Sigma512_1( e ) + Ch( e, f, g ) + KL[ t ] + W[ t ];
        t2 = Sigma512_0( a ) + Maj( a, b, c );
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // write out the intermediate hash
    psha->H[0] += a;
    psha->H[1] += b;
    psha->H[2] += c;
    psha->H[3] += d;
    psha->H[4] += e;
    psha->H[5] += f;
    psha->H[6] += g;
    psha->H[7] += h;
}
#endif



static void sha_add_to_512bit_block( struct sha512bitContext *psha, const unsigned char *data, uint32_t len )
{
    // loop for as long as there is data to process
    while ( len > 0 ) {
        if (psha->M_nbytes == 0 && len >= BLOCKSIZE_BYTES_512) {
            // if the message block is empty and we have an entire message
            // block of data available, process a message block size of data
            process_512bit_message_block( psha, data );
            psha->nbytes += BLOCKSIZE_BYTES_512;
            data += BLOCKSIZE_BYTES_512;
            len -= BLOCKSIZE_BYTES_512;
        } else {
            // otherwise add data to the message block until the block
            // is full, or no more data
            const uint32_t bytes_to_copy = MIN( len, BLOCKSIZE_BYTES_512 - psha->M_nbytes );
            memcpy( psha->M + psha->M_nbytes, data, bytes_to_copy );
            data += bytes_to_copy;
            len -= bytes_to_copy;
            psha->M_nbytes += bytes_to_copy;
            // if the message block is full, process the block
            if (psha->M_nbytes == BLOCKSIZE_BYTES_512) {
                process_512bit_message_block( psha, psha->M );
                psha->nbytes += BLOCKSIZE_BYTES_512;
                psha->M_nbytes = 0;
            }
        }
    }
}

#if defined(IMPLEMENT_SHA512) || defined(IMPLEMENT_SHA384)
static void sha_add_to_1024bit_block( struct sha1024bitContext *psha, const unsigned char *data, uint32_t len )
{
    // loop for as long as there is data to process
    while ( len > 0 ) {
        if (psha->M_nbytes == 0 && len >= BLOCKSIZE_BYTES_1024) {
            // if the message block is empty and we have an entire message
            // block of data available, process a message block size of data
            process_1024bit_message_block( psha, data );
            psha->nbytes += BLOCKSIZE_BYTES_1024;
            data += BLOCKSIZE_BYTES_1024;
            len -= BLOCKSIZE_BYTES_1024;
        } else {
            // otherwise add data to the message block until the block
            // is full, or no more data
            const uint32_t bytes_to_copy = MIN( len, BLOCKSIZE_BYTES_1024 - psha->M_nbytes );
            memcpy( psha->M + psha->M_nbytes, data, bytes_to_copy );
            data += bytes_to_copy;
            len -= bytes_to_copy;
            psha->M_nbytes += bytes_to_copy;
            // if the message block is full, process the block
            if (psha->M_nbytes == BLOCKSIZE_BYTES_1024) {
                process_1024bit_message_block( psha, psha->M );
                psha->nbytes += BLOCKSIZE_BYTES_1024;
                psha->M_nbytes = 0;
            }
        }
    }
}
#endif


static void close_512bit_hash( struct sha512bitContext *psha )
{
    // add size of remaining buffer to message length
    psha->nbytes += psha->M_nbytes;

    // append the trailing '1'
    psha->M[ psha->M_nbytes++ ] = 0x80;

    if ( psha->M_nbytes > (BLOCKSIZE_BYTES_512 - 8) ) {
        // if message block is too short for length, pad with 0's and write
        memset( psha->M + psha->M_nbytes, 0, BLOCKSIZE_BYTES_512 - psha->M_nbytes );
        process_512bit_message_block( psha, psha->M );
        psha->M_nbytes = 0;
    }

    // pad message block with 0's leaving room for length
    memset( psha->M + psha->M_nbytes, 0, (BLOCKSIZE_BYTES_512 - 8) - psha->M_nbytes );

    // add the length and write the last message block
    write64_highbytefirst( psha->nbytes * 8, psha->M + (BLOCKSIZE_BYTES_512 - 8) );
    process_512bit_message_block( psha, psha->M );
}


#if defined(IMPLEMENT_SHA512) || defined(IMPLEMENT_SHA384)
static void close_1024bit_hash( struct sha1024bitContext *psha )
{
    // add size of remaining buffer to message length
    psha->nbytes += psha->M_nbytes;

    // append the trailing '1'
    psha->M[ psha->M_nbytes++ ] = 0x80;

    if ( psha->M_nbytes > (BLOCKSIZE_BYTES_1024 - 16) ) {
        // if message block is too short for length, pad with 0's and write
        memset( psha->M + psha->M_nbytes, 0, BLOCKSIZE_BYTES_1024 - psha->M_nbytes );
        process_1024bit_message_block( psha, psha->M );
        psha->M_nbytes = 0;
    }

    // pad message block with 0's leaving room for length
    memset( psha->M + psha->M_nbytes, 0, (BLOCKSIZE_BYTES_1024 - 16) - psha->M_nbytes );

    // add the length and write the last message block
    memset( psha->M + (BLOCKSIZE_BYTES_1024 - 16), 0, 4 );  // cheating the high 64 bits to 0
    write64_highbytefirst( psha->nbytes * 8, psha->M + (BLOCKSIZE_BYTES_1024 - 8) );
    process_1024bit_message_block( psha, psha->M );
}
#endif

// write32bit_digest    used for SHA-1, SHA-224 and SHA-256
// write64bit_digest    used for SHA-384 and SHA-512
//
//    Write out the internal hash as a message digest

static void write32bit_digest( const uint32_t *hash, unsigned char *digest, uint32_t size )
{
    for ( ; size > 0; size--, hash++, digest += 4 ) {
        write32_highbytefirst( *hash, digest );
    }
}

#if defined(IMPLEMENT_SHA512) || defined(IMPLEMENT_SHA384)
static void write64bit_digest( const uint64_t *hash, unsigned char *digest, uint32_t size )
{
    for ( ; size > 0; size--, hash++, digest += 8 ) {
        write64_highbytefirst( *hash, digest );
    }
}
#endif


/////////////////////////////////////////////////
//                                             //
//                SHA-224                      //
//                                             //
/////////////////////////////////////////////////
#ifdef IMPLEMENT_SHA224

void sha224_init( sha224_t *psha )
{
    static const uint32_t H_init[] = {
        0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939, 0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4
    };
    psha->nbytes = 0;
    psha->M_nbytes = 0;
    memcpy( psha->H, H_init, sizeof( H_init ) );
/*
    psha->H[0] = 0xc1059ed8;
    psha->H[1] = 0x367cd507;
    psha->H[2] = 0x3070dd17;
    psha->H[3] = 0xf70e5939;
    psha->H[4] = 0xffc00b31;
    psha->H[5] = 0x68581511;
    psha->H[6] = 0x64f98fa7;
    psha->H[7] = 0xbefa4fa4;
*/
}

void sha224_add( sha224_t *psha, const void * const v, uint32_t len )
{
	const unsigned char * const data = v;
    sha_add_to_512bit_block( psha, data, len );
}

void sha224_done( sha224_t *psha, unsigned char *digest )
{
    // processes a trailing '1' and a message length into the hash
    close_512bit_hash( psha );

    // write the hash to the digest
    write32bit_digest( psha->H, digest, 7 );
}

#endif // IMPLEMENT_SHA224


/////////////////////////////////////////////////
//                                             //
//                SHA-256                      //
//                                             //
/////////////////////////////////////////////////
#ifdef IMPLEMENT_SHA256

void sha256_init( sha256_t *psha )
{
    static const uint32_t H_init[] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    psha->nbytes = 0;
    psha->M_nbytes = 0;
    memcpy( psha->H, H_init, sizeof( H_init ) );
/*
    psha->H[0] = 0x6a09e667;
    psha->H[1] = 0xbb67ae85;
    psha->H[2] = 0x3c6ef372;
    psha->H[3] = 0xa54ff53a;
    psha->H[4] = 0x510e527f;
    psha->H[5] = 0x9b05688c;
    psha->H[6] = 0x1f83d9ab;
    psha->H[7] = 0x5be0cd19;
*/
}

void sha256_add( sha256_t *psha, const void * const v, uint32_t len )
{
	const unsigned char * const data = v;
    sha_add_to_512bit_block( psha, data, len );
}

void sha256_done( sha256_t *psha, unsigned char *digest )
{
    // processes a trailing '1' and a message length into the hash
    close_512bit_hash( psha );

    // write the hash to the digest
    write32bit_digest( psha->H, digest, 8 );
}

#endif // IMPLEMENT_SHA256


/////////////////////////////////////////////////
//                                             //
//                SHA-384                      //
//                                             //
/////////////////////////////////////////////////
#ifdef IMPLEMENT_SHA384

void sha384_init( sha384_t *psha )
{
    static const uint64_t H_init[] = {
   		UINT64_C( 0xcbbb9d5dc1059ed8 ), UINT64_C( 0x629a292a367cd507 ), UINT64_C( 0x9159015a3070dd17 ), UINT64_C( 0x152fecd8f70e5939 ),
        UINT64_C( 0x67332667ffc00b31 ), UINT64_C( 0x8eb44a8768581511 ), UINT64_C( 0xdb0c2e0d64f98fa7 ), UINT64_C( 0x47b5481dbefa4fa4 )
    };
    psha->nbytes = 0;
    psha->M_nbytes = 0;
    memcpy( psha->H, H_init, sizeof( H_init ) );
/*
    psha->H[0] = 0xcbbb9d5dc1059ed8;
    psha->H[1] = 0x629a292a367cd507;
    psha->H[2] = 0x9159015a3070dd17;
    psha->H[3] = 0x152fecd8f70e5939;
    psha->H[4] = 0x67332667ffc00b31;
    psha->H[5] = 0x8eb44a8768581511;
    psha->H[6] = 0xdb0c2e0d64f98fa7;
    psha->H[7] = 0x47b5481dbefa4fa4;
*/
}

void sha384_add( sha384_t *psha, const void * const v, uint32_t len )
{
	const unsigned char * const data = v;
    sha_add_to_1024bit_block( psha, data, len );
}

void sha384_done( sha384_t *psha, unsigned char *digest )
{
    // processes a trailing '1' and a message length into the hash
    close_1024bit_hash( psha );

    // write the hash to the digest
    write64bit_digest( psha->H, digest, 7 );
}

#endif // IMPLEMENT_SHA384


/////////////////////////////////////////////////
//                                             //
//                SHA-512                      //
//                                             //
/////////////////////////////////////////////////
#ifdef IMPLEMENT_SHA512

void sha512_init( sha512_t *psha )
{
    static const uint64_t H_init[] = {
		UINT64_C( 0x6a09e667f3bcc908 ), UINT64_C( 0xbb67ae8584caa73b ), UINT64_C( 0x3c6ef372fe94f82b ), UINT64_C( 0xa54ff53a5f1d36f1 ),
		UINT64_C( 0x510e527fade682d1 ), UINT64_C( 0x9b05688c2b3e6c1f ), UINT64_C( 0x1f83d9abfb41bd6b ), UINT64_C( 0x5be0cd19137e2179 )
    };
    psha->nbytes = 0;
    psha->M_nbytes = 0;
    memcpy( psha->H, H_init, sizeof( H_init ) );
/*
    psha->H[0] = 0x6a09e667f3bcc908;
    psha->H[1] = 0xbb67ae8584caa73b;
    psha->H[2] = 0x3c6ef372fe94f82b;
    psha->H[3] = 0xa54ff53a5f1d36f1;
    psha->H[4] = 0x510e527fade682d1;
    psha->H[5] = 0x9b05688c2b3e6c1f;
    psha->H[6] = 0x1f83d9abfb41bd6b;
    psha->H[7] = 0x5be0cd19137e2179;
*/
}

void sha512_add( sha512_t *psha, const void * const v, uint32_t len )
{
	const unsigned char * const data = v;
    sha_add_to_1024bit_block( psha, data, len );
}

void sha512_done( sha512_t *psha, unsigned char *digest )
{
    // processes a trailing '1' and a message length into the hash
    close_1024bit_hash( psha );

    // write the hash to the digest
    write64bit_digest( psha->H, digest, 8 );
}

#endif // IMPLEMENT_SHA512




#ifdef IMPLEMENT_SHA1

void sha1_init( sha1_t *psha )
{
    static const uint32_t H_init[] = {
        0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0
    };

    psha->nbytes = 0;
    psha->M_nbytes = 0;
    memcpy( psha->H, H_init, sizeof( H_init ) );
/*
    psha->H[0] = 0x67452301;
    psha->H[1] = 0xefcdab89;
    psha->H[2] = 0x98badcfe;
    psha->H[3] = 0x10325476;
    psha->H[4] = 0xc3d2e1f0;
*/
}

static void process_sha1_message_block( struct sha512bitContext *psha, const unsigned char * const M )
{
    register uint32_t a, b, c, d, e;             // working hash variables
    uint32_t W[80];                              // the message schedule
    uint32_t t, T;                               // temp variables

    // prepare the message schedule
    for ( t = 0; t < 16; t++ ) {
        W[ t ] = read32_highbytefirst( M + (4 * t) );
    }
    for ( t = 16; t < 80; t++ ) {
        T = W[ t-3 ] ^ W[ t-8 ] ^ W[ t-14 ] ^ W[ t-16 ];
        W[ t ] = ROTL32( T, 1 );
    }

    // initialize working variables
    a = psha->H[ 0 ];
    b = psha->H[ 1 ];
    c = psha->H[ 2 ];
    d = psha->H[ 3 ];
    e = psha->H[ 4 ];

    // process the message schedule
    for ( t = 0; t < 20; t++ ) {
        T = ROTL32( a, 5 ) + Ch( b, c, d ) + e + K1 + W[ t ];
        e = d;
        d = c;
        c = ROTL32( b, 30 );
        b = a;
        a = T;
    }
    for ( t = 20; t < 40; t++ ) {
        T = ROTL32( a, 5 ) + Parity( b, c, d ) + e + K2 + W[ t ];
        e = d;
        d = c;
        c = ROTL32( b, 30 );
        b = a;
        a = T;
    }
    for ( t = 40; t < 60; t++ ) {
        T = ROTL32( a, 5 ) + Maj( b, c, d ) + e + K3 + W[ t ];
        e = d;
        d = c;
        c = ROTL32( b, 30 );
        b = a;
        a = T;
    }
    for ( t = 60; t < 80; t++ ) {
        T = ROTL32( a, 5 ) + Parity( b, c, d ) + e + K4 + W[ t ];
        e = d;
        d = c;
        c = ROTL32( b, 30 );
        b = a;
        a = T;
    }

    // write out the intermediate hash
    psha->H[0] += a;
    psha->H[1] += b;
    psha->H[2] += c;
    psha->H[3] += d;
    psha->H[4] += e;
}

void sha1_add( sha1_t *psha, const void * const v, uint32_t len )
{
	const unsigned char * data = v;
    // loop for as long as there is data to process
    while ( len > 0 ) {
        if (psha->M_nbytes == 0 && len >= BLOCKSIZE_BITS_512) {
            // if the message block is empty and we have an entire message
            // block of data available, process a message block size of data
            process_sha1_message_block( psha, data );
            psha->nbytes += BLOCKSIZE_BYTES_512;
            data += BLOCKSIZE_BYTES_512;
            len -= BLOCKSIZE_BYTES_512;
        } else {
            // otherwise add data to the message block until the block
            // is full, or no more data
            const uint32_t bytes_to_copy = MIN( len, BLOCKSIZE_BYTES_512 - psha->M_nbytes );
            memcpy( psha->M + psha->M_nbytes, data, bytes_to_copy );
            data += bytes_to_copy;
            len -= bytes_to_copy;
            psha->M_nbytes += bytes_to_copy;
            // if the message block is full, process the block
            if (psha->M_nbytes == BLOCKSIZE_BYTES_512) {
                process_sha1_message_block( psha, psha->M );
                psha->nbytes += BLOCKSIZE_BYTES_512;
                psha->M_nbytes = 0;
            }
        }
    }
}


static void close_sha1_hash( sha1_t *psha )
{
    // add size of remaining buffer to message length
    psha->nbytes += psha->M_nbytes;

    // append the trailing '1'
    psha->M[ psha->M_nbytes++ ] = 0x80;

    if ( psha->M_nbytes > 56 ) {
        // if message block is too short for length, pad with 0's and write
        memset( psha->M + psha->M_nbytes, 0, BLOCKSIZE_BYTES_512 - psha->M_nbytes );
        process_sha1_message_block( psha, psha->M );
        psha->M_nbytes = 0;
    }

    // pad message block with 0's leaving room for length
    memset( psha->M + psha->M_nbytes, 0, 56 - psha->M_nbytes );

    // add the length and write the last message block
    write64_highbytefirst( (uint64_t)(psha->nbytes) * 8, psha->M + 56 );
    process_sha1_message_block( psha, psha->M );
}

// sha1_done
//
//   close the sha-1 hash context and write out the message digest (or hash)

void sha1_done( sha1_t *psha, unsigned char *digest )
{
    // processes a trailing '1' and a message length into the hash
    close_sha1_hash( psha );

    // write the hash to the digest
    write32bit_digest( psha->H, digest, 5 );
}

#endif // IMPLEMENT_SHA1




#if defined( INCLUDE_TEST_CODE )

#include <stdio.h>


static void dump_512bit_message_block( const unsigned char *pm )
{
    int i;

    for ( i = 0; i < 32; i++ ) printf( "%2.2X", *pm++ ); printf( "\n" );
    for (      ; i < 64; i++ ) printf( "%2.2X", *pm++ ); printf( "\n" );
}
static void dump_1024bit_message_block( const unsigned char *pm )
{
    int i;

    for ( i = 0; i < 32; i++ ) printf( "%2.2X", *pm++ ); printf( "\n" );
    for (      ; i < 64; i++ ) printf( "%2.2X", *pm++ ); printf( "\n" );
    for (      ; i < 96; i++ ) printf( "%2.2X", *pm++ ); printf( "\n" );
    for (      ; i < 128; i++ ) printf( "%2.2X", *pm++ ); printf( "\n" );
}


static void sha_print_digest( unsigned char *digest, uint32_t length )
{
    while ( length-- > 0 ) {
        printf( "%2.2X", *digest++ );
    }
    putchar( '\n' );
}
void sha1_print_digest( unsigned char *digest )
{
    sha_print_digest( digest, 20 );
}
void sha224_print_digest( unsigned char *digest )
{
    sha_print_digest( digest, 28 );
}
void sha256_print_digest( unsigned char *digest )
{
    sha_print_digest( digest, 32 );
}
void sha384_print_digest( unsigned char *digest )
{
    sha_print_digest( digest, 32 );
    sha_print_digest( digest + 32, 16 );
}
void sha512_print_digest( unsigned char *digest )
{
    sha_print_digest( digest, 32 );
    sha_print_digest( digest + 32, 32 );
}

void sha1_string( unsigned char *s )
{
    sha1_t sha;
    unsigned char digest[32];

    printf( "SHA-1(\"%s\")\n", s );

    sha1_init( &sha );
    sha1_add( &sha, s, strlen( (char *) s ) );
    sha1_done( &sha, digest );
    sha1_print_digest( digest );
}

void sha224_string( unsigned char *s )
{
    sha224_t sha;
    unsigned char digest[32];

    printf( "SHA224(\"%s\")\n", s );

    sha224_init( &sha );
    sha224_add( &sha, s, strlen( (char *) s ) );
    sha224_done( &sha, digest );
    sha224_print_digest( digest );
}

void sha256_string( unsigned char *s )
{
    sha256_t sha;
    unsigned char digest[32];

    printf( "SHA256(\"%s\")\n", s );

    sha256_init( &sha );
    sha256_add( &sha, s, strlen( (char *) s ) );
    sha256_done( &sha, digest );
    sha256_print_digest( digest );
}

void sha384_string( unsigned char *s )
{
    sha384_t sha;
    unsigned char digest[64];

    printf( "SHA384(\"%s\")\n", s );

    sha384_init( &sha );
    sha384_add( &sha, s, strlen( (char *) s ) );
    sha384_done( &sha, digest );
    sha384_print_digest( digest );
}

void sha512_string( unsigned char *s )
{
    sha512_t sha;
    unsigned char digest[64];

    printf( "SHA512(\"%s\")\n", s );

    sha512_init( &sha );
    sha512_add( &sha, s, strlen( (char *) s ) );
    sha512_done( &sha, digest );
    sha512_print_digest( digest );
}

static unsigned char tstmsg0[] = "";
static unsigned char tstmsg1[] = "The quick brown fox jumps over the lazy dog";
static unsigned char tstmsg2[] = "The quick brown fox jumps over the lazy dog.";

void sha1_test( void )
{
    sha1_string( tstmsg0 );
    sha1_string( tstmsg1 );
    sha1_string( tstmsg2 );
}

void sha224_test( void )
{
    sha224_string( tstmsg0 );
    sha224_string( tstmsg1 );
    sha224_string( tstmsg2 );
}

void sha256_test( void )
{
    sha256_string( tstmsg0 );
    sha256_string( tstmsg1 );
    sha256_string( tstmsg2 );
}

void sha384_test( void )
{
    sha384_string( tstmsg0 );
    sha384_string( tstmsg1 );
    sha384_string( tstmsg2 );
}

void sha512_test( void )
{
    sha512_string( tstmsg0 );
    sha512_string( tstmsg1 );
    sha512_string( tstmsg2 );
}

// valid hash values for these strings are posted online at the sha wiki

int main( int argc, char **argv )
{
    sha1_test( );
    sha224_test( );
    sha256_test( );
    sha384_test( );
    sha512_test( );

    return 0;
}

#endif  // INCLUDE_TEST_CODE

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/branches/7.0.0/trunk/hardware/ipl/lib/sha2/sha2.c $ $Rev: 823226 $")
#endif

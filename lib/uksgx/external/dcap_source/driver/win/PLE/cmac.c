/**
 * \file cmac.c
 *
 * \brief NIST SP800-38B compliant CMAC implementation for AES and 3DES
 *
 *  Copyright (C) 2006-2016, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

/*
 * References:
 *
 * - NIST SP 800-38B Recommendation for Block Cipher Modes of Operation: The
 *      CMAC Mode for Authentication
 *   http://nvlpubs.nist.gov/nistpubs/SpecialPublications/NIST.SP.800-38b.pdf
 *
 * - RFC 4493 - The AES-CMAC Algorithm
 *   https://tools.ietf.org/html/rfc4493
 *
 * - RFC 4615 - The Advanced Encryption Standard-Cipher-based Message
 *      Authentication Code-Pseudo-Random Function-128 (AES-CMAC-PRF-128)
 *      Algorithm for the Internet Key Exchange Protocol (IKE)
 *   https://tools.ietf.org/html/rfc4615
 *
 *   Additional test vectors: ISO/IEC 9797-1
 *
 */


#include <string.h>


#include "mbedtls/cmac.h"

#ifndef asm
#define asm __asm
#endif


static int mbedtls_aesni_crypt_ecb( mbedtls_aes_context *ctx,
                             const unsigned char input[16],
                             unsigned char output[16] )
{
    asm( "movdqu    (%1), %%xmm0    \n\t" // load input
            "movdqu    (%0), %%xmm1    \n\t" // load round key 0
            "pxor      %%xmm1, %%xmm0  \n\t" // round 0
            "add       $16, %0         \n\t" // point to next round key
            "mov     $9, %%rcx        \n\t" //Set total rounds to 10
            "1:                        \n\t" // encryption loop
            "movdqu    (%0), %%xmm1    \n\t" // load round key
    AESENC     xmm1_xmm0      "\n\t" // do round
            "add       $16, %0         \n\t" // point to next round key
            "sub      $1, %%rcx          \n\t" // loop
            "jnz       1b              \n\t"
            "movdqu    (%0), %%xmm1    \n\t" // load round key
    AESENCLAST xmm1_xmm0      "\n\t" // last round

            "3:                        \n\t"
            "movdqu    %%xmm0, (%2)    \n\t" // export output
    :
    : "r" (ctx->rk), "r" (input), "r" (output)
    : "memory", "cc", "xmm0", "xmm1", "rcx" );


    return( 0 );
}



/*
 * Key expansion, 128-bit case
 */
static void aesni_setkey_enc_128( unsigned char *rk, const unsigned char *key )
{
    asm( "movdqu (%1), %%xmm0               \n\t" // copy the original key
            "movdqu %%xmm0, (%0)               \n\t" // as round key 0
            "jmp 2f                            \n\t" // skip auxiliary routine

            /*
             * Finish generating the next round key.
             *
             * On entry xmm0 is r3:r2:r1:r0 and xmm1 is X:stuff:stuff:stuff
             * with X = rot( sub( r3 ) ) ^ RCON.
             *
             * On exit, xmm0 is r7:r6:r5:r4
             * with r4 = X + r0, r5 = r4 + r1, r6 = r5 + r2, r7 = r6 + r3
             * and those are written to the round key buffer.
             */
            "1:                                \n\t"
            "pshufd $0xff, %%xmm1, %%xmm1      \n\t" // X:X:X:X
            "pxor %%xmm0, %%xmm1               \n\t" // X+r3:X+r2:X+r1:r4
            "pslldq $4, %%xmm0                 \n\t" // r2:r1:r0:0
            "pxor %%xmm0, %%xmm1               \n\t" // X+r3+r2:X+r2+r1:r5:r4
            "pslldq $4, %%xmm0                 \n\t" // etc
            "pxor %%xmm0, %%xmm1               \n\t"
            "pslldq $4, %%xmm0                 \n\t"
            "pxor %%xmm1, %%xmm0               \n\t" // update xmm0 for next time!
            "add $16, %0                       \n\t" // point to next round key
            "movdqu %%xmm0, (%0)               \n\t" // write it
            "ret                               \n\t"

            /* Main "loop" */
            "2:                                \n\t"
    AESKEYGENA xmm0_xmm1 ",0x01        \n\tcall 1b \n\t"
    AESKEYGENA xmm0_xmm1 ",0x02        \n\tcall 1b \n\t"
    AESKEYGENA xmm0_xmm1 ",0x04        \n\tcall 1b \n\t"
    AESKEYGENA xmm0_xmm1 ",0x08        \n\tcall 1b \n\t"
    AESKEYGENA xmm0_xmm1 ",0x10        \n\tcall 1b \n\t"
    AESKEYGENA xmm0_xmm1 ",0x20        \n\tcall 1b \n\t"
    AESKEYGENA xmm0_xmm1 ",0x40        \n\tcall 1b \n\t"
    AESKEYGENA xmm0_xmm1 ",0x80        \n\tcall 1b \n\t"
    AESKEYGENA xmm0_xmm1 ",0x1B        \n\tcall 1b \n\t"
    AESKEYGENA xmm0_xmm1 ",0x36        \n\tcall 1b \n\t"
    :
    : "r" (rk), "r" (key)
    : "memory", "cc", "0" );
}



/* Implementation that should never be optimized out by the compiler */
static void mbedtls_zeroize( void *v, size_t n )
{
    volatile unsigned char *p = (unsigned char*)v; while( n-- ) *p++ = 0;
}


/*
 * Multiplication by u in the Galois field of GF(2^n)
 *
 * As explained in NIST SP 800-38B, this can be computed:
 *
 *   If MSB(p) = 0, then p = (p << 1)
 *   If MSB(p) = 1, then p = (p << 1) ^ R_n
 *   with R_64 = 0x1B and  R_128 = 0x87
 *
 * Input and output MUST NOT point to the same buffer
 * Block size must be 8 bytes or 16 bytes - the block sizes for DES and AES.
 */
static int cmac_multiply_by_u( unsigned char *output, const unsigned char *input)
{
    const unsigned char R_128 = 0x87;
    unsigned char R_n, mask;
    unsigned char overflow = 0x00;
    int i;

    size_t blocksize = MBEDTLS_AES_BLOCK_SIZE;
    R_n = R_128;

    for( i = (int)blocksize - 1; i >= 0; i-- )
    {
        output[i] = input[i] << 1 | overflow;
        overflow = input[i] >> 7;
    }

    /* mask = ( input[0] >> 7 ) ? 0xff : 0x00
     * using bit operations to avoid branches */

    /* MSVC has a warning about unary minus on unsigned, but this is
     * well-defined and precisely what we want to do here */
#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4146 )
#endif
    mask = - ( input[0] >> 7 );
#if defined(_MSC_VER)
#pragma warning( pop )
#endif

    output[ blocksize - 1 ] ^= R_n & mask;

    return( 0 );
}

/*
 * Generate subkeys
 *
 * - as specified by RFC 4493, section 2.3 Subkey Generation Algorithm
 */
static int cmac_generate_subkeys( mbedtls_cipher_context_t *ctx,
                                  unsigned char* K1, unsigned char* K2 )
{
    int ret;
    unsigned char L[MBEDTLS_AES_BLOCK_SIZE];

    mbedtls_zeroize( L, sizeof( L ) );


    /* Calculate Ek(0) */
    if ( (ret = mbedtls_aesni_crypt_ecb( ctx->cipher_ctx, L,L) ) != 0)
    {

        goto exit;
    }
    /*
     * Generate K1 and K2
     */
    if( ( ret = cmac_multiply_by_u( K1, L) ) != 0 )
        goto exit;

    if( ( ret = cmac_multiply_by_u( K2, K1) ) != 0 )
        goto exit;

exit:
    mbedtls_zeroize( L, sizeof( L ) );

    return( ret );
}


static int mbedtls_cipher_setkey( mbedtls_cipher_context_t *ctx, const unsigned char *key)
{

    if( NULL == ctx)
    {
        return (MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    mbedtls_aes_context* c = (mbedtls_aes_context*)ctx->cipher_ctx;
    c->rk = c->buf;

    aesni_setkey_enc_128((unsigned char*)c->rk, key );

    return 0;

}

inline static void cmac_xor_block( unsigned char *output, const unsigned char *input1, const unsigned char *input2)
{
    size_t idx;

    for( idx = 0; idx < MBEDTLS_AES_BLOCK_SIZE; idx++ )
    {
        output[idx] = input1[idx] ^ input2[idx];
    }

}

/*
 * Create padded last block from (partial) last block.
 *
 * We can't use the padding option from the cipher layer, as it only works for
 * CBC and we use ECB mode, and anyway we need to XOR K1 or K2 in addition.
 */
inline static void cmac_pad( unsigned char padded_block[MBEDTLS_AES_BLOCK_SIZE], size_t padded_block_len, const unsigned char *last_block, size_t last_block_len )
{
    size_t j;

    for( j = 0; j < padded_block_len; j++ )
    {
        if( j < last_block_len )
        {
            padded_block[j] = last_block[j];
        }
        else if( j == last_block_len )
        {
            padded_block[j] = 0x80;
        }
        else
        {
            padded_block[j] = 0x00;
        }
    }
}


static int mbedtls_cipher_cmac_update( mbedtls_cipher_context_t *ctx, const unsigned char *input, size_t ilen )
{
    mbedtls_cmac_context_t* cmac_ctx;
    unsigned char *state;
    int ret = 0;
    size_t n, j, block_size;

    if( ctx == NULL || input == NULL || ctx->cmac_ctx == NULL )
    {
        return (MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    cmac_ctx = ctx->cmac_ctx;
    block_size = MBEDTLS_AES_BLOCK_SIZE;
    state = ctx->cmac_ctx->state;

    /* Is there data still to process from the last call, that's greater in
     * size than a block? */
    if( cmac_ctx->unprocessed_len > 0 && ilen > block_size - cmac_ctx->unprocessed_len )
    {
        memcpy( &cmac_ctx->unprocessed_block[cmac_ctx->unprocessed_len], input, block_size - cmac_ctx->unprocessed_len );

        cmac_xor_block( state, cmac_ctx->unprocessed_block, state);

        //if( ( ret = mbedtls_cipher_update( ctx, state, state) ) != 0 )
        if ( (ret = mbedtls_aesni_crypt_ecb( ctx->cipher_ctx, state, state) ) !=0 )
        {
           goto exit;
        }

        input += block_size - cmac_ctx->unprocessed_len;
        ilen -= block_size - cmac_ctx->unprocessed_len;
        cmac_ctx->unprocessed_len = 0;

    }

    /* n is the number of blocks including any final partial block */
    n = ( ilen + block_size - 1 ) / block_size;

    /* Iterate across the input data in block sized chunks, excluding any
     * final partial or complete block */
    for( j = 1; j < n; j++ )
    {
        cmac_xor_block( state, input, state);

        //if( ( ret = mbedtls_cipher_update( ctx, state, state) ) != 0 )
        if ( (ret = mbedtls_aesni_crypt_ecb( ctx->cipher_ctx, state, state ) ) != 0)
        {
            goto exit;
        }


        ilen -= block_size;
        input += block_size;
    }

    /* If there is data left over that wasn't aligned to a block */
    if( ilen > 0 )
    {
        memcpy( &cmac_ctx->unprocessed_block[cmac_ctx->unprocessed_len], input, ilen );
        cmac_ctx->unprocessed_len += ilen;
    }

exit:
    return( ret );
}

static int mbedtls_cipher_cmac_finish( mbedtls_cipher_context_t *ctx, unsigned char *output )
{
    mbedtls_cmac_context_t* cmac_ctx;
    unsigned char *state, *last_block;
    unsigned char K1[MBEDTLS_AES_BLOCK_SIZE];
    unsigned char K2[MBEDTLS_AES_BLOCK_SIZE];
    unsigned char M_last[MBEDTLS_AES_BLOCK_SIZE];
    int ret;
    size_t block_size;

    if( ctx == NULL || ctx->cmac_ctx == NULL || output == NULL )
    {
        return (MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    cmac_ctx = ctx->cmac_ctx;
    block_size = MBEDTLS_AES_BLOCK_SIZE;
    state = cmac_ctx->state;

    mbedtls_zeroize( K1, sizeof( K1 ) );
    mbedtls_zeroize( K2, sizeof( K2 ) );
    cmac_generate_subkeys( ctx, K1, K2 );

    last_block = cmac_ctx->unprocessed_block;

    /* Calculate last block */
    if( cmac_ctx->unprocessed_len < block_size )
    {
        cmac_pad( M_last, block_size, last_block, cmac_ctx->unprocessed_len );
        cmac_xor_block( M_last, M_last, K2);
    }
    else
    {
        /* Last block is complete block */
        cmac_xor_block( M_last, last_block, K1);
    }


    cmac_xor_block( state, M_last, state);

    if ( (ret = mbedtls_aesni_crypt_ecb( ctx->cipher_ctx, state, state ) ) != 0)
    {
        goto exit;
    }

    memcpy( output, state, block_size );

exit:
    /* Wipe the generated keys on the stack, and any other transients to avoid
     * side channel leakage */
    mbedtls_zeroize( K1, sizeof( K1 ) );
    mbedtls_zeroize( K2, sizeof( K2 ) );

    cmac_ctx->unprocessed_len = 0;
    mbedtls_zeroize( cmac_ctx->unprocessed_block, sizeof( cmac_ctx->unprocessed_block ) );

    mbedtls_zeroize( state, MBEDTLS_AES_BLOCK_SIZE );
    return( ret );
}


int mbedtls_cipher_cmac( const unsigned char *key, const unsigned char *input, size_t ilen, unsigned char *output )
{
    mbedtls_cipher_context_t ctx;
    int ret;

    if( key == NULL || input == NULL || output == NULL )
    {
        return (MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA);
    }

    memset( &ctx, 0, sizeof( mbedtls_cipher_context_t ) );


    mbedtls_aes_context aesctx;
    memset(&aesctx, 0x00, sizeof(mbedtls_aes_context));

    ctx.cipher_ctx = &aesctx;

    mbedtls_cmac_context_t cmacctx;
    memset(&cmacctx, 0x00, sizeof(mbedtls_cmac_context_t));
    mbedtls_zeroize( cmacctx.state, sizeof( cmacctx.state ) );

    ctx.cmac_ctx = &cmacctx;


    ret = mbedtls_cipher_setkey(&ctx, key);
    if( ret != 0 )
    {
        goto exit;
    }

    ret = mbedtls_cipher_cmac_update( &ctx, input, ilen );
    if( ret != 0 )
    {
        goto exit;

    }

    ret = mbedtls_cipher_cmac_finish( &ctx, output );

exit:

    mbedtls_zeroize(&cmacctx, sizeof( mbedtls_cmac_context_t ) );
    mbedtls_zeroize(&aesctx, sizeof(mbedtls_aes_context));
    mbedtls_zeroize(&ctx, sizeof(mbedtls_cipher_context_t) );
    return( ret );
}






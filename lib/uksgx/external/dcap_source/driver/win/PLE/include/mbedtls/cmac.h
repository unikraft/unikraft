/**
 * \file cmac.h
 *
 * \brief Cipher-based Message Authentication Code (CMAC) Mode for
 *        Authentication
 *
 *  Copyright (C) 2015-2016, ARM Limited, All Rights Reserved
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
#ifndef MBEDTLS_CMAC_H
#define MBEDTLS_CMAC_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MBEDTLS_ERR_CIPHER_BAD_INPUT_DATA                 -0x6100  /**< Bad input parameters to function. */
#define MBEDTLS_ERR_CIPHER_ALLOC_FAILED                   -0x6180  /**< Failed to allocate memory. */


#define MBEDTLS_AES_BLOCK_SIZE          16


#define AESENC      ".byte 0x66,0x0F,0x38,0xDC,"
#define AESENCLAST  ".byte 0x66,0x0F,0x38,0xDD,"
#define AESKEYGENA  ".byte 0x66,0x0F,0x3A,0xDF,"

#define xmm0_xmm1   "0xC8"
#define xmm1_xmm0   "0xC1"


typedef struct
{
    int nr;                     /*!<  number of rounds  */
    uint32_t *rk;               /*!<  AES round keys    */
    uint32_t buf[68];           /*!<  unaligned data    */
} mbedtls_aes_context;



/**
 * CMAC context (opaque struct).
 */
typedef struct mbedtls_cmac_context_t mbedtls_cmac_context_t;


/**
 * Generic cipher context.
 */
typedef struct {
    /** Buffer for data that hasn't been encrypted yet */
    unsigned char unprocessed_data[MBEDTLS_AES_BLOCK_SIZE];

    /** Number of bytes that still need processing */
    size_t unprocessed_len;

    /** Cipher-specific context */
    void *cipher_ctx;

    /** CMAC Specific context */
    mbedtls_cmac_context_t *cmac_ctx;

} mbedtls_cipher_context_t;


/**
 * CMAC context structure - Contains internal state information only
 */
struct mbedtls_cmac_context_t
{
    /** Internal state of the CMAC algorithm  */
    unsigned char       state[MBEDTLS_AES_BLOCK_SIZE];

    /** Unprocessed data - either data that was not block aligned and is still
     *  pending to be processed, or the final block */
    unsigned char       unprocessed_block[MBEDTLS_AES_BLOCK_SIZE];

    /** Length of data pending to be processed */
    size_t              unprocessed_len;
};


/**
 * \brief               Output = Generic_CMAC( cmac key, input buffer )
 *
 * \param key           CMAC key
 * \param input         buffer holding the  data
 * \param ilen          length of the input data
 * \param output        Generic CMAC-result
 *
 * \returns             0 on success, MBEDTLS_ERR_MD_BAD_INPUT_DATA if parameter
 *                      verification fails.
 */
int mbedtls_cipher_cmac( const unsigned char *key, const unsigned char *input, size_t ilen, unsigned char *output );



#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_CMAC_H */

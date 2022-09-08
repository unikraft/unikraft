/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


 
/**
* File: sgx_tcrypto.h
* Description:
*     Interface for generic crypto library APIs required in SDK implementation.
*/

#ifndef _SGX_TCRYPTO_H_
#define _SGX_TCRYPTO_H_

#include "sgx.h"
#include "sgx_defs.h"
#include "stdlib.h"

#define SGX_SHA1_HASH_SIZE              20
#define SGX_SHA256_HASH_SIZE            32
#define SGX_SHA384_HASH_SIZE            48
#define SGX_ECP256_KEY_SIZE             32
#define SGX_NISTP_ECP256_KEY_SIZE       (SGX_ECP256_KEY_SIZE/sizeof(uint32_t))
#define SGX_AESGCM_IV_SIZE              12
#define SGX_AESGCM_KEY_SIZE             16
#define SGX_AESGCM_MAC_SIZE             16
#define SGX_HMAC256_KEY_SIZE            32
#define SGX_HMAC256_MAC_SIZE            32
#define SGX_CMAC_KEY_SIZE               16
#define SGX_CMAC_MAC_SIZE               16
#define SGX_AESCTR_KEY_SIZE             16
#define SGX_RSA3072_KEY_SIZE            384
#define SGX_RSA3072_PRI_EXP_SIZE        384
#define SGX_RSA3072_PUB_EXP_SIZE        4

typedef struct _sgx_ec256_dh_shared_t
{
    uint8_t s[SGX_ECP256_KEY_SIZE];
} sgx_ec256_dh_shared_t;

typedef struct _sgx_ec256_private_t
{
    uint8_t r[SGX_ECP256_KEY_SIZE];
} sgx_ec256_private_t;

typedef struct _sgx_ec256_public_t
{
    uint8_t gx[SGX_ECP256_KEY_SIZE];
    uint8_t gy[SGX_ECP256_KEY_SIZE];
} sgx_ec256_public_t;

typedef struct _sgx_ec256_signature_t
{
    uint32_t x[SGX_NISTP_ECP256_KEY_SIZE];
    uint32_t y[SGX_NISTP_ECP256_KEY_SIZE];
} sgx_ec256_signature_t;

typedef struct _sgx_rsa3072_public_key_t
{
    uint8_t mod[SGX_RSA3072_KEY_SIZE];
    uint8_t exp[SGX_RSA3072_PUB_EXP_SIZE];
} sgx_rsa3072_public_key_t;

typedef struct _sgx_rsa3072_key_t
{
    uint8_t mod[SGX_RSA3072_KEY_SIZE];
    uint8_t d[SGX_RSA3072_PRI_EXP_SIZE];
    uint8_t e[SGX_RSA3072_PUB_EXP_SIZE];
} sgx_rsa3072_key_t;

typedef uint8_t sgx_rsa3072_signature_t[SGX_RSA3072_KEY_SIZE];

typedef void* sgx_sha_state_handle_t;
typedef void* sgx_hmac_state_handle_t;
typedef void* sgx_cmac_state_handle_t;
typedef void* sgx_ecc_state_handle_t;
typedef void* sgx_aes_state_handle_t;

typedef uint8_t sgx_sha1_hash_t[SGX_SHA1_HASH_SIZE];
typedef uint8_t sgx_sha256_hash_t[SGX_SHA256_HASH_SIZE];
typedef uint8_t sgx_sha384_hash_t[SGX_SHA384_HASH_SIZE];

typedef uint8_t sgx_aes_gcm_128bit_key_t[SGX_AESGCM_KEY_SIZE];
typedef uint8_t sgx_aes_gcm_128bit_tag_t[SGX_AESGCM_MAC_SIZE];
typedef uint8_t sgx_hmac_256bit_key_t[SGX_HMAC256_KEY_SIZE];
typedef uint8_t sgx_hmac_256bit_tag_t[SGX_HMAC256_MAC_SIZE];
typedef uint8_t sgx_cmac_128bit_key_t[SGX_CMAC_KEY_SIZE];
typedef uint8_t sgx_cmac_128bit_tag_t[SGX_CMAC_MAC_SIZE];
typedef uint8_t sgx_aes_ctr_128bit_key_t[SGX_AESCTR_KEY_SIZE];

typedef enum {
    SGX_EC_VALID,               /* validation pass successfully     */

    SGX_EC_COMPOSITE_BASE,      /* field based on composite         */
    SGX_EC_COMPLICATED_BASE,    /* number of non-zero terms in the polynomial (> PRIME_ARR_MAX) */
    SGX_EC_IS_ZERO_DISCRIMINANT,/* zero discriminant */
    SGX_EC_COMPOSITE_ORDER,     /* composite order of base point    */
    SGX_EC_INVALID_ORDER,       /* invalid base point order         */
    SGX_EC_IS_WEAK_MOV,         /* weak Meneze-Okamoto-Vanstone  reduction attack */
    SGX_EC_IS_WEAK_SSA,         /* weak Semaev-Smart,Satoh-Araki reduction attack */
    SGX_EC_IS_SUPER_SINGULAR,   /* supersingular curve */

    SGX_EC_INVALID_PRIVATE_KEY, /* !(0 < Private < order) */
    SGX_EC_INVALID_PUBLIC_KEY,  /* (order*PublicKey != Infinity)    */
    SGX_EC_INVALID_KEY_PAIR,    /* (Private*BasePoint != PublicKey) */

    SGX_EC_POINT_OUT_OF_GROUP,  /* out of group (order*P != Infinity)  */
    SGX_EC_POINT_IS_AT_INFINITY,/* point (P=(Px,Py)) at Infinity  */
    SGX_EC_POINT_IS_NOT_VALID,  /* point (P=(Px,Py)) out-of EC    */

    SGX_EC_POINT_IS_EQUAL,      /* compared points are equal     */
    SGX_EC_POINT_IS_NOT_EQUAL,  /* compared points are different  */

    SGX_EC_INVALID_SIGNATURE    /* invalid signature */
} sgx_generic_ecresult_t;


typedef enum {
	SGX_RSA_VALID,               /* validation pass successfully     */

	SGX_RSA_INVALID_SIGNATURE    /* invalid signature */
} sgx_rsa_result_t;

typedef enum {
    SGX_RSA_PRIVATE_KEY,               /* RSA private key state     */

    SGX_RSA_PUBLIC_KEY    /* RSA public key state */
} sgx_rsa_key_type_t;

#define N_SIZE_IN_BYTES    384
#define E_SIZE_IN_BYTES    4
#define D_SIZE_IN_BYTES    384
#define P_SIZE_IN_BYTES    192
#define Q_SIZE_IN_BYTES    192
#define DMP1_SIZE_IN_BYTES 192
#define DMQ1_SIZE_IN_BYTES 192
#define IQMP_SIZE_IN_BYTES 192

#define N_SIZE_IN_UINT     (N_SIZE_IN_BYTES/sizeof(unsigned int))
#define E_SIZE_IN_UINT     (E_SIZE_IN_BYTES/sizeof(unsigned int))
#define D_SIZE_IN_UINT     (D_SIZE_IN_BYTES/sizeof(unsigned int))
#define P_SIZE_IN_UINT     (P_SIZE_IN_BYTES/sizeof(unsigned int))
#define Q_SIZE_IN_UINT     (Q_SIZE_IN_BYTES/sizeof(unsigned int))
#define DMP1_SIZE_IN_UINT  (DMP1_SIZE_IN_BYTES/sizeof(unsigned int))
#define DMQ1_SIZE_IN_UINT  (DMQ1_SIZE_IN_BYTES/sizeof(unsigned int))
#define IQMP_SIZE_IN_UINT  (IQMP_SIZE_IN_BYTES/sizeof(unsigned int))

typedef struct _rsa_params_t {
	unsigned int n[N_SIZE_IN_UINT];
	unsigned int e[E_SIZE_IN_UINT];
	unsigned int d[D_SIZE_IN_UINT];
	unsigned int p[P_SIZE_IN_UINT];
	unsigned int q[Q_SIZE_IN_UINT];
	unsigned int dmp1[DMP1_SIZE_IN_UINT];
	unsigned int dmq1[DMQ1_SIZE_IN_UINT];
	unsigned int iqmp[IQMP_SIZE_IN_UINT];
}rsa_params_t;

#ifdef __cplusplus
extern "C" {
#endif

   /** SHA Hashing functions - NOTE: ONLY 256/160-bit is supported.
    *
    * NOTE: Use sgx_sha#_msg if the src pointer contains the complete msg to perform hash (Option 1)
    *       Else use the Init, Update, Update, ..., Final procedure (Option 2)
    * Option 1: If the complete dataset is available for hashing, sgx_sha#_msg
    *           is a single API call for generating the 256/160-bit hash for the given dataset.
    *      Return: If source pointer or hash pointer are NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *              If hash function fails then SGX_ERROR_UNEXPECTED is returned.
    * Option 2: If the hash is to be performed over multiple data sets, then use:
    *        A. sgx_sha#_init - to create the context - context memory is allocated by this function.
    *      Return: If out of enclave memory, SGX_ERROR_OUT_OF_MEMORY is returned.
    *              If context creation fails then SGX_ERROR_UNEXPECTED is returned.
    *        B. sgx_sha#_update - updates hash based on input source data
    *                 This function should be called for each chunk of data to be
    *                 included in the hash including the 1st and final chunks.
    *      Return: If source pointer or context pointer are NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *              If hash function fails then SGX_ERROR_UNEXPECTED is returned.
    *        C. sgx_sha#_get_hash - function obtains the hash value
    *      Return: If hash pointer or context pointer are NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *              If the function fails then SGX_ERROR_UNEXPECTED is returned.
    *        D. sgx_sha#_close - SHOULD BE CALLED to FREE context memory
    *              Upon completing the process of computing a hash over a set of data
    *              or sets of data, this function is used to free the context.
    *      Return: If context pointer is NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: uint8_t *p_src - Pointer to the input stream to be hashed
    *           uint32_t src_len - Length of the input stream to be hashed
    *   Output: sgx_sha#_hash_t *p_hash - Resultant hash from operation
   */
    sgx_status_t SGXAPI sgx_sha384_msg(const uint8_t *p_src, uint32_t src_len, sgx_sha384_hash_t *p_hash);
    sgx_status_t SGXAPI sgx_sha256_msg(const uint8_t *p_src, uint32_t src_len, sgx_sha256_hash_t *p_hash);
    sgx_status_t SGXAPI sgx_sha1_msg(const uint8_t *p_src, uint32_t src_len, sgx_sha1_hash_t *p_hash);

   /** Allocates and initializes sha state
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Output: sgx_sha_state_handle_t *p_sha_handle - Pointer to the handle of the SHA state
   */
    sgx_status_t SGXAPI sgx_sha384_init(sgx_sha_state_handle_t* p_sha_handle);
    sgx_status_t SGXAPI sgx_sha256_init(sgx_sha_state_handle_t* p_sha_handle);
    sgx_status_t SGXAPI sgx_sha1_init(sgx_sha_state_handle_t* p_sha_handle);

   /** Updates sha calculation based on the input message
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Input:  sgx_sha_state_handle_t sha_handle - Handle to the SHA state
    *           uint8_t *p_src - Pointer to the input stream to be hashed
    *           uint32_t src_len - Length of the input stream to be hashed
    */
    sgx_status_t SGXAPI sgx_sha384_update(const uint8_t *p_src, uint32_t src_len, sgx_sha_state_handle_t sha_handle);
    sgx_status_t SGXAPI sgx_sha256_update(const uint8_t *p_src, uint32_t src_len, sgx_sha_state_handle_t sha_handle);
    sgx_status_t SGXAPI sgx_sha1_update(const uint8_t *p_src, size_t src_len, sgx_sha_state_handle_t sha_handle);

   /** Returns Hash calculation
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Input:  sgx_sha_state_handle_t sha_handle - Handle to the SHA state
    *   Output: sgx_sha#_hash_t *p_hash - Resultant hash from operation
    */
    sgx_status_t SGXAPI sgx_sha384_get_hash(sgx_sha_state_handle_t sha_handle, sgx_sha384_hash_t *p_hash);
    sgx_status_t SGXAPI sgx_sha256_get_hash(sgx_sha_state_handle_t sha_handle, sgx_sha256_hash_t *p_hash);
    sgx_status_t SGXAPI sgx_sha1_get_hash(sgx_sha_state_handle_t sha_handle, sgx_sha1_hash_t *p_hash);

   /** Cleans up SHA state
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Input:  sgx_sha_state_handle_t sha_handle - Handle to the SHA state
    */
    sgx_status_t SGXAPI sgx_sha384_close(sgx_sha_state_handle_t sha_handle);
    sgx_status_t SGXAPI sgx_sha256_close(sgx_sha_state_handle_t sha_handle);
    sgx_status_t SGXAPI sgx_sha1_close(sgx_sha_state_handle_t sha_handle);

   /**Rijndael AES-GCM - Only 128-bit key AES-GCM Encryption/Decryption is supported
    *
    * The Galois/Counter Mode (GCM) is a mode of operation of the AES algorithm.
    * GCM [NIST SP 800-38D] uses a variation of the Counter mode of operation for encryption.
    * GCM assures authenticity of the confidential data (of up to about 64 GB per invocation)
    * using a universal hash function defined over a binary finite field (the Galois field).
    *
    * GCM can also provide authentication assurance for additional data
    * (of practically unlimited length per invocation) that is not encrypted.
    * GCM provides stronger authentication assurance than a (non-cryptographic) checksum or
    * error detecting code. In particular, GCM can detect both accidental modifications of
    * the data and intentional, unauthorized modifications.
    *
    * sgx_rijndael128GCM_encrypt:
    * Return: If key, source, destination, MAC, or IV pointer is NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *         If AAD size is > 0 and the AAD pointer is NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the Source Length is < 1, SGX_ERROR_INVALID_PARAMETER is returned.
    *         IV Length must = 12 (bytes) or SGX_ERROR_INVALID_PARAMETER is returned.
    *         If out of enclave memory then SGX_ERROR_OUT_OF_MEMORY is returned.
    *         If the encryption process fails then SGX_ERROR_UNEXPECTED is returned.
    *
    * sgx_rijndael128GCM_decrypt:
    * Return: If key, source, destination, MAC, or IV pointer is NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *         If AAD size is > 0 and the AAD pointer is NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the Source Length is < 1, SGX_ERROR_INVALID_PARAMETER is returned.
    *         IV Length must = 12 (bytes) or SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the decryption process fails then SGX_ERROR_UNEXPECTED is returned.
    *         If the input MAC does not match the calculated MAC, SGX_ERROR_MAC_MISMATCH is returned.
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: sgx_aes_gcm_128bit_key_t *p_key - Pointer to the key used in encryption/decryption operation
    *                                             Size MUST BE 128-bits
    *           uint8_t *p_src - Pointer to the input stream to be encrypted/decrypted
    *           uint32_t src_len - Length of the input stream to be encrypted/decrypted
    *           uint8_t *p_iv - Pointer to the initialization vector
    *           uint32_t iv_len - Length of the initialization vector - MUST BE 12 (bytes)
    *                             NIST AES-GCM recommended IV size = 96 bits
    *           uint8_t *p_aad - Pointer to the input stream of additional authentication data
    *           uint32_t aad_len - Length of the additional authentication data stream
    *           sgx_aes_gcm_128bit_tag_t *p_in_mac - Pointer to the expected MAC in decryption process
    *   Output: uint8_t *p_dst - Pointer to the cipher text for encryption or clear text for decryption. Size of buffer should be >= src_len.
    *           sgx_aes_gcm_128bit_tag_t *p_out_mac - Pointer to the MAC generated from encryption process
    * NOTE: Wrapper is responsible for confirming decryption tag matches encryption tag
    */
    sgx_status_t SGXAPI sgx_rijndael128GCM_encrypt(const sgx_aes_gcm_128bit_key_t *p_key,
                                                const uint8_t *p_src,
                                                uint32_t src_len,
                                                uint8_t *p_dst,
                                                const uint8_t *p_iv,
                                                uint32_t iv_len,
                                                const uint8_t *p_aad,
                                                uint32_t aad_len,
                                                sgx_aes_gcm_128bit_tag_t *p_out_mac);
    sgx_status_t SGXAPI sgx_rijndael128GCM_decrypt(const sgx_aes_gcm_128bit_key_t *p_key,
                                                const uint8_t *p_src,
                                                uint32_t src_len,
                                                uint8_t *p_dst,
                                                const uint8_t *p_iv,
                                                uint32_t iv_len,
                                                const uint8_t *p_aad,
                                                uint32_t aad_len,
                                                const sgx_aes_gcm_128bit_tag_t *p_in_mac);

   /** Message Authentication Rijndael 128 CMAC - Only 128-bit key size is supported.
    * NOTE: Use sgx_rijndael128_cmac_msg if the src ptr contains the complete msg to perform hash (Option 1)
    *       Else use the Init, Update, Update, ..., Final, Close procedure (Option 2)
    * Option 1: If the complete dataset is available for hashing, sgx_rijndael128_cmac_msg
    *           is a single API call for generating the 128-bit hash for the given dataset.
    *      Return: If source, key, or MAC pointer is NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *              If out of enclave memory, SGX_ERROR_OUT_OF_MEMORY is returned.
    *              If hash function fails then SGX_ERROR_UNEXPECTED is returned.
    * Option 2: If the hash is to be performed over multiple data sets, then use:
    *        A. sgx_cmac128_init - to create the context - context memory is allocated by this function.
    *      Return: If key pointer is NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *              If out of enclave memory, SGX_ERROR_OUT_OF_MEMORY is returned.
    *              If context creation fails then SGX_ERROR_UNEXPECTED is returned.
    *        B. sgx_cmac128_update - updates hash based on input source data
    *                 This function should be called for each chunk of data to be
    *                 included in the hash including the 1st and final chunks.
    *      Return: If source pointer or context pointer are NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *              If hash function fails then SGX_ERROR_UNEXPECTED is returned.
    *        C. sgx_cmac128_final - function obtains the hash value
    *              Upon completing the process of computing a hash over a set of data or sets of data,
    *              this function populates the hash value.
    *      Return: If hash pointer or context pointer are NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *              If the function fails then SGX_ERROR_UNEXPECTED is returned.
    *        D. sgx_cmac128_close - SHOULD BE CALLED to clean up the CMAC state
    *              Upon populating the hash value over a set of data or sets of data,
    *              this function is used to free the CMAC state.
    *      Return: If CMAC state pointer is NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: sgx_cmac_128bit_key_t *p_key - Pointer to the key used in encryption/decryption operation
    *           uint8_t *p_src - Pointer to the input stream to be MAC'd
    *           uint32_t src_len - Length of the input stream to be MAC'd
    *   Output: sgx_cmac_gcm_128bit_tag_t *p_mac - Pointer to the resultant MAC
    */
    sgx_status_t SGXAPI sgx_rijndael128_cmac_msg(const sgx_cmac_128bit_key_t *p_key,
                                                    const uint8_t *p_src,
                                                    uint32_t src_len,
                                                    sgx_cmac_128bit_tag_t *p_mac);
   /** Allocates and initializes CMAC state.
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: sgx_cmac_128bit_key_t *p_key - Pointer to the key used in encryption/decryption operation
    *   Output: sgx_cmac_state_handle_t *p_cmac_handle - Pointer to the handle of the CMAC state
    */
    sgx_status_t SGXAPI sgx_cmac128_init(const sgx_cmac_128bit_key_t *p_key, sgx_cmac_state_handle_t* p_cmac_handle);

   /** Updates CMAC has calculation based on the input message.
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Input:  sgx_cmac_state_handle_t cmac_handle - Handle to the CMAC state
    *           uint8_t *p_src - Pointer to the input stream to be hashed
    *           uint32_t src_len - Length of the input stream to be hashed
    */
    sgx_status_t SGXAPI sgx_cmac128_update(const uint8_t *p_src, uint32_t src_len, sgx_cmac_state_handle_t cmac_handle);

   /** Returns Hash calculation and clean up CMAC state.
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Input:  sgx_cmac_state_handle_t cmac_handle - Handle to the CMAC state
    *   Output: sgx_cmac_128bit_tag_t *p_hash - Resultant hash from operation
    */
    sgx_status_t SGXAPI sgx_cmac128_final(sgx_cmac_state_handle_t cmac_handle, sgx_cmac_128bit_tag_t *p_hash);

   /** Clean up the CMAC state
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Input: sgx_cmac_state_handle_t cmac_handle  - Handle to the CMAC state
    */
    sgx_status_t SGXAPI sgx_cmac128_close(sgx_cmac_state_handle_t cmac_handle);


    /* Message Authentication - HMAC 256
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined sgx_error.h
    *   Inputs: const unsigned char *p_src - Pointer to input stream to be MACed
    *           int src_len - Source length
    *           const unsigned char *p_key - Pointer to key used in message authentication operation
    *           int key_len - Key length
    *           int mac_len - Expected output MAC length
    *   Output: unsigned char *p_mac - Pointer to resultant MAC
    */
    sgx_status_t SGXAPI sgx_hmac_sha256_msg(const unsigned char *p_src, int src_len, const unsigned char *p_key, int key_len,
        unsigned char *p_mac, int mac_len);

    /* Allocates and initializes HMAC state
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: const unsigned char *p_key - Pointer to the key used in message authentication operation
    *           int key_len - Key length
    *   Output: sgx_hmac_state_handle_t *p_hmac_handle - Pointer to the initialized HMAC state handle
    */
    sgx_status_t SGXAPI sgx_hmac256_init(const unsigned char *p_key, int key_len, sgx_hmac_state_handle_t *p_hmac_handle);

    /* Updates HMAC hash calculation based on the input message
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.
    *	Input:  uint8_t *p_src - Pointer to the input stream to be hashed
    *	        int src_len - Length of input stream to be hashed
    *	        sgx_hmac_state_handle_t hmac_handle - Handle to the HMAC state
    */
    sgx_status_t SGXAPI sgx_hmac256_update(const uint8_t *p_src, int src_len, sgx_hmac_state_handle_t hmac_handle);

    /* Returns calculated hash
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *	Input:  sgx_hmac_state_handle_t hmac_handle - Handle to the HMAC state
    *	        int hash_len - Expected MAC length
    *   Output: unsigned char *p_hash - Resultant hash from HMAC operation
    */
    sgx_status_t SGXAPI sgx_hmac256_final(unsigned char *p_hash, int hash_len, sgx_hmac_state_handle_t hmac_handle);

    /* Clean up and free the HMAC state
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS
    *   Input:  sgx_hmac_state_handle_t hmac_handle  - Handle to the HMAC state
    */
    sgx_status_t SGXAPI sgx_hmac256_close(sgx_hmac_state_handle_t hmac_handle);


   /** AES-CTR 128-bit - Only 128-bit key size is supported.
    *
    * These functions encrypt/decrypt the input data stream of a variable length according
    * to the CTR mode as specified in [NIST SP 800-38A].  The counter can be thought of as
    * an IV which increments on successive encryption or decryption calls. For a given
    * dataset or data stream the incremented counter block should be used on successive
    * calls of the encryption/decryption process for that given stream.  However for
    * new or different datasets/streams, the same counter should not be reused, instead
    * initialize the counter for the new data set.
    * Note: SGXSSL based version doesn't support user given ctr_inc_bits. It use OpenSSL's implementation
    * which divide the counter block into two parts ([IV][counter])
    *
    * sgx_aes_ctr_encrypt
    *      Return: If source, key, counter, or destination pointer is NULL,
    *                            SGX_ERROR_INVALID_PARAMETER is returned.
    *              If out of enclave memory, SGX_ERROR_OUT_OF_MEMORY is returned.
    *              If the encryption process fails then SGX_ERROR_UNEXPECTED is returned.
    * sgx_aes_ctr_decrypt
    *      Return: If source, key, counter, or destination pointer is NULL,
    *                            SGX_ERROR_INVALID_PARAMETER is returned.
    *              If out of enclave memory, SGX_ERROR_OUT_OF_MEMORY is returned.
    *              If the decryption process fails then SGX_ERROR_UNEXPECTED is returned.
    *
    * Parameters:
    *   Return:
    *     sgx_status_t - SGX_SUCCESS or failure as defined
    *                    in sgx_error.h
    *   Inputs:
    *     sgx_aes_128bit_key_t *p_key - Pointer to the key used in
    *                                   encryption/decryption operation
    *     uint8_t *p_src - Pointer to the input stream to be
    *                      encrypted/decrypted
    *     uint32_t src_len - Length of the input stream to be
    *                        encrypted/decrypted
    *     uint8_t *p_ctr - Pointer to the counter block
    *     uint32_t ctr_inc_bits - Number of bits in counter to be
    *                             incremented
    *   Output:
    *     uint8_t *p_dst - Pointer to the cipher text.
    *                      Size of buffer should be >= src_len.
    */
    sgx_status_t SGXAPI sgx_aes_ctr_encrypt(
                        const sgx_aes_ctr_128bit_key_t *p_key,
                        const uint8_t *p_src,
                        const uint32_t src_len,
                        uint8_t *p_ctr,
                        const uint32_t ctr_inc_bits,
                        uint8_t *p_dst);

    sgx_status_t SGXAPI sgx_aes_ctr_decrypt(
                        const sgx_aes_ctr_128bit_key_t *p_key,
                        const uint8_t *p_src,
                        const uint32_t src_len,
                        uint8_t *p_ctr,
                        const uint32_t ctr_inc_bits,
                        uint8_t *p_dst);



   /**
    * Elliptic Curve Cryptography based on GF(p), 256 bit.
    *
    * Elliptic curve cryptosystems (ECCs) implement a different way of creating public keys.
    * Because elliptic curve calculation is based on the addition of the rational points in
    * the (x,y) plane and it is difficult to solve a discrete logarithm from these points,
    * a higher level of security is achieved through the cryptographic schemes that use the
    * elliptic curves. The cryptographic systems that encrypt messages by using the properties
    * of elliptic curves are hard to attack due to the extreme complexity of deciphering the
    * private key.
    *
    * Use of elliptic curves allows for shorter public key length and encourage cryptographers
    * to create cryptosystems with the same or higher encryption strength as the RSA or DSA
    * cryptosystems. Because of the relatively short key length, ECCs do encryption and decryption
    * faster on the hardware that requires less computation processing volumes. For example, with
    * a key length of 150-350 bits, ECCs provide the same encryption strength as the cryptosystems
    * who have to use 600 -1400 bits.
    *
    * ECCP stands for Elliptic Curve Cryptography Prime and these functions include operations
    * over a prime finite field GF(p).
    *
    */
   /** Allocates and initializes ecc context.
    * The function initializes the context of the elliptic curve cryptosystem over the
    * prime finite field GF(p).  This function allocates and initializes the ecc context.
    *      Return: If out of enclave memory, SGX_ERROR_OUT_OF_MEMORY is returned.
    *              If context creation fails then SGX_ERROR_UNEXPECTED is returned.
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Output: sgx_ecc_state_handle_t *p_ecc_handle - Pointer to the handle of the ECC crypto system
    */
    sgx_status_t SGXAPI sgx_ecc256_open_context(sgx_ecc_state_handle_t* p_ecc_handle);

   /** Cleans up ecc context.
    *      Return: If context pointer is NULL, SGX_ERROR_INVALID_PARAMETER is returned.
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Output: sgx_ecc_state_handle_t ecc_handle - Handle to the ECC crypto system
    */
    sgx_status_t SGXAPI sgx_ecc256_close_context(sgx_ecc_state_handle_t ecc_handle);

   /** Populates private/public key pair.
    * NOTE: Caller code allocates memory for Private & Public key pointers to be populated
    *
    * The function generates a private key p_private and computes a public key p_public of the
    * elliptic cryptosystem over a finite field GF(p).
    *
    * The private key p_private is a number that lies in the range of [1, n-1] where n is
    * the order of the elliptic curve base point.
    *
    * The public key p_public is an elliptic curve point such that p_public = p_private *G,
    * where G is the base point of the elliptic curve.
    *
    * The context of the point p_public as an elliptic curve point must be created by using
    * the function sgx_ecc256_open_context.
    *
    * Return: If context, public key, or private key pointer is NULL,
    *                            SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the key creation process fails then SGX_ERROR_UNEXPECTED is returned.
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to the ECC crypto system
    *   Outputs: sgx_ec256_private_t *p_private - Pointer to the private key - LITTLE ENDIAN
    *            sgx_ec256_public_t *p_public - Pointer to the public key - LITTLE ENDIAN
    */
    sgx_status_t SGXAPI sgx_ecc256_create_key_pair(sgx_ec256_private_t *p_private,
                                                sgx_ec256_public_t *p_public,
                                                sgx_ecc_state_handle_t ecc_handle);


    /** Checks whether the input point is a valid point on the given elliptic curve.
     * Parameters:
     *  Return: sgx_status_t - SGX_SUCCESS or failure as defined sgx_error.h
     *  Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to ECC crypto system
     *          sgx_ec256_public_t *p_point - Pointer to perform validity check on - LITTLE ENDIAN
     *  Output: int *p_valid - Return 0 if the point is an invalid point on ECC curve
     */
    sgx_status_t SGXAPI sgx_ecc256_check_point(const sgx_ec256_public_t *p_point,
                                    const sgx_ecc_state_handle_t ecc_handle,
                                    int *p_valid);


   /** Computes DH shared key based on own (local) private key and remote public Ga Key.
    * NOTE: Caller code allocates memory for Shared key pointer to be populated
    *
    * The function computes a secret number bnShare, which is a secret key shared between
    * two participants of the cryptosystem.
    *
    * In cryptography, metasyntactic names such as Alice as Bob are normally used as examples
    * and in discussions and stand for participant A and participant B.
    *
    * Both participants (Alice and Bob) use the cryptosystem for receiving a common secret point
    * on the elliptic curve called a secret key. To receive a secret key, participants apply the
    * Diffie-Hellman key-agreement scheme involving public key exchange. The value of the secret
    * key entirely depends on participants.
    *
    * According to the scheme, Alice and Bob perform the following operations:
    * 1. Alice calculates her own public key pubKeyA by using her private key
    *    privKeyA: pubKeyA = privKeyA *G, where G is the base point of the elliptic curve.
    * 2. Alice passes the public key to Bob.
    * 3. Bob calculates his own public key pubKeyB by using his private key
    *    privKeyB: pubKeyB = privKeyB *G, where G is a base point of the elliptic curve.
    * 4. Bob passes the public key to Alice.
    * 5. Alice gets Bob's public key and calculates the secret point shareA. When calculating,
    *    she uses her own private key and Bob's public key and applies the following formula:
    *    shareA = privKeyA *pubKeyB = privKeyA *privKeyB *G.
    * 6. Bob gets Alice's public key and calculates the secret point shareB. When calculating,
    *    he uses his own private key and Alice's public key and applies the following formula:
    *    shareB = privKeyB *pubKeyA = privKeyB *privKeyA *G.
    *
    * Because the following equation is true privKeyA *privKeyB *G = privKeyB *privKeyA *G,
    * the result of both calculations is the same, that is, the equation shareA = shareB is true.
    * The secret point serves as a secret key.
    *
    * Shared secret bnShare is an x-coordinate of the secret point on the elliptic curve. The elliptic
    * curve domain parameters must be hitherto defined by the function: sgx_ecc256_open_context.
    *
    * Return: If context, public key, private key, or shared key pointer is NULL,
    *                            SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the remote public key is not a valid point on the elliptic curve,
    *                            SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the key creation process fails then SGX_ERROR_UNEXPECTED is returned.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to the ECC crypto system
    *           sgx_ec256_private_t *p_private_b - Pointer to the local private key - LITTLE ENDIAN
    *           sgx_ec256_public_t *p_public_ga - Pointer to the remote public key - LITTLE ENDIAN
    *   Output: sgx_ec256_dh_shared_t *p_shared_key - Pointer to the shared DH key - LITTLE ENDIAN
    */
    sgx_status_t SGXAPI sgx_ecc256_compute_shared_dhkey(const sgx_ec256_private_t *p_private_b,
                                                    const sgx_ec256_public_t *p_public_ga,
                                                    sgx_ec256_dh_shared_t *p_shared_key,
                                                    sgx_ecc_state_handle_t ecc_handle);

   
    /** Computes signature for data based on private key.
    *
    * A message digest is a fixed size number derived from the original message with
    * an applied hash function over the binary code of the message. (SHA256 in this case)
    * The signer's private key and the message digest are used to create a signature.
    *
    * A digital signature over a message consists of a pair of large numbers, 256-bits each,
    * which the given function computes.
    *
    * The scheme used for computing a digital signature is of the ECDSA scheme,
    * an elliptic curve of the DSA scheme.
    *
    * The keys can be generated and set up by the function: sgx_ecc256_create_key_pair.
    *
    * The elliptic curve domain parameters must be created by function:
    *     sgx_ecc256_open_context
    *
    * Return: If context, private key, signature or data pointer is NULL,
    *                       SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the signature creation process fails then SGX_ERROR_UNEXPECTED is returned.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to the ECC crypto system
    *           sgx_ec256_private_t *p_private - Pointer to the private key - LITTLE ENDIAN
    *           uint8_t *p_data - Pointer to the data to be signed
    *           uint32_t data_size - Size of the data to be signed
    *   Output: ec256_signature_t *p_signature - Pointer to the signature - LITTLE ENDIAN
    */
    sgx_status_t SGXAPI sgx_ecdsa_sign(const uint8_t *p_data,
                                    uint32_t data_size,
                                    const sgx_ec256_private_t *p_private,
                                    sgx_ec256_signature_t *p_signature,
                                    sgx_ecc_state_handle_t ecc_handle);

   /** Verifies the signature for the given data based on the public key.
    * This API verifies the hash of input data `verify(SHA256(p_data))`. First it'll calculate SHA256 hash for given data
    * and then verify the signature for this hash.
    *
    * A digital signature over a message consists of a pair of large numbers, 256-bits each,
    * which could be created by function: sgx_ecdsa_sign. The scheme used for computing a
    * digital signature is of the ECDSA scheme, an elliptic curve of the DSA scheme.
    *
    * The typical result of the digital signature verification is one of the two values:
    *     SGX_Generic_ECValid - Digital signature is valid
    *     SGX_Generic_ECInvalidSignature -  Digital signature is not valid
    *
    * The elliptic curve domain parameters must be created by function:
    *     sgx_ecc256_open_context
    *
    * Return: If context, public key, signature, result or data pointer is NULL,
    *                    SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the verification process fails then SGX_ERROR_UNEXPECTED is returned.
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to the ECC crypto system
    *           sgx_ec256_public_t *p_public - Pointer to the public key
    *           uint8_t *p_data - Pointer to the data to be verified
    *           uint32_t data_size - Size of the data to be verified
    *           sgx_ec256_signature_t *p_signature - Pointer to the signature
    *   Output: uint8_t *p_result - Pointer to the result of verification check
    */
    sgx_status_t SGXAPI sgx_ecdsa_verify(const uint8_t *p_data,
                                        uint32_t data_size,
                                        const sgx_ec256_public_t *p_public,
                                        const sgx_ec256_signature_t *p_signature,
                                        uint8_t *p_result,
                                        sgx_ecc_state_handle_t ecc_handle);

   /** Directly verify the signature for the given data based on the public key. (Without calculating data's hash) see sgx_ecdsa_verify.
    *
    * A digital signature over a message consists of a pair of large numbers, 256-bits each,
    * which could be created by function: sgx_ecdsa_sign. The scheme used for computing a
    * digital signature is of the ECDSA scheme, an elliptic curve of the DSA scheme.
    *
    * The typical result of the digital signature verification is one of the two values:
    *     SGX_Generic_ECValid - Digital signature is valid
    *     SGX_Generic_ECInvalidSignature -  Digital signature is not valid
    *
    * The elliptic curve domain parameters must be created by function:
    *     sgx_ecc256_open_context
    *
    * Return: If context, public key, signature, result or data pointer is NULL,
    *                    SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the verification process fails then SGX_ERROR_UNEXPECTED is returned.
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to the ECC crypto system
    *           sgx_ec256_public_t *p_public - Pointer to the public key
    *           uint8_t *p_data - Pointer to the data to be verified
    *           sgx_ec256_signature_t *p_signature - Pointer to the signature
    *   Output: uint8_t *p_result - Pointer to the result of verification check
    */
    sgx_status_t SGXAPI sgx_ecdsa_verify_hash(const uint8_t *p_data,
                                        const sgx_ec256_public_t *p_public,
                                        const sgx_ec256_signature_t *p_signature,
                                        uint8_t *p_result,
                                        sgx_ecc_state_handle_t ecc_handle);

    /** Computes signature for a given data based on RSA 3072 private key
    *
    * A digital signature over a message consists of a 3072 bit number.
    *
    * Return: If private key, signature or data pointer is NULL,
    *                    SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the signing process fails then SGX_ERROR_UNEXPECTED is returned.
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: uint8_t *p_data - Pointer to the data to be signed
    *           uint32_t data_size - Size of the data to be signed
    *           sgx_rsa3072_key_t *p_key - Pointer to the RSA key. 
    *				Note: In IPP based version p_key->e is unused, hence it can be NULL.
    *   Output: sgx_rsa3072_signature_t *p_signature - Pointer to the signature output
    */
    sgx_status_t sgx_rsa3072_sign(const uint8_t *p_data,
        uint32_t data_size,
        const sgx_rsa3072_key_t *p_key,
        sgx_rsa3072_signature_t *p_signature);

    /** Computes signature for a given data based on RSA 3072 private key and optional
    * corresponding public key.
    *
    * A digital signature over a message consists of a 3072 bit number.
    *
    * Return: If private key, signature or data pointer is NULL,
    *                    SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the signing process fails then SGX_ERROR_UNEXPECTED is returned.
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: uint8_t *p_data - Pointer to the data to be signed
    *           uint32_t data_size - Size of the data to be signed
    *           sgx_rsa3072_key_t *p_key - Pointer to the RSA key. 
    *				Note: In IPP based version p_key->e is unused, hence it can be NULL.
    *           sgx_rsa3072_public_key_t *p_public - Pointer to the public key(optional).
    *   Output: sgx_rsa3072_signature_t *p_signature - Pointer to the signature output
    */
    sgx_status_t sgx_rsa3072_sign_ex(const uint8_t * p_data,
    uint32_t data_size,
    const sgx_rsa3072_key_t * p_key,
    const sgx_rsa3072_public_key_t *p_public,
    sgx_rsa3072_signature_t * p_signature);

    /** Verifies the signature for the given data based on the RSA 3072 public key.
    *
    * A digital signature over a message consists of a 3072 bit number.
    *
    * The typical result of the digital signature verification is one of the two values:
    *     SGX_Generic_ECValid - Digital signature is valid
    *     SGX_Generic_ECInvalidSignature -  Digital signature is not valid
    *
    * Return: If public key, signature, result or data pointer is NULL,
    *                    SGX_ERROR_INVALID_PARAMETER is returned.
    *         If the verification process fails then SGX_ERROR_UNEXPECTED is returned.
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: uint8_t *p_data - Pointer to the data to be verified
    *           uint32_t data_size - Size of the data to be verified
    *           sgx_rsa3072_public_key_t *p_public - Pointer to the public key
    *           sgx_rsa3072_signature_t *p_signature - Pointer to the signature
    *   Output: sgx_rsa_result_t *p_result - Pointer to the result of verification check
    */
    sgx_status_t sgx_rsa3072_verify(const uint8_t *p_data,
        uint32_t data_size,
        const sgx_rsa3072_public_key_t *p_public,
        const sgx_rsa3072_signature_t *p_signature,
		sgx_rsa_result_t *p_result);

    /** Create RSA key pair with <n_byte_size> key size and <e_byte_size> public exponent.
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: p_e [In/Out] Pointer to the public exponent e.
    *           n_byte_size [In] Size in bytes of the key modulus.
    *           e_byte_size	[In] Size in bytes of the key public exponent.
    *   Output: p_*			[Out] Pointer to the matching key parameter/factor buffer.
    */
    sgx_status_t sgx_create_rsa_key_pair(int n_byte_size, int e_byte_size, unsigned char *p_n, unsigned char *p_d, unsigned char *p_e,
        unsigned char *p_p, unsigned char *p_q, unsigned char *p_dmp1,
        unsigned char *p_dmq1, unsigned char *p_iqmp);

    /** Decrypt ciphertext [pin_data] using RSA private key, with OAEP SHA-256
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: rsa_key	- Pointer to the RSA private key.
    *           pin_data - Pointer to the input ciphertext buffer.
    *           pin_len - Ciphertext buffer size.
    *           pout_len[In/Out] - Pointer to the length of output buffer.
    *   Output: pout_data - Pointer to the output buffer.
    *           pout_len - Pointer to amount of data written.
    *
    */
    sgx_status_t sgx_rsa_priv_decrypt_sha256(const void* rsa_key, unsigned char* pout_data, size_t* pout_len, const unsigned char* pin_data, const size_t pin_len);

    /** Encrypt input data [pin_data] using RSA public key, with OAEP SHA-256
    *
    * Parameters:
    *   Return: sgx_status_t  - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: rsa_key - Pointer to the RSA public key.
    *           pin_data - Pointer to the input data buffer.
    *           pin_len - Input buffer size.
    *           pout_len[In/Out] - Pointer to the length of output buffer.
    *   Output: pout_data - Pointer to the output buffer.
    *           pout_len - Pointer to amount of data (ciphertext) written.
    *
    */
    sgx_status_t sgx_rsa_pub_encrypt_sha256(const void* rsa_key, unsigned char* pout_data, size_t* pout_len, const unsigned char* pin_data, const size_t pin_len);

    /** Create RSA private key using input buffer factors in little endian.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: mod_size - Size in bytes of the key modulus.
    *           exp_size - Size in bytes of the public exponent e.
    *           p_rsa_key_e	- Pointer to the public exponent e buffer.
    *           p_rsa_key_p	- Pointer to the prime number p.
    *           p_rsa_key_q	- Pointer to the prime number q.
    *           p_rsa_key_dmp1 - Pointer to dmp1 [d mod (p-1)].
    *           p_rsa_key_dmq1 - Pointer to dmq1 [d mod (q-1)].
    *           p_rsa_key_iqmp - Pointer to iqmp [q^-1 mod p].
    *   Output: new_pri_key2 - Pointer to the generated private key.
    *
    */
    sgx_status_t sgx_create_rsa_priv2_key(int mod_size, int exp_size, const unsigned char *p_rsa_key_e, const unsigned char *p_rsa_key_p, const unsigned char *p_rsa_key_q,
        const unsigned char *p_rsa_key_dmp1, const unsigned char *p_rsa_key_dmq1, const unsigned char *p_rsa_key_iqmp,
        void **new_pri_key2);

    /** Create RSA private key using input buffer factors - little endian.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: n_byte_size	- Size in bytes of the key modulus.
    *           e_byte_size	- Size in bytes of the public exponent e.
    *           d_byte_size	- Size in bytes of the private exponent d.
    *           le_e - Pointer to the public exponent e buffer.
    *           le_n - Pointer to the modulus n buffer.
    *           le_d - Pointer to the private exponent d buffer.
    *   Output: new_pri_key1 - Pointer to the generated private key.
    *
    */
    sgx_status_t sgx_create_rsa_priv1_key(int n_byte_size, int e_byte_size, int d_byte_size, const unsigned char *le_n, const unsigned char *le_e,
        const unsigned char *le_d, void **new_pri_key1);

    /** Create RSA public key using input buffer factors in little endian.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: mod_size		- Size in bytes of the key modulus.
    *           exp_size		- Size in bytes of the public exponent e.
    *           le_n			- Pointer to the modulus n buffer.
    *           le_e - Pointer to the public exponent e buffer.
    *   Output: new_pub_key1 - Pointer to the generated public key.
    *
    */
    sgx_status_t sgx_create_rsa_pub1_key(int mod_size, int exp_size, const unsigned char *le_n, const unsigned char *le_e, void **new_pub_key1);

    /** Clear and free RSA key which was generated by one of the Tcrypto "sgx_create_rsa_*" APIs.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: p_rsa_key		- Pointer to the RSA key.
    *           (Note: All input parameters below are relevant only when using IPP based tcrypto library)
    *           key_type		- key state type, relevant only when using IPP based tcrypto library.
    *                             Possible options {SGX_RSA_PRIVATE_KEY, SGX_RSA_PUBLIC_KEY}
    *           mod_size		- RSA key modulus size.
    *           exp_size		- RSA key public exponent size.
    *   Output:
    *
    */
    sgx_status_t sgx_free_rsa_key(void *p_rsa_key, sgx_rsa_key_type_t key_type, int mod_size, int exp_size);

    /** Generates an ECDSA private key based on input random seed.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: hash_drg - Pointer to the input random seed.
    *           hash_drg_len - Length of the input random seed.
    *           sgx_nistp256_r_m1 - Pointer to the buffer for n-1 where n is order of the ECC group used.
    *           sgx_nistp256_r_m1_len - Length for the buffer for nistp256.
    *   Output: out_key - Pointer to the generated ECDSA private key - BIG ENDIAN
    *           out_key_len - Length of the prepared buffer for ECDSA private key.
    *
    */
    sgx_status_t sgx_calculate_ecdsa_priv_key(const unsigned char* hash_drg, int hash_drg_len,
        const unsigned char* sgx_nistp256_r_m1, int sgx_nistp256_r_m1_len,
        unsigned char* out_key, int out_key_len);

    /** Generates an ECC public key based on a given ECC private key.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Input: p_att_priv_key - Input private key
    *   Output: p_att_pub_key - Output public key - LITTLE ENDIAN
    *
    */
    sgx_status_t sgx_ecc256_calculate_pub_from_priv(const sgx_ec256_private_t *p_att_priv_key,
        sgx_ec256_public_t  *p_att_pub_key);

    /** Allocate and initialize AES-GCM encryption state.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: p_key - Input private key.
    *           p_iv - Input initial vector. Encryption of different messages should use different IVs.
    *           iv_len - Length of initial vector, usually IV_SIZE.
    *           p_aad - AAD of AES-GCM, it could be NULL.
    *           aad_len - length of bytes of AAD.
    *   Output: aes_gcm_state - AES-GCM state pointer.
    *
    */
    sgx_status_t sgx_aes_gcm128_enc_init(
        const uint8_t *p_key,
        const uint8_t *p_iv,
        uint32_t iv_len,
        const uint8_t *p_aad,
        uint32_t aad_len,
        sgx_aes_state_handle_t *aes_gcm_state);

    /** Get authentication tag from the given AES-GCM state.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Input: aes_gcm_state - Pointer to AES-GCM state.
    *   Output: p_mac - Pointer to SGX_AESGCM_MAC_SIZE buffer to store mac in.
    *
    */
    sgx_status_t sgx_aes_gcm128_enc_get_mac(uint8_t *mac, sgx_aes_state_handle_t aes_gcm_state);

    /** Clear and free AES-GCM state.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Input: aes_gcm_state - Pointer to AES-GCM state.
    *
    */
    sgx_status_t sgx_aes_gcm_close(sgx_aes_state_handle_t aes_gcm_state);

    /** Encrypt plaintext from p_src into p_dst.
    *
    * Parameters:
    *   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
    *   Inputs: aes_gcm_state - Pointer to AES-GCM state.
    *           p_src - Pointer to plaintext buffer.
    *           src_len - Plaintext length.
    *   Output: p_dst - Pointer to ciphertext buffer.
    *
    */
    sgx_status_t sgx_aes_gcm128_enc_update(
        uint8_t *p_src,
        uint32_t src_len,
        uint8_t *p_dst,
        sgx_aes_state_handle_t aes_gcm_state);

#ifdef __cplusplus
}
#endif

#endif

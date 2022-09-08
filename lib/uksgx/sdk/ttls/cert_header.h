/**
 *
 * MIT License
 *
 * Copyright (c) Open Enclave SDK contributors.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE
 *
 */

#ifndef _CERT_HEADER_H_
#define _CERT_HEADER_H_

#include <stddef.h>
#include <stdint.h>
#include <openssl/x509.h>
#include "sgx_error.h"


#define SGX_CERT_MAGIC 0xa7a55f4322919317
#define SGX_CERT_CHAIN_MAGIC 0xa87e5d8e25671870
#define SGX_CRL_MAGIC 0x8f062e782b5760b2
#define SGX_EC_PRIVATE_KEY_MAGIC 0x9ffae0517397b76c
#define SGX_EC_PUBLIC_KEY_MAGIC 0xb8e1d57e9be31ed7
#define SGX_RSA_PRIVATE_KEY_MAGIC 0xba24987b29769828
#define SGX_RSA_PUBLIC_KEY_MAGIC 0x92f1fdf6c81b4aaa

// joint-ise-ccitt (2) country (16) usa (840) org (1) intel (113741) sgx (13.1)
#define X509_OID_FOR_QUOTE_STRING "1.2.840.113741.1.13.1"

typedef struct _sgx_cert
{
    /* Internal private implementation */
    uint64_t impl[4];
} sgx_cert_t;

typedef struct _sgx_cert_chain
{
    /* Internal private implementation */
    uint64_t impl[4];
} sgx_cert_chain_t;

typedef struct _sgx_crl
{
    /* Internal private implementation */
    uint64_t impl[4];
} sgx_crl_t;

typedef struct _crl
{
    uint64_t magic;
    X509_CRL* crl;
} crl_t;

typedef struct _sgx_oid_string
{
    char buf[128];
} sgx_oid_string_t;

typedef struct _sgx_public_key_t
{
    EVP_PKEY* pkey;
} sgx_public_key_t;

typedef struct _sgx_cert_config
{
    uint8_t* private_key_buf;
    size_t private_key_buf_size;
    uint8_t* public_key_buf;
    size_t public_key_buf_size;
    const unsigned char* subject_name;
    const unsigned char* issuer_name;
    unsigned char* date_not_valid_before;
    unsigned char* date_not_valid_after;
    uint8_t* quote_buf;
    size_t quote_buf_size;
    char* ext_oid;
    size_t ext_oid_size;
} sgx_cert_config_t;

/* includes all the headers from version number to subject unique identifier of
 * a X509 certificate */
#define SGX_MIN_CERT_SIZE 2048
#define KEY_BUFF_SIZE   SGX_MIN_CERT_SIZE

#define SGX_TLS_SAFE_FREE(x) {if(x) {free(x); x=NULL;}}

// Input: An issuer and subject key pair
// Output: A self-signed certificate embedded critical extension with quote
// Information as its content
sgx_status_t generate_x509_self_signed_certificate(
    const unsigned char* oid,
    size_t oid_size,
    const unsigned char *subject_name,
    const uint8_t *p_prv_key,
    size_t prv_key_size,
    const uint8_t *p_pub_key,
    size_t pub_key_size,
    const uint8_t* p_quote_buf,
    size_t quote_size,
    uint8_t **output_cert,
    size_t *output_cert_size);

sgx_status_t sgx_read_cert_in_der(
    sgx_cert_t* cert,
    const void* der_data,
    size_t der_size);

sgx_status_t sgx_cert_verify(
    sgx_cert_t* cert,
    sgx_cert_chain_t* chain,
    const sgx_crl_t* const* crls,
    size_t num_crls);

sgx_status_t sgx_cert_find_extension(
    const sgx_cert_t* cert,
    const char* oid,
    uint8_t* data,
    uint32_t* size);

sgx_status_t sgx_get_pubkey_from_cert(
    const sgx_cert_t* cert,
    uint8_t* pem_data,
    size_t* pem_size);

#endif

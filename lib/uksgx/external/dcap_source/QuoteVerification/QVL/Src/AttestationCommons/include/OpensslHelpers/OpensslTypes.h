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

#ifndef SGX_DCAP_COMMONS_OPENSSL_TYPES_H_
#define SGX_DCAP_COMMONS_OPENSSL_TYPES_H_

#include <tuple>
#include <memory>
#include <openssl/asn1.h>
#include <openssl/ssl.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/buffer.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/cmac.h>

namespace intel { namespace sgx { namespace dcap { namespace crypto {

/* Deleter declarations */
static void freeEVP_CIPHER_CTX(EVP_CIPHER_CTX* evp_cipher_ctx);
static void freeEVP_MD_CTX(EVP_MD_CTX* evp_md_ctx);
static void freeSTACK_OF_X509_EXTENSION(STACK_OF(X509_EXTENSION)* x509_extension_stack);
static void freeSTACK_OF_X509(STACK_OF(X509)* x509_stack);
static inline void freeSTACK_OF_ASN1TYPE(STACK_OF(ASN1_TYPE)* asn1type_stack);

/* Unique pointers using custom deleters declarations */
using ASN1_ENUMERATED_uptr          = std::unique_ptr<ASN1_ENUMERATED,          decltype(&ASN1_ENUMERATED_free)>;
using ASN1_INTEGER_uptr             = std::unique_ptr<ASN1_INTEGER,             decltype(&ASN1_INTEGER_free)>;
using ASN1_TIME_uptr                = std::unique_ptr<ASN1_TIME,                decltype(&ASN1_TIME_free)>;
using ASN1_TYPE_uptr                = std::unique_ptr<ASN1_TYPE,                decltype(&ASN1_TYPE_free)>;
using ASN1_STRING_uptr              = std::unique_ptr<ASN1_STRING,              decltype(&ASN1_STRING_free)>;
using BIGNUM_uptr                   = std::unique_ptr<BIGNUM,                   decltype(&BN_free)>;
using BN_CTX_uptr                   = std::unique_ptr<BN_CTX,                   decltype(&BN_CTX_free)>;
using BIO_uptr                      = std::unique_ptr<BIO,                      decltype(&BIO_free_all)>;
using CONF_uptr                     = std::unique_ptr<CONF,                     decltype(&NCONF_free)>;
using CMAC_CTX_uptr                 = std::unique_ptr<CMAC_CTX,                 decltype(&CMAC_CTX_free)>;
using EC_GROUP_uptr                 = std::unique_ptr<EC_GROUP,                 decltype(&EC_GROUP_free)>;
using EC_KEY_uptr                   = std::unique_ptr<EC_KEY,                   decltype(&EC_KEY_free)>;
using EC_POINT_uptr                 = std::unique_ptr<EC_POINT,                 decltype(&EC_POINT_free)>;
using ECDSA_SIG_uptr                = std::unique_ptr<ECDSA_SIG,                decltype(&ECDSA_SIG_free)>;
using EVP_CIPHER_CTX_uptr           = std::unique_ptr<EVP_CIPHER_CTX,           decltype(&freeEVP_CIPHER_CTX)>;
using EVP_MD_CTX_uptr               = std::unique_ptr<EVP_MD_CTX,               decltype(&freeEVP_MD_CTX)>;
using EVP_PKEY_uptr                 = std::unique_ptr<EVP_PKEY,                 decltype(&EVP_PKEY_free)>;
using EVP_PKEY_sptr                 = std::shared_ptr<EVP_PKEY>;
using EVP_PKEY_CTX_uptr             = std::unique_ptr<EVP_PKEY_CTX,             decltype(&EVP_PKEY_CTX_free)>;
using RSA_uptr                      = std::unique_ptr<RSA,                      decltype(&RSA_free)>;
using STACK_OF_ASN1TYPE_uptr        = std::unique_ptr<STACK_OF(ASN1_TYPE),      decltype(&freeSTACK_OF_ASN1TYPE)>;
using STACK_OF_X509_uptr            = std::unique_ptr<STACK_OF(X509),           decltype(&freeSTACK_OF_X509)>;
using STACK_OF_X509_EXTENSION_uptr  = std::unique_ptr<STACK_OF(X509_EXTENSION), decltype(&freeSTACK_OF_X509_EXTENSION)>;
using X509_uptr                     = std::unique_ptr<X509,                     decltype(&X509_free)>;
using X509_sptr                     = std::shared_ptr<X509>;
using X509_EXTENSION_uptr           = std::unique_ptr<X509_EXTENSION,           decltype(&X509_EXTENSION_free)>;
using X509_CRL_uptr                 = std::unique_ptr<X509_CRL,                 decltype(&X509_CRL_free)>;
using X509_REQ_uptr                 = std::unique_ptr<X509_REQ,                 decltype(&X509_REQ_free)>;
using X509_REVOKED_uptr             = std::unique_ptr<X509_REVOKED,             decltype(&X509_REVOKED_free)>;
using X509_STORE_uptr               = std::unique_ptr<X509_STORE,               decltype(&X509_STORE_free)>;
using X509_STORE_CTX_uptr           = std::unique_ptr<X509_STORE_CTX,           decltype(&X509_STORE_CTX_free)>;


/* Custom deleter definitions */
static inline void freeEVP_CIPHER_CTX(EVP_CIPHER_CTX* evp_cipher_ctx)
{
    EVP_CIPHER_CTX_free(evp_cipher_ctx);
}
static inline void freeEVP_MD_CTX(EVP_MD_CTX* evp_md_ctx)
{
    // before 1.1.0
    // EVP_MD_CTX_cleanup
    EVP_MD_CTX_free(evp_md_ctx);
}
static inline void freeSTACK_OF_X509_EXTENSION(STACK_OF(X509_EXTENSION)* x509_extension_stack)
{
    sk_X509_EXTENSION_pop_free(x509_extension_stack, X509_EXTENSION_free);
}

static inline void freeSTACK_OF_X509(STACK_OF(X509)* x509_stack)
{
    sk_X509_pop_free(x509_stack, X509_free);
}

static inline void freeSTACK_OF_ASN1TYPE(STACK_OF(ASN1_TYPE)* asn1type_stack)
{
    sk_ASN1_TYPE_pop_free(asn1type_stack, ASN1_TYPE_free);
}

static inline void freeOPENSSL(void *ptr)
{
    // OPENSSL_free is a macro, so we need to wrap it
    // to function
    OPENSSL_free(ptr);
}

/* Base template for the making of OpenSSL crypto objects, automatically selected deleter - unique_ptr variant */
template<typename T>
inline auto make_unique(T*) -> std::unique_ptr<T, void(*)(T*)>;

// Specialization for family of ASN1_TYPE typedefs:
//      ASN1_TYPE        ASN1_TYPE_free
template<>
inline ASN1_TYPE_uptr make_unique(ASN1_TYPE* raw_pointer)
{
    return ASN1_TYPE_uptr(raw_pointer, ASN1_TYPE_free);
}

// Specialization for family of asn1_string_st typedefs:
//      ASN1_INTEGER            ASN1_INTEGER_free
//      ASN1_ENUMERATED         ASN1_ENUMERATED_free
//      ASN1_BIT_STRING         ASN1_BIT_STRING_free
//      ASN1_OCTET_STRING       ASN1_OCTET_STRING_free
//      ASN1_PRINTABLESTRING    ASN1_PRINTABLESTRING_free
//      ASN1_T61STRING          ASN1_T61STRING_free
//      ASN1_IA5STRING          ASN1_IA5STRING_free
//      ASN1_GENERALSTRING      ASN1_GENERALSTRING_free
//      ASN1_UNIVERSALSTRING    ASN1_UNIVERSALSTRING_free
//      ASN1_BMPSTRING          ASN1_BMPSTRING_free
//      ASN1_UTCTIME            ASN1_UTCTIME_free
//      ASN1_TIME               ASN1_TIME_free
//      ASN1_GENERALIZEDTIME    ASN1_GENERALIZEDTIME_free
//      ASN1_VISIBLESTRING      ASN1_VISIBLESTRING_free
//      ASN1_UTF8STRING         ASN1_UTF8STRING_free
//      ASN1_STRING             ASN1_STRING_free
template<>
inline ASN1_STRING_uptr make_unique(ASN1_STRING* raw_pointer)
{
    return ASN1_STRING_uptr(raw_pointer, ASN1_STRING_free);
}

/* Section of helper functions for specific OpenSSL crypto objects */
template<>
inline BIGNUM_uptr make_unique(BIGNUM* raw_pointer)
{
    return BIGNUM_uptr(raw_pointer, BN_free);
}

template<>
inline BN_CTX_uptr make_unique(BN_CTX* raw_pointer)
{
    return BN_CTX_uptr(raw_pointer, BN_CTX_free);
}

template<>
inline BIO_uptr make_unique(BIO* raw_pointer)
{
    return BIO_uptr(raw_pointer, BIO_free_all);
}

template<>
inline CONF_uptr make_unique(CONF* raw_pointer)
{
    return CONF_uptr(raw_pointer, NCONF_free);
}

template<>
inline CMAC_CTX_uptr make_unique(CMAC_CTX* raw_pointer)
{
    return CMAC_CTX_uptr(raw_pointer, CMAC_CTX_free);
}

template<>
inline EC_GROUP_uptr make_unique(EC_GROUP* raw_pointer)
{
    return EC_GROUP_uptr(raw_pointer, EC_GROUP_free);
}

template<>
inline EC_KEY_uptr make_unique(EC_KEY* raw_pointer)
{
    return EC_KEY_uptr(raw_pointer, EC_KEY_free);
}

template<>
inline EC_POINT_uptr make_unique(EC_POINT* raw_pointer)
{
    return EC_POINT_uptr(raw_pointer, EC_POINT_free);
}

template<>
inline ECDSA_SIG_uptr make_unique(ECDSA_SIG* raw_pointer)
{
    return ECDSA_SIG_uptr(raw_pointer, ECDSA_SIG_free);
}

template<>
inline EVP_CIPHER_CTX_uptr make_unique(EVP_CIPHER_CTX* raw_pointer)
{
    return EVP_CIPHER_CTX_uptr(raw_pointer, freeEVP_CIPHER_CTX);
}

template<>
inline EVP_MD_CTX_uptr make_unique(EVP_MD_CTX* raw_pointer)
{
    return EVP_MD_CTX_uptr(raw_pointer, freeEVP_MD_CTX);
}

template<>
inline EVP_PKEY_uptr make_unique(EVP_PKEY* raw_pointer)
{
    return EVP_PKEY_uptr(raw_pointer, EVP_PKEY_free);
}

template<>
inline EVP_PKEY_CTX_uptr make_unique(EVP_PKEY_CTX* raw_pointer)
{
    return EVP_PKEY_CTX_uptr(raw_pointer, EVP_PKEY_CTX_free);
}

template<>
inline RSA_uptr make_unique(RSA* raw_pointer)
{
    return RSA_uptr(raw_pointer, RSA_free);
}

template<>
inline STACK_OF_ASN1TYPE_uptr make_unique(STACK_OF(ASN1_TYPE)* raw_pointer)
{
    return STACK_OF_ASN1TYPE_uptr(raw_pointer, freeSTACK_OF_ASN1TYPE);
}

template<>
inline STACK_OF_X509_EXTENSION_uptr make_unique(STACK_OF(X509_EXTENSION)* raw_pointer)
{
    return STACK_OF_X509_EXTENSION_uptr(raw_pointer, freeSTACK_OF_X509_EXTENSION);
}

template<>
inline STACK_OF_X509_uptr make_unique(STACK_OF(X509)* raw_pointer)
{
    return STACK_OF_X509_uptr(raw_pointer, freeSTACK_OF_X509);
}

template<>
inline X509_uptr make_unique(X509* raw_pointer)
{
    return X509_uptr(raw_pointer, X509_free);
}

template<>
inline X509_EXTENSION_uptr make_unique(X509_EXTENSION* raw_pointer)
{
    return X509_EXTENSION_uptr(raw_pointer, X509_EXTENSION_free);
}

template<>
inline X509_CRL_uptr make_unique(X509_CRL* raw_pointer)
{
    return X509_CRL_uptr(raw_pointer, X509_CRL_free);
}

template<>
inline X509_REQ_uptr make_unique(X509_REQ* raw_pointer)
{
    return X509_REQ_uptr(raw_pointer, X509_REQ_free);
}

template<>
inline X509_REVOKED_uptr make_unique(X509_REVOKED* raw_pointer)
{
    return X509_REVOKED_uptr(raw_pointer, X509_REVOKED_free);
}

template<>
inline X509_STORE_uptr make_unique(X509_STORE* raw_pointer)
{
    return X509_STORE_uptr(raw_pointer, X509_STORE_free);
}

template<>
inline X509_STORE_CTX_uptr make_unique(X509_STORE_CTX* raw_pointer)
{
    return X509_STORE_CTX_uptr(raw_pointer, X509_STORE_CTX_free);
}


}}}} // namespace intel { namespace sgx { namespace dcap { namespace crypto {

#endif


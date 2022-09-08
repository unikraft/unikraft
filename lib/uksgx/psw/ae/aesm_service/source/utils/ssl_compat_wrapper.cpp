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

#include "ssl_compat_wrapper.h"
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <stdlib.h>
#include <string.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L

static void *OPENSSL_zalloc(size_t num)
{
    void *ret = OPENSSL_malloc(num);

    if (ret != NULL)
        memset(ret, 0, num);
    return ret;
} 


int RSA_set0_key(RSA *r, BIGNUM *n, BIGNUM *e, BIGNUM *d)
{
    /* If the fields n and e in r are NULL, the corresponding input
     * parameters MUST be non-NULL for n and e.  d may be
     * left NULL (in case only the public key is used).
     */
    if (r == NULL 
        || (r->n == NULL && n == NULL)
        || (r->e == NULL && e == NULL))
        return 0;
 
    if (n != NULL)
    {
        BN_free(r->n);
        r->n = n;
    }
    if (e != NULL)
    {
        BN_free(r->e);
        r->e = e;
    }
    if (d != NULL)
    {
        BN_free(r->d);
        r->d = d;
    }
 
    return 1;
}

EVP_MD_CTX *EVP_MD_CTX_new(void)
{
   return (EVP_MD_CTX *)OPENSSL_zalloc(sizeof(EVP_MD_CTX));
}

void EVP_MD_CTX_free(EVP_MD_CTX *ctx)
{
   EVP_MD_CTX_cleanup(ctx);
   OPENSSL_free(ctx);
}

void ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps)
 {
    if (pr != NULL)
        *pr = sig->r;
    if (ps != NULL)
        *ps = sig->s;
 }

 int ECDSA_SIG_set0(ECDSA_SIG *sig, BIGNUM *r, BIGNUM *s)
 {
    if (r == NULL || s == NULL)
        return 0;
    BN_clear_free(sig->r);
    BN_clear_free(sig->s);
    sig->r = r;
    sig->s = s;
    return 1;
 }

BIGNUM *BN_lebin2bn(const unsigned char *s, int len, BIGNUM *ret)
{
    unsigned char *le_s = (unsigned char *)malloc(len);
    if(le_s == NULL)
        return NULL;
    for(unsigned int i = 0; i< len; i++)
    {
        le_s[i] = s[len - 1 - i];
    }
    BIGNUM *res = BN_bin2bn(le_s, len, ret);
    free(le_s);
    return res;
}

#endif


void crypto_initialize()
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
#else
    OPENSSL_init_crypto(0, NULL);
#endif

}

void crypto_cleanup()
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_remove_thread_state(NULL);
    ERR_free_strings();
#endif

}

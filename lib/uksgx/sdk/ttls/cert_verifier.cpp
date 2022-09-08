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

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/asn1.h>
#include <openssl/ec.h>
#include <openssl/rsa.h>
#include <string.h>
#include <sgx_error.h>
#include "cert_header.h"

typedef struct _cert
{
    uint64_t magic;
    X509* x509;
} cert_t;


static void _cert_init(cert_t* impl, X509* x509)
{
    impl->magic = SGX_CERT_MAGIC;
    impl->x509 = x509;
}

static bool _cert_is_valid(const cert_t* impl)
{
    return impl && (impl->magic == SGX_CERT_MAGIC) && impl->x509;
}

static void _cert_clear(cert_t* impl)
{
    if (impl)
    {
        impl->magic = 0;
        impl->x509 = NULL;
    }
}

typedef struct _cert_chain
{
    uint64_t magic;
    STACK_OF(X509) * sk;
} cert_chain_t;

static bool _cert_chain_is_valid(const cert_chain_t* impl)
{
    return impl && (impl->magic == SGX_CERT_CHAIN_MAGIC) && impl->sk;
}

/* Clone the certificate to clear any verification state */
static X509* _clone_x509(X509* x509)
{
    X509* ret = NULL;
    BIO* out = NULL;
    BIO* in = NULL;
    BUF_MEM* mem;

    if (!x509)
        goto done;

    if (!(out = BIO_new(BIO_s_mem())))
        goto done;

    if (!PEM_write_bio_X509(out, x509))
        goto done;

    if (!BIO_get_mem_ptr(out, &mem))
        goto done;

    if (mem->length > INT_MAX)
        goto done;

    if (!(in = BIO_new_mem_buf(mem->data, (int)mem->length)))
        goto done;

    ret = PEM_read_bio_X509(in, NULL, 0, NULL);

done:

    if (out)
        BIO_free(out);

    if (in)
        BIO_free(in);

    return ret;
}

#if OPENSSL_VERSION_NUMBER < 0x10100000L
/* Needed because some versions of OpenSSL do not support X509_up_ref() */
static int X509_up_ref(X509* x509)
{
    if (!x509)
        return 0;

    CRYPTO_add(&x509->references, 1, CRYPTO_LOCK_X509);
    return 1;
}

static const STACK_OF(X509_EXTENSION) * X509_get0_extensions(const X509* x)
{
    if (!x->cert_info)
    {
        return NULL;
    }
    return x->cert_info->extensions;
}

#endif

static STACK_OF(X509) * _clone_chain(STACK_OF(X509) * chain)
{
    STACK_OF(X509)* sk = NULL;
    int n = sk_X509_num(chain);

    if (!(sk = sk_X509_new(NULL)))
        return NULL;

    for (int i = 0; i < n; i++)
    {
        X509* x509;

        if (!(x509 = sk_X509_value(chain, (int)i)))
            return NULL;

        if (!(x509 = _clone_x509(x509)))
            return NULL;

        if (!sk_X509_push(sk, x509))
            return NULL;
    }

    return sk;
}

static sgx_status_t _verify_cert(
    X509* cert,
    STACK_OF(X509) * chain_,
    const sgx_crl_t* const* crls,
    size_t num_crls)
{
    sgx_status_t result = SGX_ERROR_UNEXPECTED;
    X509_STORE_CTX* ctx = NULL;
    X509_STORE* store = NULL;
    X509* x509 = NULL;
    STACK_OF(X509)* chain = NULL;

    /* Clone the certificate to clear any cached verification state */
    if (!(x509 = _clone_x509(cert)))
        goto end;

    /* Clone the chain to clear any cached verification state */
    if (chain_ && !(chain = _clone_chain(chain_)))
        goto end;

    /* Create a store for the verification */
    if (!(store = X509_STORE_new()))
        goto end;

    /* Create a context for verification */
    if (!(ctx = X509_STORE_CTX_new()))
        goto end;

    /* Initialize the context that will be used to verify the certificate */
    if (!X509_STORE_CTX_init(ctx, store, NULL, NULL))
        goto end;

    /* Create a store with CRLs if needed */
    if (crls && num_crls)
    {
        X509_VERIFY_PARAM* verify_param = NULL;

        for (size_t i = 0; i < num_crls; i++)
        {
            crl_t* crl_impl = (crl_t*)crls[i];

            /* X509_STORE_add_crl manages its own addition refcount */
            if (!X509_STORE_add_crl(store, crl_impl->crl))
                goto end;
        }

        /* Get the verify parameter (must not be null) */
        if (!(verify_param = X509_STORE_CTX_get0_param(ctx)))
            goto end;

        X509_VERIFY_PARAM_set_flags(verify_param, X509_V_FLAG_CRL_CHECK);
        X509_VERIFY_PARAM_set_flags(verify_param, X509_V_FLAG_CRL_CHECK_ALL);
    }

    /* Inject the certificate into the verification context */
    X509_STORE_CTX_set_cert(ctx, x509);

    /* Set the CA chain into the verification context */
    if (chain)
        X509_STORE_CTX_trusted_stack(ctx, chain);
    else
        X509_STORE_add_cert(store, x509);

    /* Finally verify the certificate */
    if (!X509_verify_cert(ctx))
    {
        int errorno = X509_STORE_CTX_get_error(ctx);
        if (errorno != X509_V_OK)
            goto end;
    }

    result = SGX_SUCCESS;

end:
    if (x509)
        X509_free(x509);

    if (chain)
        sk_X509_pop_free(chain, X509_free);

    if (store)
        X509_STORE_free(store);

    if (ctx)
        X509_STORE_CTX_free(ctx);

    return result;
}


sgx_status_t sgx_read_cert_in_der(
    sgx_cert_t* cert,
    const void* der_data,
    size_t der_size)
{
    sgx_status_t result = SGX_ERROR_UNEXPECTED;
    cert_t* impl = (cert_t*)cert;
    X509* x509 = NULL;
    unsigned char* p = NULL;

    /* Zero-initialize the implementation */
    if (impl)
        impl->magic = 0;

    /* Check parameters */
    if (!der_data || !der_size || der_size > INT_MAX || !cert)
        return SGX_ERROR_INVALID_PARAMETER;

    /* Initialize OpenSSL (if not already initialized) */
    //sgxssl_crypto_initialize();

    p = (unsigned char*)der_data;

    /* Convert the PEM BIO into a certificate object */
    if (!(x509 = d2i_X509(NULL, (const unsigned char**)&p, (int)der_size)))
        goto end;

    _cert_init(impl, x509);
    x509 = NULL;

    result = SGX_SUCCESS;

end:

    X509_free(x509);

    return result;
}

sgx_status_t sgx_cert_free(sgx_cert_t* cert)
{
    sgx_status_t result = SGX_ERROR_UNEXPECTED;
    cert_t* impl = (cert_t*)cert;

    /* Check parameters */
    if (!_cert_is_valid(impl))
        goto end;

    /* Free the certificate */
    X509_free(impl->x509);
    _cert_clear(impl);

    result = SGX_SUCCESS;

end:
    return result;
}

sgx_status_t sgx_cert_verify(
    sgx_cert_t* cert,
    sgx_cert_chain_t* chain,
    const sgx_crl_t* const* crls,
    size_t num_crls)
{
    sgx_status_t result = SGX_ERROR_UNEXPECTED;
    cert_t* cert_impl = (cert_t*)cert;
    cert_chain_t* chain_impl = (cert_chain_t*)chain;

    /* Check for invalid cert parameter */
    if (!_cert_is_valid(cert_impl))
        return SGX_ERROR_INVALID_PARAMETER;

    /* Check for invalid chain parameter */
    if (chain && !_cert_chain_is_valid(chain_impl))
        return SGX_ERROR_INVALID_PARAMETER;

    /* Verify the certificate */
    _verify_cert(
        cert_impl->x509,
        (chain_impl != NULL ? chain_impl->sk : NULL),
        crls,
        num_crls);

    result = SGX_SUCCESS;

    return result;
}

sgx_status_t sgx_cert_find_extension(
    const sgx_cert_t* cert,
    const char* oid,
    uint8_t* data,
    uint32_t* size)
{
    sgx_status_t result = SGX_ERROR_UNEXPECTED;
    const cert_t* impl = (const cert_t*)cert;
    const STACK_OF(X509_EXTENSION) * extensions;
    int num_extensions;

    /* Reject invalid parameters */
    if (!_cert_is_valid(impl) || !oid || !size) {
        result = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    /* Set a pointer to the stack of extensions (possibly NULL) */
    if (!(extensions = X509_get0_extensions(impl->x509)))
        goto done;

    /* Get the number of extensions (possibly zero) */
    num_extensions = sk_X509_EXTENSION_num(extensions);

    /* Find the certificate with this OID */
    for (int i = 0; i < num_extensions; i++)
    {
        X509_EXTENSION* ext;
        ASN1_OBJECT* obj;
        sgx_oid_string_t ext_oid;

        /* Get the i-th extension from the stack */
        if (!(ext = sk_X509_EXTENSION_value(extensions, i)))
            goto done;

        /* Get the OID */
        if (!(obj = X509_EXTENSION_get_object(ext)))
            goto done;

        /* Get the string name of the OID */
        if (!OBJ_obj2txt(ext_oid.buf, sizeof(ext_oid.buf), obj, 1))
            goto done;

        /* If found then get the data */
        if (strcmp(ext_oid.buf, oid) == 0)
        {
            ASN1_OCTET_STRING* str;

            /* Get the data from the extension */
            if (!(str = X509_EXTENSION_get_data(ext)))
                goto done;

            /* If the caller's buffer is too small, raise error */
            if ((size_t)str->length > *size)
            {
                *size = (size_t)str->length;
                result = SGX_ERROR_INVALID_PARAMETER;
                goto done;
            }

            if (data)
            {
                memcpy(data, str->data, (size_t)str->length);
                *size = (size_t)str->length;
                result = SGX_SUCCESS;
                goto done;
            }
        }
    }

done:
    return result;
}

sgx_status_t sgx_cert_get_public_key(
    const sgx_cert_t* cert,
    sgx_public_key_t* public_key)
{
    sgx_status_t result = SGX_ERROR_UNEXPECTED;
    const cert_t* impl = (const cert_t*)cert;
    EVP_PKEY* pkey = NULL;

    if (public_key)
        memset(public_key, 0, sizeof(sgx_public_key_t));

    if (!_cert_is_valid(impl) || !public_key) {
        result = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!(pkey = X509_get_pubkey(impl->x509))) {
        result = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    public_key->pkey = pkey;
    pkey = NULL;

    result = SGX_SUCCESS;

done:

    if (pkey)
    {
        EVP_PKEY_free(pkey);
    }

    return result;
}

sgx_status_t sgx_public_key_write_pem(
    const sgx_public_key_t* key,
    uint8_t* data,
    size_t* size)
{
    sgx_status_t result = SGX_ERROR_UNEXPECTED;
    BIO* bio = NULL;
    const sgx_public_key_t* impl = (const sgx_public_key_t*)key;
    const char null_terminator = '\0';

    /* If buffer is null, then size must be zero */
    if (!key || (!data && *size != 0)) {
        result = SGX_ERROR_INVALID_PARAMETER;
        goto done;
    }

    /* Create memory BIO object to write key to */
    if (!(bio = BIO_new(BIO_s_mem()))) {
        result = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    /* Write key to BIO */
    if (!PEM_write_bio_PUBKEY(bio, impl->pkey)) {
        result = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    /* Write a NULL terminator onto BIO */
    if (BIO_write(bio, &null_terminator, sizeof(null_terminator)) <= 0) {
        result = SGX_ERROR_UNEXPECTED;
        goto done;
    }

    /* Copy the BIO onto caller's memory */
    {
        BUF_MEM* mem;

        if (!BIO_get_mem_ptr(bio, &mem)) {
            result = SGX_ERROR_UNEXPECTED;
            goto done;
        }

        /* If buffer is too small */
        if (*size < mem->length)
        {
            *size = mem->length;

            result = SGX_ERROR_OUT_OF_MEMORY;
            goto done;
        }

        /* Copy result to output buffer */
        memcpy(data, mem->data, mem->length);
        *size = mem->length;
    }

    result = SGX_SUCCESS;

done:

    if (bio)
        BIO_free(bio);

    return result;
}


sgx_status_t sgx_get_pubkey_from_cert(
    const sgx_cert_t* cert,
    uint8_t* pem_data,
    size_t* pem_size)
{
    sgx_status_t result = SGX_ERROR_UNEXPECTED;
    sgx_public_key_t public_key;

    if (SGX_SUCCESS != sgx_cert_get_public_key(cert, &public_key)) {
        goto done;
    }
    
    if (SGX_SUCCESS != sgx_public_key_write_pem(&public_key, pem_data, pem_size)) {
        goto done;
    }

    result = SGX_SUCCESS;

done:

    if (public_key.pkey) {
        EVP_PKEY_free(public_key.pkey);
        public_key.pkey = NULL;
    }

    return result;
}

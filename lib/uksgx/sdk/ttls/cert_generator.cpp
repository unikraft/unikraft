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
#include <openssl/x509_vfy.h>
#include <openssl/x509v3.h>
#include <string.h>
#include "cert_header.h"
#include "se_memcpy.h"


#define SSL_ERR_BREAK(x, y) {if(!x) { result = y; break;}}
#define X509_MAX_NAME_SIZE 256

#define SUBJECT_NAME "CN=Intel SGX Enclave,O=Intel Corporation,C=US"
#define DATE_NOT_VALID_BEFORE "20210401000000"
#define DATE_NOT_VALID_AFTER "20501231235959"

/*
 * Value used by the _decode_oid_to_str() function. Although the OID
 * standard does not limit the depth of an OID definition tree (i.e., the
 * number of arcs), our implementation only supports a simple decoding
 * with a limited depth (i.e., decoding into a fixed size string).
 */

/*
 * Parse the name string into X509_NAME struct. The format of the string is
 * "KEY1=VALUE1,KEY2=VALUE2,KEY3=VALUE3...". The implementation is based
 * on the mbedtls_x509_string_to_names from Mbed TLS.
 * Note that the string is expected to use commas as the separators instead
 * of slashes as OpenSSL CLI does. Also, the implementation does not
 * support multivalue-RDN names (with the "+" in the value).
 */
static X509_NAME* X509_parse_name(const char* name_string)
{
    const char* s = name_string;
    const char* c = s;
    const char* end = s + strlen(s);
    int in_tag = 1;
    char key[X509_MAX_NAME_SIZE];
    char data[X509_MAX_NAME_SIZE];
    char* d = data;
    X509_NAME* name = NULL;
    int error = 1;

    name = X509_NAME_new();
    if (name == NULL)
        goto done;

    while (c <= end)
    {
        if (in_tag && *c == '=')
        {
            size_t len = (size_t)(c - s) + 1;
            if (len > X509_MAX_NAME_SIZE)
                goto done;

            if (memcpy_s(key, X509_MAX_NAME_SIZE, s, len))
                goto done;
            key[len - 1] = '\0';
            s = c + 1;
            in_tag = 0;
            d = data;
        }

        if (!in_tag && *c == '\\' && c != end)
        {
            c++;
            /* Only support escaping commas */
            if (c == end || *c != ',')
                goto done;
        }
        else if (!in_tag && (*c == ',' || c == end))
        {
            /*
             * The check of if(d - data == OE_X509_MAX_NAME_SIZE)
             * below ensures that d should never go beyond the boundary of data.
             * Place null that indicates the end of the string.
             */
            *d = '\0';
            if (!X509_NAME_add_entry_by_txt(
                    name, key, MBSTRING_UTF8, (unsigned char*)data, -1, -1, 0))
                goto done;

            /* Skip the spaces after the comma */
            while (c < end && *(c + 1) == ' ')
                c++;
            s = c + 1;
            in_tag = 1;
        }

        if (!in_tag && s != c + 1)
        {
            *(d++) = *c;
            if (d - data == X509_MAX_NAME_SIZE)
                goto done;
        }

        c++;
    }

    error = 0;

done:
    if (error && name)
    {
        X509_NAME_free(name);
        name = NULL;
    }

    return name;
}

static sgx_status_t sgx_gen_custom_x509_cert(
    sgx_cert_config_t* config,
    unsigned char* cert_buf,
    size_t cert_buf_size,
    size_t* bytes_written)
{
    sgx_status_t result = SGX_ERROR_UNEXPECTED;
    X509* x509cert = NULL;
    X509V3_CTX ctx;
    BIO* bio = NULL;
    X509_NAME* name = NULL;
    EVP_PKEY* subject_issuer_key_pair = NULL;
    X509_EXTENSION* ext = NULL;
    ASN1_OBJECT* obj = NULL;
    ASN1_OCTET_STRING* data = NULL;
    BASIC_CONSTRAINTS* bc = NULL;
    unsigned char* buf = NULL;
    unsigned char* p = NULL;
    char* oid = NULL;
    char date_str[16];
    int len = 0;
    int ret = 0;

    do {
        // Initialize SGXSSL crypto
        OPENSSL_init_crypto(0, NULL);

        x509cert = X509_new();
        subject_issuer_key_pair = EVP_PKEY_new();

        /* Allocate buffer for certificate */
        if ((buf = (unsigned char*)malloc(cert_buf_size)) == NULL)
            SSL_ERR_BREAK(buf, SGX_ERROR_OUT_OF_MEMORY);

        /* Set certificate info */

        /* Parse public key */
        bio = BIO_new_mem_buf((const void*)config->public_key_buf, (int)config->public_key_buf_size);
        SSL_ERR_BREAK(bio, SGX_ERROR_UNEXPECTED);

        if (!PEM_read_bio_PUBKEY(bio, &subject_issuer_key_pair, NULL, NULL))
            break;

        EVP_PKEY_base_id(subject_issuer_key_pair);

        BIO_free(bio);
        bio = NULL;

        /* Parse private key */
        bio = BIO_new_mem_buf(
            (const void*)config->private_key_buf,
            (int)config->private_key_buf_size);
        SSL_ERR_BREAK(bio, SGX_ERROR_UNEXPECTED);

        if (!PEM_read_bio_PrivateKey(bio, &subject_issuer_key_pair, NULL, NULL))
            break;

        BIO_free(bio);
        bio = NULL;

        /* Set version to V3 */
        ret = X509_set_version(x509cert, 2);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);

        /* Set key */
        ret = X509_set_pubkey(x509cert, subject_issuer_key_pair);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);

        /* Covert the subject string to X509_name struct */
        name = X509_parse_name((const char*)config->subject_name);
        SSL_ERR_BREAK(name, SGX_ERROR_UNEXPECTED);

        /* Set subject name */
        ret = X509_set_subject_name(x509cert, name);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);

        X509_NAME_free(name);
        name = NULL;

        /* Covert the issuer string to X509_name struct */
        name = X509_parse_name((const char*)config->issuer_name);
        SSL_ERR_BREAK(name, SGX_ERROR_UNEXPECTED);

        /* Set issuer name */
        ret = X509_set_issuer_name(x509cert, name);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);

        X509_NAME_free(name);
        name = NULL;

        /* Set serial number */
        ret = ASN1_INTEGER_set(X509_get_serialNumber(x509cert), 1);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);

        /* Convert the format YYYYMMDDHHMMSS to YYYYMMDDHHMMSSZ */
        strncpy(date_str, (const char*)config->date_not_valid_before, 14);
        date_str[14] = 'Z';
        date_str[15] = '\0';

        /* Set validity start date */
        ret = ASN1_TIME_set_string(X509_getm_notBefore(x509cert), date_str);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);

        /* Convert the format YYYYMMDDHHMMSS to YYYYMMDDHHMMSSZ */
        strncpy(date_str, (const char*)config->date_not_valid_after, 14);
        date_str[14] = 'Z';
        date_str[15] = '\0';

        /* Set validity end date */
        ret = ASN1_TIME_set_string(X509_getm_notAfter(x509cert), date_str);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);

        /* Initialize the ctx. Required by X509V3_EXT_conf_nid. */
        /* No configuration database */
        X509V3_set_ctx_nodb(&ctx);
        /* Use the target as both issuer and subject for the self-signed
        * certificate. */
        X509V3_set_ctx(&ctx, x509cert, x509cert, NULL, NULL, 0);

        /* Set the basic constraints extention */
        ext = X509V3_EXT_conf_nid(NULL, &ctx, NID_basic_constraints, "CA:FALSE");
        SSL_ERR_BREAK(ext, SGX_ERROR_UNEXPECTED);

        ret = X509_add_ext(x509cert, ext, -1);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);
   
        X509_EXTENSION_free(ext);
        ext = NULL;

        /* Set the subject key identifier extension */
        ext = X509V3_EXT_conf_nid(NULL, &ctx, NID_subject_key_identifier, "hash");
        SSL_ERR_BREAK(ext, SGX_ERROR_UNEXPECTED);

        ret = X509_add_ext(x509cert, ext, -1);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);

        X509_EXTENSION_free(ext);
        ext = NULL;

        /* Set the authority key identifier extension */
        ext = X509V3_EXT_conf_nid(
            NULL, &ctx, NID_authority_key_identifier, "keyid:always");
        SSL_ERR_BREAK(ext, SGX_ERROR_UNEXPECTED);
   
        ret = X509_add_ext(x509cert, ext, -1);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);
    
        X509_EXTENSION_free(ext);
        ext = NULL;

        /* Set the custom extension */
        data = ASN1_OCTET_STRING_new();
        SSL_ERR_BREAK(data, SGX_ERROR_UNEXPECTED);

        ret = ASN1_OCTET_STRING_set(
            data,
            (const unsigned char*)config->quote_buf,
            (int)config->quote_buf_size);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);
    
        obj = OBJ_txt2obj(config->ext_oid, 1);
        SSL_ERR_BREAK(obj, SGX_ERROR_UNEXPECTED);

        if (!X509_EXTENSION_create_by_OBJ(&ext, obj, 0, data))
            break;

        ret = X509_add_ext(x509cert, ext, -1);
        SSL_ERR_BREAK(ret, SGX_ERROR_UNEXPECTED);

        /* Sign the certificate */
        if (!X509_sign(x509cert, subject_issuer_key_pair, EVP_sha256()))
            break;

        /*
         * Write to DER
         * The use of temporary variable is mandatory.
         * If p is not NULL, the i2d_x509 function writes the DER encoded data to
         * the buffer at *p and increments p to point after the data just written.
         */
        p = buf;
        len = i2d_X509(x509cert, &p);
        if (len <= 0)
            break;

        *bytes_written = (size_t)len;

        /* Copy DER data to buffer */
        if (0 != (memcpy_s((void*)cert_buf, cert_buf_size, (const void*)buf, *bytes_written)))
            break;

        result = SGX_SUCCESS;

    } while(0);

    if (x509cert)
        X509_free(x509cert);
    if (ext)
        X509_EXTENSION_free(ext);
    if (name)
        X509_NAME_free(name);
    if (bio)
        BIO_free(bio);
    if (obj)
        ASN1_OBJECT_free(obj);
    if (data)
        ASN1_OCTET_STRING_free(data);
    if (bc)
        BASIC_CONSTRAINTS_free(bc);
    if (subject_issuer_key_pair)
        EVP_PKEY_free(subject_issuer_key_pair);
    if (buf)
    {
        free(buf);
        buf = NULL;
    }
    if (oid)
    {
        free(oid);
        oid = NULL;
    }
    p = NULL;

    return result;
}

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
    size_t *output_cert_size)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    size_t bytes_written = 0;
    uint8_t* cert_buf = NULL;
    sgx_cert_config_t config;
    size_t sgx_cert_size = 0;

    config.private_key_buf = (uint8_t*)p_prv_key;
    config.private_key_buf_size = prv_key_size;
    config.public_key_buf = (uint8_t*)p_pub_key;
    config.public_key_buf_size = pub_key_size;
    config.subject_name = (subject_name != NULL)
                              ? subject_name
                              : (const unsigned char*)SUBJECT_NAME;
    config.issuer_name = config.subject_name;
    config.date_not_valid_before = (unsigned char*)DATE_NOT_VALID_BEFORE;
    config.date_not_valid_after = (unsigned char*)DATE_NOT_VALID_AFTER;
    config.quote_buf = (uint8_t*)p_quote_buf;
    config.quote_buf_size = quote_size;
    config.ext_oid = (char*)oid;
    config.ext_oid_size = oid_size;

    do {
        // allocate memory for cert output buffer and leave room for paddings
        sgx_cert_size = quote_size + pub_key_size + SGX_MIN_CERT_SIZE;
        cert_buf = (uint8_t*)malloc(sgx_cert_size);
        if (cert_buf == NULL)
            break;

        try {
            ret = sgx_gen_custom_x509_cert(&config, cert_buf, sgx_cert_size, &bytes_written);

            if (ret != SGX_SUCCESS)
                break;
        }
        catch (...) {
            ret = SGX_ERROR_UNEXPECTED;
            break;
        }

        *output_cert_size = (size_t)bytes_written;
        *output_cert = cert_buf;
        ret = SGX_SUCCESS;
    } while (0);

    if (ret != SGX_SUCCESS) {
        if(cert_buf) {
            free(cert_buf);
            cert_buf = NULL;
        }
    }

    return ret;
}

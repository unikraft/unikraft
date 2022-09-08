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

#ifndef SGX_DCAP_COMMONS_TEST_X509_CERT_GENERATOR_H
#define SGX_DCAP_COMMONS_TEST_X509_CERT_GENERATOR_H

#include <SgxEcdsaAttestation/AttestationParsers.h>

#include "X509CertTypes.h"
#include "OpensslHelpers/Bytes.h"
#include "OpensslHelpers/OpensslTypes.h"

#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/asn1t.h>

#include <stdio.h>
#include <stdlib.h>

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace test {

class X509CertGenerator {
public:
    crypto::X509_uptr generateCaCert(int version, const Bytes &serialNumber, long notBeforeOffset, long notAfterOffset,
                                     const EVP_PKEY *pubKey, const EVP_PKEY *signingKey,
                                     const dcap::parser::x509::DistinguishedName &subject, const dcap::parser::x509::DistinguishedName &issuer) const;

    crypto::X509_uptr generateTcbSigningCert(int version, const Bytes &serialNumber, long notBeforeOffset, long notAfterOffset,
                                             const EVP_PKEY *pubKey, const EVP_PKEY *signingKey,
                                             const dcap::parser::x509::DistinguishedName &subject, const dcap::parser::x509::DistinguishedName &issuer) const;

    crypto::X509_uptr generatePCKCert(int version, const Bytes &serialNumber, long notBeforeOffset, long notAfterOffset,
                                   const EVP_PKEY *pubKey, const EVP_PKEY *signingKey,
                                   const dcap::parser::x509::DistinguishedName &subject,
                                   const dcap::parser::x509::DistinguishedName &issuer,
                                   const Bytes& ppid, const Bytes& cpusvn, const Bytes& pcesvn,
                                   const Bytes& pceId, const Bytes& fmspc,
                                   const int sgxType, const Bytes& platformInstanceId = {},
                                   const bool dynamicPlatform = false, const bool cachedKeys = false,
                                   const bool smtEnabled = false, bool unexpectedExtension = false) const;

    crypto::EVP_PKEY_uptr generateEcKeypair() const;

    std::string x509ToString(const X509 *cert);

private:
    crypto::X509_uptr generateBaseCert(int version, const Bytes &serialNumber,
        long notBeforeOffset, long notAfterOffset,
        const EVP_PKEY *pubKey,
        const dcap::parser::x509::DistinguishedName &subject, const dcap::parser::x509::DistinguishedName &issuer) const;

    void addStandardCaExtensions(const crypto::X509_uptr& cert) const;
    void addStandardNonCaExtensions(const crypto::X509_uptr& cert) const;
    void addSGXPckExtensions(const crypto::X509_uptr &cert, const Bytes &ppid, const Bytes &cpusvn, const Bytes &pcesvn,
                             const Bytes &pceId, const Bytes &fmspc, const int sgxType, const Bytes &platformInstanceId,
                             const bool dynamicPlatform, const bool cachedKeys, const bool smtEnabled,
                             const bool unexpectedExtension) const;

    void add_ext(X509 *cert, int nid, char *value) const;
    void createTcbComp(const std::string &parentOid, int index, unsigned char value, crypto::SGX_INT* dest) const;

    void signCert(const crypto::X509_uptr& cert, const EVP_PKEY *signingKey) const;
};

}}}}}

#endif //SGX_DCAP_COMMONS_TEST_X509_CERT_GENERATOR_H

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

#ifndef SGXECDSAATTESTATION_X509CRLGENERATOR_H
#define SGXECDSAATTESTATION_X509CRLGENERATOR_H

#include <PckParser/PckParser.h>
#include <openssl/x509v3.h>
#include <openssl/x509.h>
#include <openssl/bn.h>

#include <OpensslHelpers/OpensslTypes.h>
#include <OpensslHelpers/Bytes.h>

namespace intel{ namespace sgx{ namespace dcap{ namespace test{

enum CRLVersion {
    CRL_VERSION_1 = 0,
    CRL_VERSION_2 = 1
};

class X509CrlGenerator {
public:
    crypto::X509_CRL_uptr generateCRL(CRLVersion version, long notBeforeOffset, long notAfterOffset,
                                      const crypto::X509_uptr &issuerCert, const std::vector<Bytes> &revokedSerials = std::vector<Bytes>{}) const;

    static std::string x509CrlToPEMString(const X509_CRL *crl);
    static std::string x509CrlToDERString(const X509_CRL *crl);

private:
    void revokeSerialNumber(const crypto::X509_CRL_uptr &crl, const Bytes &serialNumber) const;
    void addStandardCrlExtensions(const crypto::X509_CRL_uptr& crl, const crypto::X509_uptr& issuerCert) const;

};

}}}}


#endif //SGXECDSAATTESTATION_X509CRLGENERATOR_H

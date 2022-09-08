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

#include "X509CrlGenerator.h"
#include <algorithm>

namespace intel{ namespace sgx{ namespace dcap{ namespace test{

crypto::X509_CRL_uptr X509CrlGenerator::generateCRL(CRLVersion version, long lastUpdateOffset, long nextUpdateOffset,
                                                    const crypto::X509_uptr &issuerCert, const std::vector<Bytes> &revokedSerials) const {

    auto crl = crypto::make_unique(X509_CRL_new());
    auto crlPtr = crl.get();

    X509_CRL_set_version(crlPtr, version);

    auto issuerName = X509_get_subject_name(issuerCert.get());
    X509_CRL_set_issuer_name(crlPtr, issuerName);

    auto lastUpdate = crypto::make_unique(ASN1_TIME_new());
    X509_gmtime_adj(lastUpdate.get(), lastUpdateOffset);
    auto nextUpdate = crypto::make_unique(ASN1_TIME_new());
    X509_gmtime_adj(nextUpdate.get(), nextUpdateOffset);

    X509_CRL_set_lastUpdate(crlPtr, lastUpdate.get());
    X509_CRL_set_nextUpdate(crlPtr, nextUpdate.get());

    addStandardCrlExtensions(crl, issuerCert);

    for(auto &serial : revokedSerials)
    {
        revokeSerialNumber(crl, serial);
    }
    X509_CRL_sort(crl.get());

    auto signingKey = X509_get0_pubkey(issuerCert.get());
    X509_CRL_sign(crlPtr, signingKey, EVP_sha256());

    return crl;
}

void X509CrlGenerator::revokeSerialNumber(const crypto::X509_CRL_uptr &crl, const Bytes &serialNumber) const
{
    auto revoked = crypto::make_unique(X509_REVOKED_new());

    auto bn_serial = crypto::make_unique(BN_new());
    auto bnPtr = bn_serial.get();
    BN_bin2bn(serialNumber.data(), static_cast<int>(serialNumber.size()), bnPtr);
    auto serial = crypto::make_unique(BN_to_ASN1_INTEGER(bnPtr, nullptr));
    X509_REVOKED_set_serialNumber(revoked.get(), serial.get());

    auto revocationDate = crypto::make_unique(ASN1_TIME_new());
    X509_gmtime_adj(revocationDate.get(), 0);
    X509_REVOKED_set_revocationDate(revoked.get(), revocationDate.get());

    X509_CRL_add0_revoked(crl.get(), revoked.release());
}

std::string X509CrlGenerator::x509CrlToPEMString(const X509_CRL *crl)
{
    if (nullptr == crl)
    {
        return "";
    }
    auto crlMutable = const_cast<X509_CRL*>(crl);

    auto bio = crypto::make_unique<BIO>(BIO_new(BIO_s_mem()));
    if (!bio)
    {
        return "";
    }

    if (0 == PEM_write_bio_X509_CRL(bio.get(), crlMutable))
    {
        return "";
    }

    char *dataStart;
    const long nameLength = BIO_get_mem_data(bio.get(), &dataStart);

    if(nameLength <= 0)
    {
        return "";
    }

    std::string ret;
    std::copy_n(dataStart, nameLength, std::back_inserter(ret));
    return ret;
}

std::string X509CrlGenerator::x509CrlToDERString(const X509_CRL *crl)
{
    if (nullptr == crl)
    {
        return "";
    }
    auto crlMutable = const_cast<X509_CRL*>(crl);
    unsigned char *buf = nullptr;
    const auto len = i2d_X509_CRL(crlMutable, &buf);
    if (0 == len)
    {
        return "";
    }

    std::vector<uint8_t> ret(buf, buf + len);
    OPENSSL_free(buf);
    return bytesToHexString(ret);
}

void X509CrlGenerator::addStandardCrlExtensions(const crypto::X509_CRL_uptr& crl, const crypto::X509_uptr& issuerCert) const
{
    auto crlPtr = crl.get();
    auto crtPtr = issuerCert.get();
    const int flags = 0;
    const int atPosition = -1;
    const int nonCritical = 0;

    X509V3_CTX ctx;
    X509V3_set_ctx_nodb(&ctx);
    X509V3_set_ctx(&ctx, crtPtr, nullptr, nullptr, crlPtr, flags);
    auto ext = crypto::make_unique(X509V3_EXT_conf_nid(nullptr, &ctx, NID_authority_key_identifier, const_cast<char *>((const char *)"keyid:always")));
    X509_CRL_add_ext(crlPtr, ext.get(), atPosition);

    auto crlNumber = crypto::make_unique(ASN1_INTEGER_new());
    ASN1_INTEGER_set(crlNumber.get(), 1000);
    X509_CRL_add1_ext_i2d(crlPtr, NID_crl_number, crlNumber.get(), nonCritical, flags);
}

}}}}

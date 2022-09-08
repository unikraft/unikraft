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

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include "X509Constants.h"
#include "ParserUtils.h"

#include <OpensslHelpers/OpensslTypes.h>

#include <algorithm>
#include <iterator>

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

Certificate::Certificate(): _version{},
                            _subject{},
                            _issuer{},
                            _validity{},
                            _extensions{},
                            _signature{},
                            _serialNumber{},
                            _pubKey{},
                            _info{}
{}

bool Certificate::operator==(const Certificate& other) const
{
    return _version == other._version &&
           _subject == other._subject &&
           _issuer == other._issuer &&
           _validity == other._validity &&
           _extensions == other._extensions &&
           _signature == other._signature &&
           _serialNumber == other._serialNumber &&
           _pubKey == other._pubKey &&
           _info == other._info;
}

uint32_t Certificate::getVersion() const
{
    return _version;
}

const std::vector<uint8_t>& Certificate::getSerialNumber() const
{
    return _serialNumber;
}

const DistinguishedName& Certificate::getSubject() const
{
    return _subject;
}

const DistinguishedName& Certificate::getIssuer() const
{
    return _issuer;
}

const Validity& Certificate::getValidity() const
{
    return _validity;
}

const std::vector<Extension>& Certificate::getExtensions() const
{
    return _extensions;
}

const std::vector<uint8_t>& Certificate::getInfo() const
{
    return _info;
}

const Signature& Certificate::getSignature() const
{
    return _signature;
}

const std::vector<uint8_t>& Certificate::getPubKey() const
{
    return _pubKey;
}

const std::string& Certificate::getPem() const
{
    return _pem;
}

Certificate Certificate::parse(const std::string& pem)
{
    return Certificate(pem);
}

// Protected

Certificate::Certificate(const std::string &pem)
{
    _pem = pem;
    crypto::BIO_uptr bio(BIO_new(BIO_s_mem()), ::BIO_free_all);
    BIO_puts(bio.get(), pem.c_str());

    auto x509 = crypto::make_unique(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
    if (!x509) {
        throw FormatException("PEM_read_bio_X509 failed " + getLastError());
    }

    setPublicKey(x509.get());
    setInfo(x509.get());
    setSignature(x509.get());
    setVersion(x509.get());
    setSerialNumber(x509.get());
    setSubject(x509.get());
    setIssuer(x509.get());
    setValidity(x509.get());
    setExtensions(x509.get());
}

// Private

void Certificate::setInfo(X509 *x509)
{
    size_t len = static_cast<size_t>(i2d_re_X509_tbs(x509, NULL));

    _info = std::vector<uint8_t>(len);
    auto info = _info.data();

    i2d_re_X509_tbs(x509, &info);
}

void Certificate::setVersion(const X509 *x509)
{
    // version is zero-indexed thus +1
    _version = static_cast<uint32_t>(X509_get_version(x509) + 1);
}

void Certificate::setSerialNumber(const X509 *x509)
{
    const ASN1_INTEGER *serialNumber = X509_get0_serialNumber(x509);
    const crypto::BIGNUM_uptr bn = crypto::make_unique(ASN1_INTEGER_to_BN(serialNumber, nullptr));

    _serialNumber = bn2Vec(bn.get());
}

void Certificate::setSubject(const X509 *x509)
{
    // this is an internal pointer and must not be freed !
    X509_NAME *subject = X509_get_subject_name(x509);

    if(!subject)
    {
        throw FormatException(getLastError());
    }

    _subject = DistinguishedName(subject);
}

void Certificate::setIssuer(const X509 *x509)
{
    // this is an internal pointer and must not be freed !
    X509_NAME *issuer = X509_get_issuer_name(x509);

    if(!issuer)
    {
        throw FormatException(getLastError());
    }

    _issuer = DistinguishedName(issuer);
}

void Certificate::setValidity(const X509 *x509)
{
    // before 1.1.0 you shall use
    // X509_get_notBefore
    // X509_get_notAfter
    const ASN1_TIME *notBefore = X509_get0_notBefore(x509);
    if(!notBefore)
    {
        throw FormatException(getLastError());
    }

    const ASN1_TIME *notAfter = X509_get0_notAfter(x509);
    if(!notAfter)
    {
        throw FormatException(getLastError());
    }

    auto period = asn1TimePeriodToCTime(notBefore, notAfter);

    _validity = Validity(std::get<0>(period), std::get<1>(period));
}

void Certificate::setExtensions(const X509 *x509)
{
    const int extsCount = X509_get_ext_count(x509);

    if(extsCount < 0)
    {
        throw FormatException(getLastError());
    }

    std::vector<Extension> extensions(static_cast<size_t>(extsCount));
    int index = 0;

    std::generate(extensions.begin(), extensions.end(),
                  [&x509, &index]{ return Extension(X509_get_ext(x509, index++)); });

    std::vector<int> expectedExtensions = constants::REQUIRED_X509_EXTENSIONS;

    for (const auto& extension : extensions)
    {
        expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), extension.getNid()), expectedExtensions.end());
    }

    if (!expectedExtensions.empty())
    {
        std::string err = "Required Certificate extensions not found. Missing [";

        // Convert all but the last element to avoid a trailing ","
        std::for_each(expectedExtensions.begin(), expectedExtensions.end() - 1, [&err](const auto& extension) {
            err += oids::extension2Description(extension) + ", ";
        });

        // Now add the last element with no delimiter
        err += oids::extension2Description(expectedExtensions.back()) + "]";

        throw InvalidExtensionException(err);
    }

    _extensions = extensions;
}

void Certificate::setSignature(const X509 *x509)
{
    // both pointers are internal and must not be freed
    const ASN1_BIT_STRING *psig = nullptr;
    const X509_ALGOR *palg = nullptr;
    X509_get0_signature(&psig, &palg, x509);

    if(!psig || !palg)
    {
        throw FormatException(getLastError());
    }

    if(psig->length == 0)
    {
        throw FormatException("Signature should not be empty");
    }

    _signature = Signature(psig);
}

void Certificate::setPublicKey(const X509 *x509)
{
    ASN1_BIT_STRING* asn1PubKey = X509_get0_pubkey_bitstr(x509);

    if(asn1PubKey == nullptr)
    {
        throw FormatException("Certificate should not be NULL");
    }

    size_t len = static_cast<size_t>(asn1PubKey->length);

    _pubKey = std::vector<uint8_t>(asn1PubKey->data, asn1PubKey->data + len);
}


}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

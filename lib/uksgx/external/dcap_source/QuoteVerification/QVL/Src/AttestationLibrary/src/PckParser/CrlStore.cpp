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


#include "CrlStore.h"
#include "FormatException.h"

#include <OpensslHelpers/Assert.h>

#include <algorithm>

namespace intel { namespace sgx { namespace dcap { namespace pckparser {

CrlStore::CrlStore()
    : _crl{crypto::make_unique(X509_CRL_new())},
      _issuer{},
      _validity{},
      _revoked{},
      _extensions{},
      _signature{},
      _crlNum{}
{
}


bool CrlStore::operator==(const CrlStore& other) const
{

    return (X509_CRL_cmp(&getCrl(), &other.getCrl()) == 0);
}

bool CrlStore::operator!=(const CrlStore& other) const
{
    return !(*this == other);
}

bool CrlStore::parse(const std::string& crlString)
{
    try
    {
        _crl = pckparser::str2X509Crl(crlString);
        QVL_ASSERT(_crl);

        _issuer = pckparser::getIssuer(*_crl);
        _validity = pckparser::getValidity(*_crl);
        _extensions = pckparser::getExtensions(*_crl);
        _revoked = pckparser::getRevoked(*_crl);
        _signature = pckparser::getSignature(*_crl);
        _crlNum = pckparser::getCrlNum(*_crl);
    }
    catch(const FormatException&)
    {
        return false;
    }

    return true;
}

bool CrlStore::expired(const time_t& expirationDate) const
{
    return !_validity.isValid(expirationDate);
}

const Issuer& CrlStore::getIssuer() const
{
    return _issuer;
}

const Validity& CrlStore::getValidity() const
{
    return _validity;
}

const std::vector<Revoked>& CrlStore::getRevoked() const
{
    return _revoked;
}

long CrlStore::getCrlNum() const
{
    return _crlNum;
}

const std::vector<Extension>& CrlStore::getExtensions() const
{
    return _extensions;
}

const Signature& CrlStore::getSignature() const
{
    return _signature;
}

const X509_CRL& CrlStore::getCrl() const
{
    QVL_ASSERT(_crl);
    return *_crl;
}

bool CrlStore::isRevoked(const dcap::parser::x509::Certificate& cert) const
{
    return std::find_if(
            getRevoked().cbegin(),
            getRevoked().cend(), [&](const pckparser::Revoked& revoked){
                return revoked.serialNumber == cert.getSerialNumber();
            }) != getRevoked().cend();
}

}}}} // namespace intel { namespace sgx { namespace dcap { namespace pckparser {

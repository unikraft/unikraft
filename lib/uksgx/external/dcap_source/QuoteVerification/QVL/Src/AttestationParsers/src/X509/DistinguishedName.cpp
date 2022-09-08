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

#include "ParserUtils.h"

#include <openssl/obj_mac.h>

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

DistinguishedName::DistinguishedName(const std::string& raw,
                                     const std::string& commonName,
                                     const std::string& countryName,
                                     const std::string& organizationName,
                                     const std::string& locationName,
                                     const std::string& stateName):
                                        _raw(raw),
                                        _commonName(commonName),
                                        _countryName(countryName),
                                        _organizationName(organizationName),
                                        _locationName(locationName),
                                        _stateName(stateName)
{}

const std::string& DistinguishedName::getRaw() const
{
    return _raw;
}

const std::string& DistinguishedName::getCommonName() const
{
    return _commonName;
}

const std::string& DistinguishedName::getCountryName() const
{
    return _countryName;
}

const std::string& DistinguishedName::getOrganizationName() const
{
    return _organizationName;
}

const std::string& DistinguishedName::getLocationName() const
{
    return _locationName;
}

const std::string& DistinguishedName::getStateName() const
{
    return _stateName;
}

bool DistinguishedName::operator==(const DistinguishedName &other) const {
    return _commonName == other._commonName && // do not compare RAW as order may differ
           _countryName == other._countryName &&
           _organizationName == other._organizationName &&
           _locationName == other._locationName &&
           _stateName == other._stateName;
}

bool DistinguishedName::operator!=(const DistinguishedName &other) const {
    return !operator==(other);
}

// Private

DistinguishedName::DistinguishedName(X509_name_st *x509Name):
                                                      DistinguishedName(x509NameToString(x509Name),
                                                                        getNameEntry(x509Name, NID_commonName),
                                                                        getNameEntry(x509Name, NID_countryName),
                                                                        getNameEntry(x509Name, NID_organizationName),
                                                                        getNameEntry(x509Name, NID_localityName),
                                                                        getNameEntry(x509Name, NID_stateOrProvinceName))
{}

}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

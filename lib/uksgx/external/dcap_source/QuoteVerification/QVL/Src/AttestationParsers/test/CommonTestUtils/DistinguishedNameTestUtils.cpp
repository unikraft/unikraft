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

#include "DistinguishedNameTestUtils.h"

namespace intel { namespace sgx { namespace dcap { namespace parser {

/// Example values for issuer
const std::string RAW_ISSUER("C=US,ST=CA,L=Santa Clara,O=Intel Corporation,CN=Intel SGX PCK Certificate");
const std::string COMMON_NAME_ISSUER("Intel SGX PCK Certificate");
const std::string COUNTRY_NAME_ISSUER("US");
const std::string ORGANIZATION_NAME_ISSUER("Intel Corporation");
const std::string LOCATION_NAME_ISSUER("Santa Clara");
const std::string STATE_NAME_ISSUER("CA");

/// Example values for subject
const std::string RAW_SUBJECT("C=US,ST=CA,L=Santa Clara,O=Intel Corporation,CN=Intel SGX PCK Platform CA");
const std::string COMMON_NAME_SUBJECT("Intel SGX PCK Platform CA");
const std::string COUNTRY_NAME_SUBJECT("US");
const std::string ORGANIZATION_NAME_SUBJECT("Intel Corporation");
const std::string LOCATION_NAME_SUBJECT("Santa Clara");
const std::string STATE_NAME_SUBJECT("CA");

x509::DistinguishedName createDistinguishedName(const std::string& raw,
                                               const std::string& commonName,
                                               const std::string& countryName,
                                               const std::string& organizationName,
                                               const std::string& locationName,
                                               const std::string& stateName)
{
    return x509::DistinguishedName(
            raw,
            commonName,
            countryName,
            organizationName,
            locationName,
            stateName);
}

}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser {

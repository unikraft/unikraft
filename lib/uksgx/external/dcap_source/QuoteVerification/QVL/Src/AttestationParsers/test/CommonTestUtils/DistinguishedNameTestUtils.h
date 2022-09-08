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

#ifndef SGX_DCAP_PARSERS_TEST_DISTINGUISHED_NAME_TEST_UTILS_H
#define SGX_DCAP_PARSERS_TEST_DISTINGUISHED_NAME_TEST_UTILS_H

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include <string>

namespace intel { namespace sgx { namespace dcap { namespace parser {

/// Example values for issuer
extern const std::string RAW_ISSUER;
extern const std::string COMMON_NAME_ISSUER;
extern const std::string COUNTRY_NAME_ISSUER;
extern const std::string ORGANIZATION_NAME_ISSUER;
extern const std::string LOCATION_NAME_ISSUER;
extern const std::string STATE_NAME_ISSUER;

/// Example values for subject
extern const std::string RAW_SUBJECT;
extern const std::string COMMON_NAME_SUBJECT;
extern const std::string COUNTRY_NAME_SUBJECT;
extern const std::string ORGANIZATION_NAME_SUBJECT;
extern const std::string LOCATION_NAME_SUBJECT;
extern const std::string STATE_NAME_SUBJECT;

x509::DistinguishedName createDistinguishedName(const std::string& raw = RAW_ISSUER,
                                                const std::string& commonName = COMMON_NAME_ISSUER,
                                                const std::string& countryName = COUNTRY_NAME_ISSUER,
                                                const std::string& organizationName = ORGANIZATION_NAME_ISSUER,
                                                const std::string& locationName = LOCATION_NAME_ISSUER,
                                                const std::string& stateName = STATE_NAME_ISSUER);
}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser {

#endif // SGX_DCAP_PARSERS_TEST_DISTINGUISHED_NAME_TEST_UTILS_H

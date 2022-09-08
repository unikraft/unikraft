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

#include "X509TestConstants.h"

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace constants {

const x509::DistinguishedName ROOT_CA_SUBJECT
{
        "CN=Intel SGX Root CA, O=Intel Corporation, L=Santa Clara, ST=CA, C=US",

        "Intel SGX Root CA",    // commonName
        "US",                   // countryName
        "Intel Corporation",    // organizationName
        "Santa Clara",          // locationName
        "CA"                    // stateName
};

const x509::DistinguishedName PLATFORM_CA_SUBJECT
{
        "CN=Intel SGX PCK Platform CA, O=Intel Corporation, L=Santa Clara, ST=CA, C=US",

        "Intel SGX PCK Platform CA",    // commonName
        "US",                           // countryName
        "Intel Corporation",            // organizationName
        "Santa Clara",                  // locationName
        "CA"                            // stateName
};

const x509::DistinguishedName PROCESSOR_CA_SUBJECT
{
        "CN=Intel SGX PCK Processor CA, O=Intel Corporation, L=Santa Clara, ST=CA, C=US",

        "Intel SGX PCK Processor CA",   // commonName
        "US",                           // countryName
        "Intel Corporation",            // organizationName
        "Santa Clara",                  // locationName
        "CA"                            // stateName
};

const x509::DistinguishedName PCK_SUBJECT
{
        "CN=Intel SGX PCK Certificate, O=Intel Corporation, L=Santa Clara, ST=CA, C=US",

        "Intel SGX PCK Certificate",    // commonName
        "US",                           // countryName
        "Intel Corporation",            // organizationName
        "Santa Clara",                  // locationName
        "CA"                            // stateName
};

const x509::DistinguishedName TCB_SUBJECT
{
        "CN=Intel SGX TCB Signing, O=Intel Corporation, L=Santa Clara, ST=CA, C=US",

        "Intel SGX TCB Signing",        // commonName
        "US",                           // countryName
        "Intel Corporation",            // organizationName
        "Santa Clara",                  // locationName
        "CA"                            // stateName
};

const std::string SGX_ROOT_CA_CN_PHRASE = "SGX Root CA";
const std::string SGX_INTERMEDIATE_CN_PHRASE = "CA";
const std::string SGX_PCK_CN_PHRASE = "Intel SGX PCK Certificate";
const std::string SGX_TCB_SIGNING_CN_PHRASE = "SGX TCB Signing";

const std::vector<x509::Extension> REQUIRED_X509_EXTENSIONS
{
        x509::Extension(NID_authority_key_identifier, "", {}),
        x509::Extension(NID_crl_distribution_points, "", {}),
        x509::Extension(NID_subject_key_identifier, "", {}),
        x509::Extension(NID_key_usage, "", {}),
        x509::Extension(NID_basic_constraints, "X509v3 Basic Constraints", {})
};

const std::vector<x509::Extension> PCK_X509_EXTENSIONS
{
        x509::Extension(NID_basic_constraints, "X509v3 Basic Constraints", {}),
        x509::Extension(NID_undef, "1.2.840.113741.1.13.1", {}),
        x509::Extension(NID_subject_key_identifier, "X509v3 Subject Key Identifier", {}),
        x509::Extension(NID_key_usage, "X509v3 Key Usage", {}),
        x509::Extension(NID_crl_distribution_points, "X509v3 CRL Distribution Points", {}),
        x509::Extension(NID_authority_key_identifier, "X509v3 Authority Key Identifier", {})
};

}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace constants {
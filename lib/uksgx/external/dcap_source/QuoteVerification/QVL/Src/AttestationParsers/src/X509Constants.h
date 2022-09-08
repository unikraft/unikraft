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

#ifndef SGX_DCAP_PARSERS_X509_CONSTANTS_H_
#define SGX_DCAP_PARSERS_X509_CONSTANTS_H_

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include <openssl/x509v3.h>

#include <string>

namespace intel { namespace sgx { namespace dcap { namespace constants {

const std::vector<int> REQUIRED_X509_EXTENSIONS
{
    NID_authority_key_identifier,
    NID_crl_distribution_points,
    NID_subject_key_identifier,
    NID_key_usage,
    NID_basic_constraints
};

const std::vector<parser::x509::Extension::Type> PCK_REQUIRED_SGX_EXTENSIONS
{
    parser::x509::Extension::Type::PPID,
    parser::x509::Extension::Type::PCEID,
    parser::x509::Extension::Type::FMSPC,
    parser::x509::Extension::Type::SGX_TYPE,
    parser::x509::Extension::Type::TCB,
};

const std::vector<parser::x509::Extension::Type> PLATFORM_PCK_REQUIRED_SGX_EXTENSIONS
{
        parser::x509::Extension::Type::PLATFORM_INSTANCE_ID,
        parser::x509::Extension::Type::CONFIGURATION,
};

const std::vector<parser::x509::Extension::Type> TCB_REQUIRED_SGX_EXTENSIONS
{
    parser::x509::Extension::Type::CPUSVN,
    parser::x509::Extension::Type::PCESVN,
    parser::x509::Extension::Type::SGX_TCB_COMP01_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP02_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP03_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP04_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP05_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP06_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP07_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP08_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP09_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP10_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP11_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP12_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP13_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP14_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP15_SVN,
    parser::x509::Extension::Type::SGX_TCB_COMP16_SVN
};

const std::vector<parser::x509::Extension::Type> CONFIGURATION_REQUIRED_SGX_EXTENSIONS
{
        parser::x509::Extension::Type::DYNAMIC_PLATFORM,
        parser::x509::Extension::Type::CACHED_KEYS,
        parser::x509::Extension::Type::SMT_ENABLED
};

const size_t PPID_BYTE_LEN = 16;
const size_t CPUSVN_BYTE_LEN = 16;
const size_t SGX_TCB_SVN_COMP_BYTE_LEN = 1;
const size_t PCEID_BYTE_LEN = 2;
const size_t PCESVN_BYTE_LEN = 2;
const size_t FMSPC_BYTE_LEN = 6;
const size_t DYNAMIC_PLATFORM_BYTE_LEN = 1;
const size_t PLATFORM_INSTANCE_ID_LEN = 16;
const size_t SGX_TYPE_BYTE_LEN = 1;
const size_t TCB_SEQUENCE_LEN = 18;
const size_t MISCSELECT_BYTE_LEN = 4;
const size_t ATTRIBUTES_BYTE_LEN = 16;
const size_t MRSIGNER_BYTE_LEN = 32;
const size_t MRENCLAVE_BYTE_LEN = 32;
const size_t ECDSA_P256_SIGNATURE_BYTE_LEN = 64;

}}}} // namespace intel { namespace sgx { namespace dcap { namespace constants {

#endif

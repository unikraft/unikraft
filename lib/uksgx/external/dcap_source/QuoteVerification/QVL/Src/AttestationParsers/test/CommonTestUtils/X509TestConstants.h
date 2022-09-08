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

#ifndef SGX_DCAP_PARSERS_TEST_X509_TEST_CONSTANTS_H_
#define SGX_DCAP_PARSERS_TEST_X509_TEST_CONSTANTS_H_

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include <openssl/x509v3.h>

#include <string>
#include <vector>

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace constants {

extern const x509::DistinguishedName ROOT_CA_SUBJECT;
extern const x509::DistinguishedName PLATFORM_CA_SUBJECT;
extern const x509::DistinguishedName PROCESSOR_CA_SUBJECT;
extern const x509::DistinguishedName PCK_SUBJECT;
extern const x509::DistinguishedName TCB_SUBJECT;

extern const std::string SGX_ROOT_CA_CN_PHRASE;
extern const std::string SGX_INTERMEDIATE_CN_PHRASE;
extern const std::string SGX_PCK_CN_PHRASE;
extern const std::string SGX_TCB_SIGNING_CN_PHRASE;

extern const std::vector<x509::Extension> REQUIRED_X509_EXTENSIONS;
extern const std::vector<x509::Extension> PCK_X509_EXTENSIONS;

const size_t PPID_BYTE_LEN = 16;
const size_t CPUSVN_BYTE_LEN = 16;
const size_t SGX_TCB_SVN_COMP_BYTE_LEN = 1;
const size_t PCEID_BYTE_LEN = 2;
const size_t FMSPC_BYTE_LEN = 6;
const size_t DYNAMIC_PLATFORM_BYTE_LEN = 1;
const size_t SGX_TYPE_BYTE_LEN = 1;
const size_t TCB_SEQUENCE_LEN = 18;
const size_t MISCSELECT_BYTE_LEN = 4;
const size_t ATTRIBUTES_BYTE_LEN = 16;
const size_t MRSIGNER_BYTE_LEN = 32;
const size_t MRENCLAVE_BYTE_LEN = 32;

}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace constants {

#endif //SGX_DCAP_PARSERS_TEST_X509_TEST_CONSTANTS_H_

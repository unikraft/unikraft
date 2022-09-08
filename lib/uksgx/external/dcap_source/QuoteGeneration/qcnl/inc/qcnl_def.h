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
#ifndef QCNLDEF_H_
#define QCNLDEF_H_
#pragma once

namespace consts {
constexpr char CA_PLATFORM[] = "platform";
constexpr char CA_PROCESSOR[] = "processor";
constexpr uint32_t QE3_ID_SIZE = 16;
constexpr uint32_t ENC_PPID_SIZE = 384;
constexpr uint32_t CPUSVN_SIZE = 16;
constexpr uint32_t PCESVN_SIZE = 2;
constexpr uint32_t PCEID_SIZE = 2;
constexpr uint32_t FMSPC_SIZE = 6;
constexpr uint32_t PLATFORM_MANIFEST_SIZE = 53000;
} // namespace consts

// Headers in Intel PCS's http response are case insensitive
namespace intelpcs {
constexpr char PCK_CERT_ISSUER_CHAIN[] = "sgx-pck-certificate-issuer-chain";
constexpr char CRL_ISSUER_CHAIN[] = "sgx-pck-crl-issuer-chain";
constexpr char SGX_TCB_INFO_ISSUER_CHAIN[] = "sgx-tcb-info-issuer-chain";
constexpr char TCB_INFO_ISSUER_CHAIN[] = "tcb-info-issuer-chain";
constexpr char SGX_TCBM[] = "sgx-tcbm";
constexpr char ENCLAVE_ID_ISSUER_CHAIN[] = "sgx-enclave-identity-issuer-chain";
constexpr char REQUEST_ID[] = "request-id";
} // namespace intelpcs

// Azure returns the headers in JSON format inside body, so they are case sensitive
namespace azurepccs {
constexpr char PCK_CERT[] = "pckCert";
constexpr char PCK_CERT_ISSUER_CHAIN[] = "sgx-Pck-Certificate-Issuer-Chain";
constexpr char SGX_TCBM[] = "sgx-Tcbm";
// constexpr char CRL_ISSUER_CHAIN[] = "SGX-PCK-CRL-Issuer-Chain";
// constexpr char SGX_TCB_INFO_ISSUER_CHAIN[] = "SGX-TCB-Info-Issuer-Chain";
// constexpr char TCB_INFO_ISSUER_CHAIN[] = "TCB-Info-Issuer-Chain";
// constexpr char ENCLAVE_ID_ISSUER_CHAIN[] = "SGX-Enclave-Identity-Issuer-Chain";
} // namespace azurepccs

#endif
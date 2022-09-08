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

#ifndef INTEL_SGX_QVL_QUOTE_CONSTANTS_H_
#define INTEL_SGX_QVL_QUOTE_CONSTANTS_H_

#include <array>

// includes needed on Fedora 32
#include <cstddef>
#include <cstdint>

namespace intel { namespace sgx { namespace dcap { namespace constants {

const uint32_t TEE_TYPE_SGX = 0x00000000;
const uint32_t TEE_TYPE_TDX = 0x00000081;
const uint16_t QUOTE_VERSION_3 = 3;
const uint16_t QUOTE_VERSION_4 = 4;

const uint16_t ECDSA_256_WITH_P256_CURVE = 2;
const uint16_t ECDSA_384_WITH_P384_CURVE = 3;
constexpr size_t ECDSA_P256_SIGNATURE_BYTE_LEN = 64;
constexpr size_t ENCLAVE_REPORT_BYTE_LEN = 384;
constexpr size_t TD_REPORT_BYTE_LEN = 584;

const uint16_t PCK_ID_PLAIN_PPID = 1;
const uint16_t PCK_ID_ENCRYPTED_PPID_2048 = 2;
const uint16_t PCK_ID_ENCRYPTED_PPID_3072 = 3;
const uint16_t PCK_ID_PCK_CERTIFICATE = 4;
const uint16_t PCK_ID_PCK_CERT_CHAIN = 5;
const uint16_t PCK_ID_QE_REPORT_CERTIFICATION_DATA = 6;

const std::array<uint16_t, 2> ALLOWED_QUOTE_VERSIONS = {{ QUOTE_VERSION_3, QUOTE_VERSION_4 }};
const std::array<uint32_t, 2> ALLOWED_TEE_TYPES = {{ TEE_TYPE_SGX, TEE_TYPE_TDX }};
const std::array<uint16_t, 1> ALLOWED_ATTESTATION_KEY_TYPES = {{ ECDSA_256_WITH_P256_CURVE }};
const std::array<uint8_t, 16> INTEL_QE_VENDOR_ID = {{ 0x93, 0x9A, 0x72, 0x33, 0xF7, 0x9C, 0x4C, 0xA9, 0x94, 0x0A, 0x0D, 0xB3, 0x95, 0x7F, 0x06, 0x07 }};

    constexpr size_t HEADER_BYTE_LEN = 48;
    constexpr size_t AUTH_DATA_SIZE_BYTE_LEN = 4;

    constexpr size_t ECDSA_SIGNATURE_BYTE_LEN = 64;
    constexpr size_t ECDSA_PUBKEY_BYTE_LEN = 64;
    constexpr size_t QE_REPORT_BYTE_LEN = ENCLAVE_REPORT_BYTE_LEN;
    constexpr size_t QE_REPORT_SIG_BYTE_LEN = ECDSA_SIGNATURE_BYTE_LEN;
    constexpr size_t CERTIFICATION_DATA_TYPE_BYTE_LEN = 2;
    constexpr size_t CERTIFICATION_DATA_SIZE_BYTE_LEN = 4;
    constexpr size_t QE_AUTH_DATA_SIZE_BYTE_LEN = 2;
    constexpr size_t QE_CERT_DATA_TYPE_BYTE_LEN = 2;
    constexpr size_t QE_CERT_DATA_SIZE_BYTE_LEN = 4;

    constexpr size_t AUTH_DATA_MIN_BYTE_LEN =
            ECDSA_SIGNATURE_BYTE_LEN +
            ECDSA_PUBKEY_BYTE_LEN +
            QE_REPORT_BYTE_LEN +
            QE_REPORT_SIG_BYTE_LEN +
            QE_AUTH_DATA_SIZE_BYTE_LEN +
            QE_CERT_DATA_TYPE_BYTE_LEN +
            QE_CERT_DATA_SIZE_BYTE_LEN;

    constexpr size_t QUOTE_MIN_BYTE_LEN = // Actual minimal size is a Quote V3 with Enclave report
            HEADER_BYTE_LEN +
            ENCLAVE_REPORT_BYTE_LEN +
            AUTH_DATA_SIZE_BYTE_LEN +
            AUTH_DATA_MIN_BYTE_LEN;

}}}} // namespace intel { namespace sgx { namespace dcap { namespace constants {

#endif

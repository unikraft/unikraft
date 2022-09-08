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

#ifndef SGXECDSAATTESTATION_TEST_QUOTEV4_GENERATOR_H_
#define SGXECDSAATTESTATION_TEST_QUOTEV4_GENERATOR_H_

#include <cstdint>
#include <array>
#include <vector>
#include <OpensslHelpers/Bytes.h>

namespace intel { namespace sgx { namespace dcap { namespace test {

class QuoteV4Generator {
public:

    struct QuoteHeader
    {
        uint16_t version = 4;
        uint16_t attestationKeyType{};
        uint32_t teeType{};
        uint32_t reserved{};
        std::array<uint8_t, 16> qeVendorId{};
        std::array<uint8_t, 20> userData{};

        Bytes bytes() const;
    };

    struct EnclaveReport
    {
        std::array<uint8_t, 16> cpuSvn;
        uint32_t miscSelect;
        std::array<uint8_t, 28> reserved1;
        std::array<uint8_t, 16> attributes;
        std::array<uint8_t, 32> mrEnclave;
        std::array<uint8_t, 32> reserved2;
        std::array<uint8_t, 32> mrSigner;
        std::array<uint8_t, 96> reserved3;
        uint16_t isvProdID;
        uint16_t isvSvn;
        std::array<uint8_t, 60> reserved4;
        std::array<uint8_t, 64> reportData;

        Bytes bytes() const;
    };

    struct TDReport
    {
        std::array<uint8_t, 16> teeTcbSvn;
        std::array<uint8_t, 48> mrSeam;
        std::array<uint8_t, 48> mrSignerSeam;
        std::array<uint8_t, 8> seamAttributes;
        std::array<uint8_t, 8> tdAttributes;
        std::array<uint8_t, 8> xFAM;
        std::array<uint8_t, 48> mrTd;
        std::array<uint8_t, 48> mrConfigId;
        std::array<uint8_t, 48> mrOwner;
        std::array<uint8_t, 48> mrOwnerConfig;
        std::array<uint8_t, 48> rtMr0;
        std::array<uint8_t, 48> rtMr1;
        std::array<uint8_t, 48> rtMr2;
        std::array<uint8_t, 48> rtMr3;
        std::array<uint8_t, 64> reportData;

        Bytes bytes() const;
    };

    struct EcdsaSignature
    {
        std::array<uint8_t, 64> signature;
        Bytes bytes() const;
    };

    struct EcdsaPublicKey
    {
        std::array<uint8_t, 64> publicKey;
        Bytes bytes() const;
    };

    struct QeAuthData
    {
        uint16_t size;
        Bytes data;

        Bytes bytes() const;
    };

    struct CertificationData
    {
        uint16_t keyDataType;
        uint32_t size;
        Bytes keyData;

        Bytes bytes() const;
    };

    struct QuoteAuthData
    {
        uint32_t authDataSize = 134;

        EcdsaSignature ecdsaSignature;
        EcdsaPublicKey ecdsaAttestationKey;
        CertificationData certificationData;

        Bytes bytes() const;
    };

    struct QEReportCertificationData
    {
        EnclaveReport qeReport;
        EcdsaSignature qeReportSignature;

        QeAuthData qeAuthData;
        CertificationData certificationData;

        Bytes bytes() const;
    };

    QuoteV4Generator();
 
    QuoteV4Generator& withHeader(const QuoteHeader& header);
    QuoteV4Generator& withEnclaveReport(const EnclaveReport& _body);
    QuoteV4Generator& withTDReport(const TDReport& _body);
    QuoteV4Generator& withAuthDataSize(uint32_t size);
    QuoteV4Generator& withAuthData(const QuoteAuthData& authData);

    QuoteHeader& getHeader() {return header;}
    EnclaveReport& getEnclaveReport() {return enclaveReport;}
    TDReport& getTdReport() {return tdReport;}
    uint32_t& getAuthSize() {return quoteAuthData.authDataSize;}
    QuoteAuthData& getAuthData() {return quoteAuthData;}

    // auth data utils  
    QuoteV4Generator& withQuoteSignature(const EcdsaSignature& signature);
    QuoteV4Generator& withAttestationKey(const EcdsaPublicKey& pubKey);
    QuoteV4Generator& withCertificationData(const CertificationData& certificationData);
    QuoteV4Generator& withCertificationData(uint16_t type, const Bytes& keyData);

    Bytes buildSgxQuote();
    Bytes buildTdxQuote();

private:
    QuoteHeader header;
    EnclaveReport enclaveReport;
    TDReport tdReport;
    QuoteAuthData quoteAuthData;
};

}}}} // namespace intel { namespace sgx { namespace dcap { namespace test {


#endif //SGXECDSAATTESTATION_TEST_QUOTEV4_GENERATOR_H_

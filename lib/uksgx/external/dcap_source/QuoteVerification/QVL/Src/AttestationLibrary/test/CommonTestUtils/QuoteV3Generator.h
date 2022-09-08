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

#ifndef SGXECDSAATTESTATION_TEST_QUOTEV3_GENERATOR_H_
#define SGXECDSAATTESTATION_TEST_QUOTEV3_GENERATOR_H_

#include <cstdint>
#include <array>
#include <vector>
#include <OpensslHelpers/Bytes.h>

namespace intel { namespace sgx { namespace dcap { namespace test {

class QuoteV3Generator {
public:

    struct QuoteHeader
    {
        uint16_t version = 3;
        uint16_t attestationKeyType;
        uint16_t teeType;
        uint16_t reserved;
        uint16_t qeSvn;
        uint16_t pceSvn;
        std::array<uint8_t, 16> qeVendorId;
        std::array<uint8_t, 20> userData;

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
        uint32_t authDataSize;

        EcdsaSignature ecdsaSignature;
        EcdsaPublicKey ecdsaAttestationKey;
        
        EnclaveReport qeReport;
        EcdsaSignature qeReportSignature;

        QeAuthData qeAuthData;
        CertificationData certificationData;

        Bytes bytes() const;
    };


    QuoteV3Generator();
 
    QuoteV3Generator& withHeader(const QuoteHeader& header);
    QuoteV3Generator& withEnclaveReport(const EnclaveReport& _body);
    QuoteV3Generator& withAuthDataSize(uint32_t size);
    QuoteV3Generator& withAuthData(const QuoteAuthData& authData);

    QuoteHeader& getHeader() {return header;}
    EnclaveReport& getEnclaveReport() {return enclaveReport;}
    uint32_t& getAuthSize() {return quoteAuthData.authDataSize;}
    QuoteAuthData& getAuthData() {return quoteAuthData;}


    // header utils   
    QuoteV3Generator& withQeSvn(uint16_t qeSvn);
    QuoteV3Generator& withPceSvn(uint16_t pceSvn);

    // auth data utils  
    QuoteV3Generator& withQuoteSignature(const EcdsaSignature& signature);
    QuoteV3Generator& withAttestationKey(const EcdsaPublicKey& pubKey);
    QuoteV3Generator& withQeReport(const EnclaveReport& report);
    QuoteV3Generator& withQeReportSignature(const EcdsaSignature& sign);

    QuoteV3Generator& withQeAuthData(const QuoteV3Generator::QeAuthData& qeAuth);
    QuoteV3Generator& withQeAuthData(const Bytes& report);
    QuoteV3Generator& withcertificationData(const CertificationData& certificationData);
    QuoteV3Generator& withCertificationData(uint16_t type, const Bytes& keyData);

    Bytes buildQuote();
    Bytes buildTdxQuote();

private:  
    QuoteHeader header;
    EnclaveReport enclaveReport;
    QuoteAuthData quoteAuthData;
};

}}}} // namespace intel { namespace sgx { namespace dcap { namespace test {


#endif //SGXECDSAATTESTATION_TEST_QUOTEV3_GENERATOR_H_

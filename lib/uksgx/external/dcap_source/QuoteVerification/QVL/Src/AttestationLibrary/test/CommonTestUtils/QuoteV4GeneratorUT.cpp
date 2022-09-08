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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>

#include <QuoteVerification/Quote.h>
#include <SgxEcdsaAttestation/QuoteVerification.h>
#include "QuoteV4Generator.h"
#include "QuoteUtils.h"

using namespace testing;
using namespace ::intel::sgx::dcap;

struct QuoteV4GeneratorTests : public Test
{
};

namespace {

template<size_t N>
std::array<uint8_t, N> randomByteArray()
{
    std::array<uint8_t, N> arr{};
    for(size_t i=0; i<N; i++)
        arr[i] = static_cast<uint8_t>(rand() % 256);
    return arr;
}

bool areBytesAtPosition(Bytes container, size_t position, const Bytes& bytes)
{
    if (bytes.size() + position > container.size())
    {
        return false;
    }
    auto sliceBegin = std::next(container.cbegin(), (long) position);
    auto sliceEnd = std::next(sliceBegin, (long) bytes.size());
    Bytes slice(sliceBegin, sliceEnd);
    return bytes == slice;
}

MATCHER_P2(DataAtPositionEq, position, data,
    std::string("Data at position " + PrintToString(position) + (negation ? " isn't" : " is") + " equal to " + PrintToString(data)))
{
    auto bytes = test::toBytes(data);
    return areBytesAtPosition(arg, (size_t) position, bytes);
}

MATCHER_P2(BytesAtPositionEq, position, bytes,
    std::string("Data at position " + PrintToString(position) + (negation ? " isn't" : " is") + " equal to " + PrintToString(bytes)))
{
    return areBytesAtPosition(arg, position, bytes);
}

// Header
constexpr size_t VERSION_POSITION_IN_QUOTE = 0;
constexpr size_t ATTESTATION_KEY_TYPE_POSITION_IN_QUOTE = VERSION_POSITION_IN_QUOTE + 2;
constexpr size_t TEE_TYPE_POSITION_IN_QUOTE = ATTESTATION_KEY_TYPE_POSITION_IN_QUOTE + 2;
constexpr size_t RESERVED_POSITION_IN_QUOTE = TEE_TYPE_POSITION_IN_QUOTE + 4;
constexpr size_t QE_VENDOR_ID_POSITION_IN_QUOTE = RESERVED_POSITION_IN_QUOTE + 4;
constexpr size_t USER_DATA_POSITION_IN_QUOTE = QE_VENDOR_ID_POSITION_IN_QUOTE + 16;

// Enclave Report
constexpr size_t CPUSVN_POSITION_IN_QUOTE = USER_DATA_POSITION_IN_QUOTE + 20;
constexpr size_t MISCSELECT_POSITION_IN_QUOTE = CPUSVN_POSITION_IN_QUOTE + 16;
constexpr size_t RESERVED1_POSITION_IN_QUOTE = MISCSELECT_POSITION_IN_QUOTE + 4;
constexpr size_t ATTRIBUTES_POSITION_IN_QUOTE = RESERVED1_POSITION_IN_QUOTE + 28;
constexpr size_t MRENCLAVE_POSITION_IN_QUOTE = ATTRIBUTES_POSITION_IN_QUOTE + 16;
constexpr size_t RESERVED2_POSITION_IN_QUOTE = MRENCLAVE_POSITION_IN_QUOTE + 32;
constexpr size_t MRSIGNER_POSITION_IN_QUOTE = RESERVED2_POSITION_IN_QUOTE + 32;
constexpr size_t RESERVED3_POSITION_IN_QUOTE = MRSIGNER_POSITION_IN_QUOTE + 32;
constexpr size_t ISVPRODID_POSITION_IN_QUOTE = RESERVED3_POSITION_IN_QUOTE + 96;
constexpr size_t ISVSVN_POSITION_IN_QUOTE = ISVPRODID_POSITION_IN_QUOTE + 2;
constexpr size_t RESERVED4_POSITION_IN_QUOTE = ISVSVN_POSITION_IN_QUOTE + 2;
constexpr size_t REPORTDATA_POSITION_IN_QUOTE = RESERVED4_POSITION_IN_QUOTE + 60;

// TD Report
constexpr size_t TEE_TCB_SVN_POSITION_IN_QUOTE = USER_DATA_POSITION_IN_QUOTE + 20;
constexpr size_t MRSEAM_POSITION_IN_QUOTE = TEE_TCB_SVN_POSITION_IN_QUOTE + 16;
constexpr size_t MRSIGNERSEAM_POSITION_IN_QUOTE = MRSEAM_POSITION_IN_QUOTE + 48;
constexpr size_t SEAMATTRIBUTES_POSITION_IN_QUOTE = MRSIGNERSEAM_POSITION_IN_QUOTE + 48;
constexpr size_t TDATTRIBUTES_POSITION_IN_QUOTE = SEAMATTRIBUTES_POSITION_IN_QUOTE + 8;
constexpr size_t XFAM_POSITION_IN_QUOTE = TDATTRIBUTES_POSITION_IN_QUOTE + 8;
constexpr size_t MRTD_POSITION_IN_QUOTE = XFAM_POSITION_IN_QUOTE + 8;
constexpr size_t MRCONFIGID_POSITION_IN_QUOTE = MRTD_POSITION_IN_QUOTE + 48;
constexpr size_t MROWNER_POSITION_IN_QUOTE = MRCONFIGID_POSITION_IN_QUOTE + 48;
constexpr size_t MROWNERCONFIG_POSITION_IN_QUOTE = MROWNER_POSITION_IN_QUOTE + 48;
constexpr size_t RTMR0_POSITION_IN_QUOTE = MROWNERCONFIG_POSITION_IN_QUOTE + 48;
constexpr size_t RTMR1_POSITION_IN_QUOTE = RTMR0_POSITION_IN_QUOTE + 48;
constexpr size_t RTMR2_POSITION_IN_QUOTE = RTMR1_POSITION_IN_QUOTE + 48;
constexpr size_t RTMR3_POSITION_IN_QUOTE = RTMR2_POSITION_IN_QUOTE + 48;
constexpr size_t TD_REPORTDATA_POSITION_IN_QUOTE = RTMR3_POSITION_IN_QUOTE + 48;

// Auth Data Size
constexpr size_t AUTH_DATA_SIZE_POSITION_IN_SGX_QUOTE = REPORTDATA_POSITION_IN_QUOTE + 64;
constexpr size_t AUTH_DATA_SIZE_POSITION_IN_TDX_QUOTE = TD_REPORTDATA_POSITION_IN_QUOTE + 64;

// Auth Data
constexpr size_t QUOTE_SIGNATURE_POSITION_SGX_QUOTE = AUTH_DATA_SIZE_POSITION_IN_SGX_QUOTE + 4;
constexpr size_t QUOTE_SIGNATURE_POSITION_TDX_QUOTE = AUTH_DATA_SIZE_POSITION_IN_TDX_QUOTE + 4;
constexpr size_t ATTESTATION_KEY_POSITION_SGX_QUOTE = QUOTE_SIGNATURE_POSITION_SGX_QUOTE + 64;
constexpr size_t ATTESTATION_KEY_POSITION_TDX_QUOTE = QUOTE_SIGNATURE_POSITION_TDX_QUOTE + 64;
constexpr size_t CERTIFICATION_DATA_POSITION_SGX_QUOTE = ATTESTATION_KEY_POSITION_SGX_QUOTE + 64;
constexpr size_t CERTIFICATION_DATA_POSITION_TDX_QUOTE = ATTESTATION_KEY_POSITION_TDX_QUOTE + 64;

} //anonymous namespace

TEST_F(QuoteV4GeneratorTests, shouldProvideGeneratedBinaryQuote)
{
    test::QuoteV4Generator generator;
    auto quote = generator.buildSgxQuote();
    EXPECT_THAT(quote, SizeIs(test::QUOTE_V4_MINIMAL_SIZE));
}

TEST_F(QuoteV4GeneratorTests, shouldAllowSettingHeader)
{
    uint16_t version = 1;
    uint16_t attestationKeyType = 2;
    uint32_t teeType = 3;
    uint32_t reserved = 4;
    auto qeVendor = randomByteArray<16>();
    auto userData = randomByteArray<20>();

    test::QuoteV4Generator generator;
    test::QuoteV4Generator::QuoteHeader header{};
    header.version = version;
    header.attestationKeyType = attestationKeyType;
    header.teeType = teeType;
    header.reserved = reserved;
    header.qeVendorId = qeVendor;
    header.userData = userData;

    generator.withHeader(header);
    auto sgxQuote = generator.buildSgxQuote();
    auto tdxQuote = generator.buildTdxQuote();

    EXPECT_THAT(sgxQuote, DataAtPositionEq(VERSION_POSITION_IN_QUOTE, version));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(ATTESTATION_KEY_TYPE_POSITION_IN_QUOTE, attestationKeyType));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(TEE_TYPE_POSITION_IN_QUOTE, teeType));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(RESERVED_POSITION_IN_QUOTE, reserved));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(QE_VENDOR_ID_POSITION_IN_QUOTE, qeVendor));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(USER_DATA_POSITION_IN_QUOTE, userData));

    EXPECT_THAT(tdxQuote, DataAtPositionEq(VERSION_POSITION_IN_QUOTE, version));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(ATTESTATION_KEY_TYPE_POSITION_IN_QUOTE, attestationKeyType));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(TEE_TYPE_POSITION_IN_QUOTE, teeType));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(RESERVED_POSITION_IN_QUOTE, reserved));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(QE_VENDOR_ID_POSITION_IN_QUOTE, qeVendor));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(USER_DATA_POSITION_IN_QUOTE, userData));
}

TEST_F(QuoteV4GeneratorTests, shouldAllowSettingEnclaveReport)
{
    auto cpuSvn = randomByteArray<16>();
    uint32_t miscSelect = 1;
    auto reserved1 = randomByteArray<28>();
    auto attributes = randomByteArray<16>();
    auto mrenclave = randomByteArray<32>();
    auto reserved2 = randomByteArray<32>();
    auto mrsigner = randomByteArray<32>();
    auto reserved3 = randomByteArray<96>();
    uint16_t isvProdId = 2;
    uint16_t isvSvn = 3;
    auto reserved4 = randomByteArray<60>();
    auto reportedData = randomByteArray<64>();

    test::QuoteV4Generator generator;
    test::QuoteV4Generator::EnclaveReport enclaveReport{};

    enclaveReport.cpuSvn = cpuSvn;
    enclaveReport.miscSelect = miscSelect;
    enclaveReport.reserved1 = reserved1;
    enclaveReport.attributes = attributes;
    enclaveReport.mrEnclave = mrenclave;
    enclaveReport.reserved2 = reserved2;
    enclaveReport.mrSigner = mrsigner;
    enclaveReport.reserved3 = reserved3;
    enclaveReport.isvProdID = isvProdId;
    enclaveReport.isvSvn = isvSvn;
    enclaveReport.reserved4 = reserved4;
    enclaveReport.reportData = reportedData;

    generator.withEnclaveReport(enclaveReport);
    auto sgxQuote = generator.buildSgxQuote();

    EXPECT_THAT(sgxQuote, DataAtPositionEq(CPUSVN_POSITION_IN_QUOTE, cpuSvn));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(MISCSELECT_POSITION_IN_QUOTE, miscSelect));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(RESERVED1_POSITION_IN_QUOTE, reserved1));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(ATTRIBUTES_POSITION_IN_QUOTE, attributes));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(MRENCLAVE_POSITION_IN_QUOTE, mrenclave));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(RESERVED2_POSITION_IN_QUOTE, reserved2));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(MRSIGNER_POSITION_IN_QUOTE, mrsigner));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(RESERVED3_POSITION_IN_QUOTE, reserved3));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(ISVPRODID_POSITION_IN_QUOTE, isvProdId));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(ISVSVN_POSITION_IN_QUOTE, isvSvn));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(RESERVED4_POSITION_IN_QUOTE, reserved4));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(REPORTDATA_POSITION_IN_QUOTE, reportedData));
}

TEST_F(QuoteV4GeneratorTests, shouldAllowSettingTDReport)
{
    auto teeTcbSvn = randomByteArray<16>();
    auto mrSeam = randomByteArray<48>();
    auto mrSignerSeam = randomByteArray<48>();
    auto seamAttributes = randomByteArray<8>();
    auto tdAttributes = randomByteArray<8>();
    auto xFAM = randomByteArray<8>();
    auto mrTd = randomByteArray<48>();
    auto mrConfigId = randomByteArray<48>();
    auto mrOwner = randomByteArray<48>();
    auto mrOwnerConfig = randomByteArray<48>();
    auto rtMr0 = randomByteArray<48>();
    auto rtMr1 = randomByteArray<48>();
    auto rtMr2 = randomByteArray<48>();
    auto rtMr3 = randomByteArray<48>();
    auto reportedData = randomByteArray<64>();

    test::QuoteV4Generator generator;
    test::QuoteV4Generator::TDReport tdReport{};

    tdReport.teeTcbSvn = teeTcbSvn;
    tdReport.mrSeam = mrSeam;
    tdReport.mrSignerSeam = mrSignerSeam;
    tdReport.seamAttributes = seamAttributes;
    tdReport.tdAttributes = tdAttributes;
    tdReport.xFAM = xFAM;
    tdReport.mrTd = mrTd;
    tdReport.mrConfigId = mrConfigId;
    tdReport.mrOwner = mrOwner;
    tdReport.mrOwnerConfig = mrOwnerConfig;
    tdReport.rtMr0 = rtMr0;
    tdReport.rtMr1 = rtMr1;
    tdReport.rtMr2 = rtMr2;
    tdReport.rtMr3 = rtMr3;
    tdReport.reportData = reportedData;

    generator.withTDReport(tdReport);
    auto tdxQuote = generator.buildTdxQuote();

    EXPECT_THAT(tdxQuote, DataAtPositionEq(TEE_TCB_SVN_POSITION_IN_QUOTE, teeTcbSvn));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(MRSEAM_POSITION_IN_QUOTE, mrSeam));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(MRSIGNERSEAM_POSITION_IN_QUOTE, mrSignerSeam));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(SEAMATTRIBUTES_POSITION_IN_QUOTE, seamAttributes));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(TDATTRIBUTES_POSITION_IN_QUOTE, tdAttributes));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(XFAM_POSITION_IN_QUOTE, xFAM));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(MRTD_POSITION_IN_QUOTE, mrTd));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(MRCONFIGID_POSITION_IN_QUOTE, mrConfigId));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(MROWNER_POSITION_IN_QUOTE, mrOwner));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(MROWNERCONFIG_POSITION_IN_QUOTE, mrOwnerConfig));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(RTMR0_POSITION_IN_QUOTE, rtMr0));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(RTMR1_POSITION_IN_QUOTE, rtMr1));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(RTMR2_POSITION_IN_QUOTE, rtMr2));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(RTMR3_POSITION_IN_QUOTE, rtMr3));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(TD_REPORTDATA_POSITION_IN_QUOTE, reportedData));
}

TEST_F(QuoteV4GeneratorTests, shouldAllowSettingAuthDataSize)
{
    std::array<uint8_t, 4> authDataSize = { 0x15, 0x03, 0,  0}; // LE 789

    test::QuoteV4Generator generator;

    generator.withAuthDataSize(789);
    auto tdxQuote = generator.buildTdxQuote();
    auto sgxQuote = generator.buildSgxQuote();

    EXPECT_THAT(tdxQuote, DataAtPositionEq(AUTH_DATA_SIZE_POSITION_IN_TDX_QUOTE, authDataSize));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(AUTH_DATA_SIZE_POSITION_IN_SGX_QUOTE, authDataSize));
}

TEST_F(QuoteV4GeneratorTests, shouldAllowSettingAuthData)
{
    test::QuoteV4Generator::QuoteAuthData authData = {};
    std::array<uint8_t, 4> authDataSize = { 0x15, 0x03, 0,  0}; // LE 789
    auto ecdsaSignature = test::QuoteV4Generator::EcdsaSignature{};
    ecdsaSignature.signature = randomByteArray<64>();
    auto ecdsaPublicKey = test::QuoteV4Generator::EcdsaPublicKey{};
    ecdsaPublicKey.publicKey = randomByteArray<64>();
    auto certificationData = test::QuoteV4Generator::CertificationData{};
    certificationData.keyDataType = 254;
    auto tmp = randomByteArray<596>();
    certificationData.keyData = std::vector<uint8_t>(tmp.begin(), tmp.end());
    certificationData.size = 255;

    test::QuoteV4Generator generator;
    authData.authDataSize = 789;
    authData.ecdsaSignature = ecdsaSignature;
    authData.ecdsaAttestationKey = ecdsaPublicKey;
    authData.certificationData = certificationData;

    generator.withAuthData(authData);
    auto tdxQuote = generator.buildTdxQuote();
    auto sgxQuote = generator.buildSgxQuote();

    EXPECT_THAT(tdxQuote, DataAtPositionEq(AUTH_DATA_SIZE_POSITION_IN_TDX_QUOTE, authDataSize));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(QUOTE_SIGNATURE_POSITION_TDX_QUOTE, ecdsaSignature.signature));
    EXPECT_THAT(tdxQuote, DataAtPositionEq(ATTESTATION_KEY_POSITION_TDX_QUOTE, ecdsaPublicKey.publicKey));
    EXPECT_THAT(tdxQuote, BytesAtPositionEq(CERTIFICATION_DATA_POSITION_TDX_QUOTE, certificationData.bytes()));

    EXPECT_THAT(sgxQuote, DataAtPositionEq(AUTH_DATA_SIZE_POSITION_IN_SGX_QUOTE, authDataSize));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(QUOTE_SIGNATURE_POSITION_SGX_QUOTE, ecdsaSignature.signature));
    EXPECT_THAT(sgxQuote, DataAtPositionEq(ATTESTATION_KEY_POSITION_SGX_QUOTE, ecdsaPublicKey.publicKey));
    EXPECT_THAT(sgxQuote, BytesAtPositionEq(CERTIFICATION_DATA_POSITION_SGX_QUOTE, certificationData.bytes()));
}

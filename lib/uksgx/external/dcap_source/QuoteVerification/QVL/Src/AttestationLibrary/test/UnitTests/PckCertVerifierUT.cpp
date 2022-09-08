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

#include <SgxEcdsaAttestation/QuoteVerification.h>
#include <Verifiers/PckCertVerifier.h>

#include "Mocks/CertCrlStoresMocks.h"
#include "CertVerification/X509Constants.h"

using namespace intel::sgx;
/*
TEST(PckVerifier, shouldReturnRootCaMissingWhenNoRootCaFound)
{
    // GIVEN
    auto chainRoot = std::make_shared<const dcap::test::PckCertificateMock>();
    const auto trustedRoot = std::make_shared<const dcap::test::PckCertificateMock>();
    dcap::test::CertificateChainMock chain;

    EXPECT_CALL(chain, get(dcap::test::constants::ROOT_CA_SUBJECT))
            .WillRepeatedly(testing::Return(nullptr));

    // WHEN
    const auto actual = dcap::PckCertVerifier{}.verify(chain, *trustedRoot);

    // THEN
    EXPECT_EQ(STATUS_SGX_ROOT_CA_MISSING, actual);
}

TEST(PckVerifier, shouldReturnIntermediateMissingWhenPlatformOrProcessorCaNotFound)
{
    // GIVEN
    const auto chainRoot = std::make_shared<const dcap::test::PckCertificateMock>();
    const auto intermediate = std::make_shared<const dcap::test::PckCertificateMock>();
    const auto pck = std::make_shared<const dcap::test::PckCertificateMock>();
    const auto trustedRoot = std::make_shared<const dcap::test::PckCertificateMock>();
    dcap::test::CertificateChainMock chain;

    EXPECT_CALL(chain, get(dcap::test::constants::ROOT_CA_SUBJECT))
            .WillRepeatedly(testing::Return(chainRoot));

    EXPECT_CALL(chain, get(dcap::test::constants::PLATFORM_CA_SUBJECT))
            .WillRepeatedly(testing::Return(nullptr));

    EXPECT_CALL(chain, get(dcap::test::constants::PROCESSOR_CA_SUBJECT))
            .WillRepeatedly(testing::Return(nullptr));

    // WHEN
    const auto actual = dcap::PckCertVerifier{}.verify(chain, *trustedRoot);

    // THEN
    EXPECT_EQ(STATUS_SGX_INTERMEDIATE_CA_MISSING, actual);
}

TEST(PckVerifier, shouldReturnPckMissingWhenPckCertNotFound)
{
    // GIVEN
    const auto chainRoot = std::make_shared<const dcap::test::PckCertificateMock>();
    const auto intermediate = std::make_shared<const dcap::test::PckCertificateMock>();
    const auto pck = std::make_shared<const dcap::test::PckCertificateMock>();
    const auto trustedRoot = std::make_shared<const dcap::test::PckCertificateMock>();
    dcap::test::CertificateChainMock chain;

    EXPECT_CALL(chain, get(dcap::test::constants::ROOT_CA_SUBJECT))
            .WillRepeatedly(testing::Return(chainRoot));

    EXPECT_CALL(chain, get(dcap::test::constants::PLATFORM_CA_SUBJECT))
            .WillRepeatedly(testing::Return(intermediate));

    EXPECT_CALL(chain, get(dcap::test::constants::PROCESSOR_CA_SUBJECT))
            .WillRepeatedly(testing::Return(nullptr));

    EXPECT_CALL(chain, get(dcap::test::constants::PCK_SUBJECT))
            .WillRepeatedly(testing::Return(nullptr));

    // WHEN
    const auto actual = dcap::PckCertVerifier{}.verify(chain, *trustedRoot);

    // THEN
    EXPECT_EQ(STATUS_SGX_PCK_MISSING, actual);
}
*/

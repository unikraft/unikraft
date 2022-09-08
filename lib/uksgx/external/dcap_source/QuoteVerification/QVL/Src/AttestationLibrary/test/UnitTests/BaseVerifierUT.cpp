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


#include <Verifiers/BaseVerifier.h>
#include <PckParser/PckParser.h>
#include <gtest/gtest.h>
#include <string>

using namespace intel::sgx::dcap;
using namespace ::testing;


struct BaseVerifierUT: public testing::Test {};

struct CommonNameVerificationData {
    std::string commonName;
    std::string match;
    bool expected;
};

struct BaseVerifierCommonNameParametrized : public BaseVerifierUT,
                                            public testing::WithParamInterface<CommonNameVerificationData>
{};

TEST_P(BaseVerifierCommonNameParametrized, testAllMatches)
{
auto params = GetParam();

pckparser::Subject subject;
subject.commonName = params.commonName;

EXPECT_EQ(BaseVerifier{}.commonNameContains(subject, params.match), params.expected);
}

INSTANTIATE_TEST_SUITE_P(AllPatterns,
                        BaseVerifierCommonNameParametrized,
                        testing::Values(
                                CommonNameVerificationData{"OOOOTestTTT", "Test", true},
                                CommonNameVerificationData{"OOOOTTTTest", "Test", true},
                                CommonNameVerificationData{"TestOOOOTTT", "Test", true},
                                CommonNameVerificationData{"OOOOTesTTT", "Test", false},
                                CommonNameVerificationData{"OOOOTTT", "Test", false})
);
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

#include "SgxEcdsaAttestation/AttestationParsers.h"
#include "SignatureTestUtils.h"

#include <gtest/gtest.h>

using namespace intel::sgx::dcap::parser::x509;
using namespace intel::sgx::dcap::parser;
using namespace ::testing;

struct SignatureUT: public testing::Test {};

TEST_F(SignatureUT, signatureConstructors)
{
    ASSERT_NO_THROW(createSignature());
}

TEST_F(SignatureUT, signatureGetters)
{
    const auto& signature = createSignature();
    ASSERT_EQ(signature.getRawDer(), RAW_DER);
    ASSERT_EQ(signature.getR(), R);
    ASSERT_EQ(signature.getS(), S);
}

TEST_F(SignatureUT, signatureOperators)
{
    const auto& signature = createSignature();
    ASSERT_EQ(signature, createSignature());
    ASSERT_FALSE(signature == createSignature({ 0, 1}));
    ASSERT_FALSE(signature == createSignature(RAW_DER, { 0, 1}));
    ASSERT_FALSE(signature == createSignature(RAW_DER, R, { 0, 1}));
}
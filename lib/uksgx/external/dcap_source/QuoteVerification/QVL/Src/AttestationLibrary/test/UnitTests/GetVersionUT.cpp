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

#include <SgxEcdsaAttestation/QuoteVerification.h>
#include <Version/Version.h>
#include <Utils/SafeMemcpy.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstring>
#include <memory>

TEST(sgxAttestationGetVersion, checkIfVersionReturnedByAPIIsEqualToVersionInVersionHeader)
{
    ASSERT_TRUE(sgxAttestationGetVersion() != nullptr);
    EXPECT_EQ(0, std::strcmp(VERSION, sgxAttestationGetVersion()));
}

TEST(sgxAttestationGetVersion, checkIfVersionCanBeCopiedWithCAndCPPManner)
{
    // GIVEN
    const size_t len = strlen(sgxAttestationGetVersion());
    ASSERT_TRUE(len > 0);
    std::unique_ptr<char[]> dest(new char[len + 1]);
    memset(dest.get(), '\0', len + 1);

    // WHEN C-like copy
    safeMemcpy(dest.get(), sgxAttestationGetVersion(), len);

    // WHEN CPP-like copy
    const std::string sgxVersion(sgxAttestationGetVersion());

    // THEN
    EXPECT_EQ(0, std::strcmp(dest.get(), sgxVersion.c_str()));
}


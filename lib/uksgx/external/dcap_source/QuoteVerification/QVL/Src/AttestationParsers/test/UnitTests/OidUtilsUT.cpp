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

#include "OpensslHelpers/OidUtils.h"

#include <gtest/gtest.h>
#include <openssl/asn1.h>

struct OidUtilsUT: public testing::Test {};

using namespace intel::sgx::dcap::crypto;

ASN1_TYPE_uptr createOidValueFromLong(int oidType, long oidValue)
{
    auto asn1Type = make_unique(ASN1_TYPE_new());
    switch(oidType)
    {
        case V_ASN1_INTEGER:
        {
            const auto val = ASN1_INTEGER_new();
            ASN1_INTEGER_set(val, oidValue);
            ASN1_TYPE_set(asn1Type.get(), oidType, val);
            break;
        }
        case V_ASN1_ENUMERATED:
        {
            const auto val = ASN1_ENUMERATED_new();
            ASN1_ENUMERATED_set(val, oidValue);
            ASN1_TYPE_set(asn1Type.get(), oidType, val);
        }
    }

    return asn1Type;
}

TEST_F(OidUtilsUT, oidToUInt)
{
    ASN1_TYPE_uptr oidValue = createOidValueFromLong(V_ASN1_INTEGER, 4);
    ASSERT_EQ(oidToUInt(oidValue.get()), 4);

    oidValue = createOidValueFromLong(V_ASN1_INTEGER, -5);
    ASSERT_EQ(oidToUInt(oidValue.get()), 4294967291);

    oidValue = createOidValueFromLong(V_ASN1_INTEGER, 2147483647);
    ASSERT_EQ(oidToUInt(oidValue.get()), 2147483647);

    oidValue = createOidValueFromLong(V_ASN1_INTEGER, -2147483647);
    ASSERT_EQ(oidToUInt(oidValue.get()), 2147483649);
}

TEST_F(OidUtilsUT, oidToEnum)
{
    ASN1_TYPE_uptr oidValue = createOidValueFromLong(V_ASN1_ENUMERATED, 4);
    ASSERT_EQ(oidToEnum(oidValue.get()), 4);

    oidValue = createOidValueFromLong(V_ASN1_ENUMERATED, -5);
    ASSERT_EQ(oidToEnum(oidValue.get()), -5);

    oidValue = createOidValueFromLong(V_ASN1_ENUMERATED, 2147483647);
    ASSERT_EQ(oidToEnum(oidValue.get()), 2147483647);

    oidValue = createOidValueFromLong(V_ASN1_ENUMERATED, -2147483647);
    ASSERT_EQ(oidToEnum(oidValue.get()), -2147483647);
}
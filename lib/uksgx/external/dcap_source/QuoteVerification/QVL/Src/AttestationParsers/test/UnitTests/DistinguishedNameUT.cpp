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
#include "DistinguishedNameTestUtils.h"

#include <gtest/gtest.h>

using namespace intel::sgx::dcap::parser::x509;
using namespace intel::sgx::dcap::parser;
using namespace ::testing;

struct DistinguishedNameUT: public testing::Test {};

TEST_F(DistinguishedNameUT, distinguishedNameConstructors)
{
    ASSERT_NO_THROW(createDistinguishedName());
}

TEST_F(DistinguishedNameUT, distinguishedNameGetters)
{
    const auto distinguishedName = createDistinguishedName();
    ASSERT_EQ(distinguishedName.getRaw(), RAW_ISSUER);
    ASSERT_EQ(distinguishedName.getCommonName(), COMMON_NAME_ISSUER);
    ASSERT_EQ(distinguishedName.getCountryName(), COUNTRY_NAME_ISSUER);
    ASSERT_EQ(distinguishedName.getOrganizationName(), ORGANIZATION_NAME_ISSUER);
    ASSERT_EQ(distinguishedName.getLocationName(), LOCATION_NAME_ISSUER);
    ASSERT_EQ(distinguishedName.getStateName(), STATE_NAME_ISSUER);
}

TEST_F(DistinguishedNameUT, distinguishedNameOperators)
{
    const auto distinguishedNameIssuer = createDistinguishedName();
    const auto distinguishedNameSubject = createDistinguishedName(RAW_SUBJECT,
                                                                  COMMON_NAME_SUBJECT,
                                                                  COUNTRY_NAME_SUBJECT,
                                                                  ORGANIZATION_NAME_SUBJECT,
                                                                  LOCATION_NAME_SUBJECT,
                                                                  STATE_NAME_SUBJECT);

    ASSERT_NE(distinguishedNameIssuer, distinguishedNameSubject);

    ASSERT_EQ(distinguishedNameIssuer, createDistinguishedName());
    ASSERT_EQ(distinguishedNameIssuer, createDistinguishedName("dummy")); // RAW value may have values in random order
    ASSERT_NE(distinguishedNameIssuer, createDistinguishedName(RAW_ISSUER, "dummy"));
    ASSERT_NE(distinguishedNameIssuer, createDistinguishedName(RAW_ISSUER, COMMON_NAME_ISSUER, "dummy"));
    ASSERT_NE(distinguishedNameIssuer, createDistinguishedName(RAW_ISSUER, COMMON_NAME_ISSUER, COUNTRY_NAME_ISSUER, "dummy"));
    ASSERT_NE(distinguishedNameIssuer, createDistinguishedName(RAW_ISSUER, COMMON_NAME_ISSUER, COUNTRY_NAME_ISSUER,
                                                               ORGANIZATION_NAME_ISSUER, "dummy"));
    ASSERT_NE(distinguishedNameIssuer, createDistinguishedName(RAW_ISSUER, COMMON_NAME_ISSUER, COUNTRY_NAME_ISSUER,
                                                               ORGANIZATION_NAME_ISSUER, LOCATION_NAME_ISSUER, "dummy"));
}
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
#include "ExtensionTestUtils.h"
#include "UnitTests.h"
#include "OpensslHelpers/OpensslTypes.h"
#include "X509CertTypes.h"

#include <gtest/gtest.h>

using namespace intel::sgx::dcap::parser::x509;
using namespace intel::sgx::dcap::parser;
using namespace intel::sgx::dcap;
using namespace ::testing;

struct ExtensionUT: public testing::Test {};

TEST_F(ExtensionUT, extensionConstructors)
{
    ASSERT_NO_THROW(createExtension());
}

TEST_F(ExtensionUT, extensionGetters)
{
    const auto& extension = createExtension();
    ASSERT_EQ(extension.getNid(), NID);
    ASSERT_EQ(extension.getName(), NAME);
    ASSERT_EQ(extension.getValue(), VALUE);
}

TEST_F(ExtensionUT, extensionOperators)
{
    const auto& extension = createExtension();
    ASSERT_EQ(extension, createExtension());
    ASSERT_NE(extension, createExtension(1));
    ASSERT_NE(extension, createExtension(NID, "dummy"));
    ASSERT_NE(extension, createExtension(NID, NAME, { 0x04 }));
}

TEST_F(ExtensionUT, extensionFromSgxExtension)
{
    const auto ext = crypto::make_unique(X509_EXTENSION_new());
    auto tmp = ext.get();
    unsigned char name[20];
    std::copy(NAME.begin(), NAME.end(), name);
    const auto object = crypto::make_unique(ASN1_OBJECT_create(NID, name, (int) NAME.size(), nullptr, nullptr));
    const auto value = crypto::make_unique(ASN1_OCTET_STRING_new());
    ASN1_OCTET_STRING_set(value.get(), VALUE.data(), (int) VALUE.size());
    X509_EXTENSION_create_by_OBJ(&tmp, object.get(), 0, value.get());
    ASSERT_NO_THROW(UnitTests::createExtension(ext.get()));

    const auto& extension = UnitTests::createExtension(ext.get());
    ASSERT_EQ(extension.getNid(), NID);
    ASSERT_EQ(extension.getName(), "2.4.101.115.116");
    ASSERT_EQ(extension.getValue(), VALUE);
}

TEST_F(ExtensionUT, extensionFromX509Extension)
{
    const auto ext = crypto::make_unique(X509_EXTENSION_new());
    const auto value = crypto::make_unique(ASN1_OCTET_STRING_new());
    auto tmp = ext.get();
    X509_EXTENSION_create_by_NID(&tmp, NID_subject_key_identifier, 0, value.get());
    ASSERT_NO_THROW(UnitTests::createExtension(ext.get()));

    const auto& extension = UnitTests::createExtension(ext.get());
    ASSERT_EQ(extension.getNid(), NID_subject_key_identifier);
    ASSERT_EQ(extension.getName(), LN_subject_key_identifier);
    ASSERT_EQ(extension.getValue(), std::vector<uint8_t>{});
}
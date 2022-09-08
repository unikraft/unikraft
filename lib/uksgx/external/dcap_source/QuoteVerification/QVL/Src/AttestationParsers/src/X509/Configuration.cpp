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

#include "X509Constants.h"
#include "ParserUtils.h"
#include "OpensslHelpers/OpensslTypes.h"
#include "OpensslHelpers/OidUtils.h"

#include <openssl/asn1.h>

#include <algorithm>


namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

Configuration::Configuration(
        const bool dynamicPlatform,
        const bool cachedKeys,
        const bool smtEnabled):
                              _dynamicPlatform(dynamicPlatform),
                              _cachedKeys(cachedKeys),
                              _smtEnabled(smtEnabled)
{}

bool Configuration::isDynamicPlatform() const
{
    return _dynamicPlatform;
}

bool Configuration::isCachedKeys() const
{
    return _cachedKeys;
}

bool Configuration::isSmtEnabled() const
{
    return _smtEnabled;
}

bool Configuration::operator ==(const Configuration& other) const
{
    return _dynamicPlatform == other._dynamicPlatform &&
            _cachedKeys == other._cachedKeys &&
            _smtEnabled == other._smtEnabled;
}

// Private

Configuration::Configuration(const ASN1_TYPE *configurationSeq)
{
    crypto::validateOid(oids::CONFIGURATION, configurationSeq, V_ASN1_SEQUENCE);

    const auto stack = crypto::oidToStack(configurationSeq);
    const auto stackEntries = sk_ASN1_TYPE_num(stack.get());

    std::vector<Extension::Type> expectedExtensions = constants::CONFIGURATION_REQUIRED_SGX_EXTENSIONS;

    // Iterate through SGX Configuration Extensions stored as sequence(tuple) of OIDName and OIDValue
    for (int i=0; i < stackEntries; i++)
    {
        const auto oidTupleWrapper = sk_ASN1_TYPE_value(stack.get(), i);
        crypto::validateOid(oids::CONFIGURATION, oidTupleWrapper, V_ASN1_SEQUENCE);

        const auto oidTuple = crypto::oidToStack(oidTupleWrapper);
        const auto oidTupleEntries = sk_ASN1_TYPE_num(oidTuple.get());
        if (oidTupleEntries != 2)
        {
            std::string err = "OID tuple [" + oids::CONFIGURATION + "] expected number of elements is [2] given [" +
                              std::to_string(oidTupleEntries) + "]";
            throw InvalidExtensionException(err);
        }

        const auto oidName = sk_ASN1_TYPE_value(oidTuple.get(), 0);
        const auto oidValue = sk_ASN1_TYPE_value(oidTuple.get(), 1);
        const auto oidNameStr = obj2Str(oidName->value.object);

        if (oidNameStr == oids::DYNAMIC_PLATFORM) // TODO DRY
        {
            crypto::validateOid(oids::DYNAMIC_PLATFORM, oidValue, V_ASN1_BOOLEAN);
            _dynamicPlatform = oidValue->value.boolean;
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::DYNAMIC_PLATFORM), expectedExtensions.end());
        }
        else if (oidNameStr == oids::CACHED_KEYS)
        {
            crypto::validateOid(oids::CACHED_KEYS, oidValue, V_ASN1_BOOLEAN);
            _cachedKeys = oidValue->value.boolean;
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::CACHED_KEYS), expectedExtensions.end());
        }
        else if (oidNameStr == oids::SMT_ENABLED)
        {
            crypto::validateOid(oids::SMT_ENABLED, oidValue, V_ASN1_BOOLEAN);
            _smtEnabled = oidValue->value.boolean;
            expectedExtensions.erase(std::remove(expectedExtensions.begin(), expectedExtensions.end(), Extension::Type::SMT_ENABLED), expectedExtensions.end());
        }
    }

    if (!expectedExtensions.empty())
    {
        std::string err = "Required Configuration SGX extensions not found. Missing [";

        // Convert all but the last element to avoid a trailing ","
        std::for_each(expectedExtensions.begin(), expectedExtensions.end() - 1, [&err](const auto& extension) {
            err += oids::type2Description(extension) + ", ";
        });

        // Now add the last element with no delimiter
        err += oids::type2Description(expectedExtensions.back()) + "]";

        throw InvalidExtensionException(err);
    }
}

}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

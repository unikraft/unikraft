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

#ifndef SGXECDSAATTESTATION_ATTESTATIONLIBRARYADAPTERMOCK_H
#define SGXECDSAATTESTATION_ATTESTATIONLIBRARYADAPTERMOCK_H

#include <AppCore/IAttestationLibraryAdapter.h>
#include <gmock/gmock.h>

namespace intel { namespace sgx { namespace dcap { namespace test {

class AttestationLibraryAdapterMock : public dcap::IAttestationLibraryAdapter
{
public:
    MOCK_CONST_METHOD0(getVersion, std::string());
    MOCK_CONST_METHOD5(verifyQuote, Status(const std::vector<uint8_t>&, const std::string&, const std::string&, const std::string&, const std::string&));
    MOCK_CONST_METHOD5(verifyPCKCertificate, Status(const std::string&, const std::string&, const std::string&, const std::string&, const time_t&));
    MOCK_CONST_METHOD5(verifyTCBInfo, Status(const std::string&, const std::string&, const std::string&, const std::string&,const time_t&));
    MOCK_CONST_METHOD5(verifyQeIdentity, Status(const std::string&, const std::string&, const std::string&, const std::string&, const time_t&));
};
}}}}

#endif //SGXECDSAATTESTATION_ATTESTATIONLIBRARYADAPTERMOCK_H

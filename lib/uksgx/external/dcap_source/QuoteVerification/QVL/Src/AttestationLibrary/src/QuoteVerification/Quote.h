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

#ifndef INTEL_SGX_QVL_QUOTE_H_
#define INTEL_SGX_QVL_QUOTE_H_

#include "QuoteStructures.h"

namespace intel { namespace sgx { namespace dcap {
using namespace intel::sgx::dcap::quote;

class Quote
{
public:
    bool parse(const std::vector<uint8_t>& rawQuote);

    bool validate() const;

    const Header& getHeader() const;
    const EnclaveReport& getEnclaveReport() const;
    const TDReport& getTdReport() const;
    uint32_t getAuthDataSize() const;
    const std::vector<uint8_t>& getSignedData() const;

    // Auth data getters
    const Ecdsa256BitQuoteV3AuthData& getAuthDataV3() const;
    const Ecdsa256BitQuoteV4AuthData& getAuthDataV4() const;
    const std::array<uint8_t, constants::ECDSA_SIGNATURE_BYTE_LEN>& getQeReportSignature() const;
    const EnclaveReport& getQeReport() const;
    const std::array<uint8_t, constants::ECDSA_PUBKEY_BYTE_LEN>& getAttestKeyData() const;
    const std::vector<uint8_t>& getQeAuthData() const;
    const CertificationData& getCertificationData() const;
    const std::array<uint8_t, constants::ECDSA_SIGNATURE_BYTE_LEN>& getQuoteSignature() const;

protected:
    Header header{};
    EnclaveReport enclaveReport{};
    TDReport tdReport{};
    uint32_t authDataSize;
    std::vector<uint8_t> signedData{};

    // Auth data
    Ecdsa256BitQuoteV3AuthData authDataV3{};
    Ecdsa256BitQuoteV4AuthData authDataV4{};
    std::array<uint8_t, constants::ECDSA_SIGNATURE_BYTE_LEN> qeReportSignature{};
    EnclaveReport qeReport{};
    std::array<uint8_t, constants::ECDSA_PUBKEY_BYTE_LEN> attestKeyData{};
    std::vector<uint8_t> qeAuthData{};
    CertificationData certificationData{};
    std::array<uint8_t, constants::ECDSA_SIGNATURE_BYTE_LEN> quoteSignature{};

private:
    std::vector<uint8_t> getDataToSignatureVerification(const std::vector<uint8_t>& rawQuote,
                                                        const std::vector<uint8_t>::difference_type) const;
};

}}} // namespace intel { namespace sgx { namespace dcap { namespace test {


#endif //SGXECDSAATTESTATION_QUOTEGENERATOR_H

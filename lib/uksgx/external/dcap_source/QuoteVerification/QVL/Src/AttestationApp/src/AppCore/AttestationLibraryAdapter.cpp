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

#include "AttestationLibraryAdapter.h"

#include <array>

namespace intel { namespace sgx { namespace dcap {
std::string AttestationLibraryAdapter::getVersion() const
{
#ifdef SGX_TRUSTED
    return enclave.getVersion();
#else
    return ::sgxAttestationGetVersion();
#endif
}

Status AttestationLibraryAdapter::verifyQuote(const std::vector<uint8_t>& quote,
                                              const std::string& pckCertChain,
                                              const std::string& pckCrl,
                                              const std::string& tcbInfo,
                                              const std::string& qeIdentity) const
{
    const auto qeIdentityRawPtr = qeIdentity.empty() ? nullptr : qeIdentity.c_str();
#ifdef SGX_TRUSTED
    return static_cast<Status>(enclave.verifyQuote(quote.data(), (uint32_t) quote.size(), pckCertChain.c_str(), pckCrl.c_str(), tcbInfo.c_str(), qeIdentityRawPtr));
#else
    return ::sgxAttestationVerifyQuote(quote.data(), (uint32_t) quote.size(), pckCertChain.c_str(), pckCrl.c_str(), tcbInfo.c_str(), qeIdentityRawPtr);
#endif
}

Status AttestationLibraryAdapter::verifyPCKCertificate(const std::string& pemCertChain,
                                                       const std::string& pemRootCaCRL,
                                                       const std::string& intermediateCaCRL,
                                                       const std::string& pemTrustedRootCaCertificate,
                                                       const time_t& expirationDate) const
{
    const std::array<const char*, 2> crls{{pemRootCaCRL.data(), intermediateCaCRL.data()}};
#ifdef SGX_TRUSTED
    return static_cast<Status>(enclave.verifyPCKCertificate(pemCertChain.c_str(), crls.data(), pemTrustedRootCaCertificate.c_str(), &expirationDate));
#else
    return ::sgxAttestationVerifyPCKCertificate(pemCertChain.c_str(), crls.data(), pemTrustedRootCaCertificate.c_str(), &expirationDate);
#endif
}

Status AttestationLibraryAdapter::verifyTCBInfo(const std::string& tcbInfo,
                                                const std::string& pemSigningChain,
                                                const std::string& pemRootCaCrl,
                                                const std::string& pemTrustedRootCaCertificate,
                                                const time_t& expirationDate) const
{
#ifdef SGX_TRUSTED
    return static_cast<Status>(enclave.verifyTCBInfo(tcbInfo.c_str(), pemSigningChain.c_str(), pemRootCaCrl.c_str(), pemTrustedRootCaCertificate.c_str(), &expirationDate));
#else
    return ::sgxAttestationVerifyTCBInfo(tcbInfo.c_str(), pemSigningChain.c_str(), pemRootCaCrl.c_str(), pemTrustedRootCaCertificate.c_str(), &expirationDate);
#endif
}

Status AttestationLibraryAdapter::verifyQeIdentity(const std::string &qeIdentity,
                                                   const std::string &pemSigningChain,
                                                   const std::string &pemRootCaCrl,
                                                   const std::string &pemTrustedRootCaCertificate,
                                                   const time_t& expirationDate) const
{
#ifdef SGX_TRUSTED
    return static_cast<Status>(enclave.verifyQEIdentity(qeIdentity.c_str(),
                                                        pemSigningChain.c_str(),
                                                        pemRootCaCrl.c_str(),
                                                        pemTrustedRootCaCertificate.c_str(),
                                                        &expirationDate));
#else
    return ::sgxAttestationVerifyEnclaveIdentity(qeIdentity.c_str(),
                                                 pemSigningChain.c_str(),
                                                 pemRootCaCrl.c_str(),
                                                 pemTrustedRootCaCertificate.c_str(),
                                                 &expirationDate);
#endif
}

}}}

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

#ifndef INTEL_SGX_QVL_TCB_INFO_VERIFIER_H_
#define INTEL_SGX_QVL_TCB_INFO_VERIFIER_H_

#include "CommonVerifier.h"
#include "PckCrlVerifier.h"
#include "TCBSigningChain.h"

#include <CertVerification/CertificateChain.h>
#include <SgxEcdsaAttestation/AttestationParsers.h>
#include <PckParser/CrlStore.h>

namespace intel { namespace sgx { namespace dcap {

class TCBInfoVerifier
{
public:
    TCBInfoVerifier();
    TCBInfoVerifier(std::unique_ptr<CommonVerifier>&& commonVerifier,
                    std::unique_ptr<TCBSigningChain>&& tcbSigningChain);
    TCBInfoVerifier(const TCBInfoVerifier&) = delete;
    TCBInfoVerifier(TCBInfoVerifier&&) = delete;
    ~TCBInfoVerifier() = default;

    TCBInfoVerifier& operator=(const TCBInfoVerifier&) = delete;
    TCBInfoVerifier& operator=(TCBInfoVerifier&&) = default;

    /**
     * Verify corectness of TCB info certificate and Json
     * Checks subject, issuer, validity period, extensions and signature.
     *
     * @param tcbJson - TCB info Json verify
     * @param chain - TCB info chain verify
     * @param rootCaCrl - root CRL
     * @param trustedRoot - trusted root certificate
     * @return Status code of the operation
     */
    Status verify(
            const dcap::parser::json::TcbInfo &tcbJson,
            const CertificateChain &chain,
            const pckparser::CrlStore &rootCaCrl,
            const dcap::parser::x509::Certificate &trustedRoot,
            const std::time_t& expirationDate) const;

private:
    std::unique_ptr<CommonVerifier> _commonVerifier;
    std::unique_ptr<TCBSigningChain> _tcbSigningChain;
};

}}} // namespace intel { namespace sgx { namespace dcap {

#endif // INTEL_SGX_QVL_TCB_INFO_VERIFIER_H_

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

#include "TCBInfoVerifier.h"

#include <CertVerification/X509Constants.h>
#include <OpensslHelpers/SignatureVerification.h>

namespace intel { namespace sgx { namespace dcap {

TCBInfoVerifier::TCBInfoVerifier()
        : _commonVerifier(new CommonVerifier()),
          _tcbSigningChain(new TCBSigningChain())
{
}

TCBInfoVerifier::TCBInfoVerifier(std::unique_ptr<CommonVerifier>&& commonVerifier,
                                 std::unique_ptr<TCBSigningChain>&& tcbSigningChain)
        : _commonVerifier(std::move(commonVerifier)),
          _tcbSigningChain(std::move(tcbSigningChain))
{
}

Status TCBInfoVerifier::verify(
        const dcap::parser::json::TcbInfo &tcbJson,
        const CertificateChain &chain,
        const pckparser::CrlStore &rootCaCrl,
        const dcap::parser::x509::Certificate &trustedRoot,
        const std::time_t& expirationDate) const
{
    const auto status = _tcbSigningChain->verify(chain, rootCaCrl, trustedRoot);
    if (status != STATUS_OK)
    {
        return status;
    }

    const auto tcbSigningCert = chain.getTopmostCert();
    if(!_commonVerifier->checkSha256EcdsaSignature(
            tcbJson.getSignature(), tcbJson.getInfoBody(), tcbSigningCert->getPubKey()))
    {
        return STATUS_TCB_INFO_INVALID_SIGNATURE;
    }

    const auto rootCa = chain.getRootCert();
    if(expirationDate > rootCa->getValidity().getNotAfterTime())
    {
        return STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED;
    }

    if (expirationDate > tcbSigningCert->getValidity().getNotAfterTime())
    {
        return STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED;
    }

    if (rootCaCrl.expired(expirationDate))
    {
        return STATUS_SGX_CRL_EXPIRED;
    }

    if(expirationDate > tcbJson.getNextUpdate())
    {
        return STATUS_SGX_TCB_INFO_EXPIRED;
    }

    return STATUS_OK;
}

}}} //namespace intel { namespace sgx { namespace dcap {

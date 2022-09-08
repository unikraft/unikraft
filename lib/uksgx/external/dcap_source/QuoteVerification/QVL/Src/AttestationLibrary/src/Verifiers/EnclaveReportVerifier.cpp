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

#include "EnclaveReportVerifier.h"
#include "QuoteVerification/ByteOperands.h"
#include "EnclaveIdentityV2.h"
#include "EnclaveIdentityV2.h"
#include <algorithm>
#include <functional>
#include <numeric>
#include <iostream>
#include <memory>


namespace intel { namespace sgx { namespace dcap {

Status EnclaveReportVerifier::verify(const EnclaveIdentityV2 *enclaveIdentity, const EnclaveReport& enclaveReport) const
{
    const auto miscselectMask = vectorToUint32(enclaveIdentity->getMiscselectMask());
    const auto miscselect = vectorToUint32(enclaveIdentity->getMiscselect());

    /// 4.1.2.9.5
    if((enclaveReport.miscSelect & miscselectMask) != miscselect)
    {
        return STATUS_SGX_ENCLAVE_REPORT_MISCSELECT_MISMATCH;
    }

    /// 4.1.2.9.6
    auto attributesReport = enclaveReport.attributes;
    std::vector<uint8_t> attributes(attributesReport.begin(), attributesReport.end());
    if(applyMask(attributes, enclaveIdentity->getAttributesMask()) != enclaveIdentity->getAttributes())
    {
        return STATUS_SGX_ENCLAVE_REPORT_ATTRIBUTES_MISMATCH;
    }

    /// 4.1.2.9.7
    std::vector<uint8_t> mrSigner(enclaveReport.mrSigner.begin(), enclaveReport.mrSigner.end());

    const std::vector<uint8_t>& enclaveIdentityMrSigner = enclaveIdentity->getMrsigner();

    if(!enclaveIdentityMrSigner.empty() && enclaveIdentityMrSigner != mrSigner)
    {
        return STATUS_SGX_ENCLAVE_REPORT_MRSIGNER_MISMATCH;
    }

    /// 4.1.2.9.8
    if(enclaveReport.isvProdID != enclaveIdentity->getIsvProdId())
    {
        return STATUS_SGX_ENCLAVE_REPORT_ISVPRODID_MISMATCH;
    }

    /// 4.1.2.9.9 & 4.1.2.9.10
    auto enclaveIdentityStatus = enclaveIdentity->getTcbStatus(enclaveReport.isvSvn);
    if(enclaveIdentityStatus != TcbStatus::UpToDate)
    {
        if (enclaveIdentityStatus == TcbStatus::Revoked)
        {
            return STATUS_SGX_ENCLAVE_REPORT_ISVSVN_REVOKED;
        }
        return STATUS_SGX_ENCLAVE_REPORT_ISVSVN_OUT_OF_DATE;
    }

    /// 4.1.2.9.11
    return STATUS_OK;
}

uint32_t EnclaveReportVerifier::vectorToUint32(const std::vector<uint8_t>& input) const
{
    auto position = input.cbegin();
    return swapBytes(toUint32(*position, *(std::next(position)), *(std::next(position, 2)), *(std::next(position, 3))));
}

}}} // namespace intel { namespace sgx { namespace dcap {

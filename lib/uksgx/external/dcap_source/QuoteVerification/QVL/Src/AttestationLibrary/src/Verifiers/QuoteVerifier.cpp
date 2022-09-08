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

#include "QuoteVerifier.h"
#include "EnclaveIdentityV2.h"
#include "Utils/RuntimeException.h"

#include <algorithm>
#include <functional>

#include <CertVerification/X509Constants.h>
#include <QuoteVerification/QuoteConstants.h>
#include <OpensslHelpers/DigestUtils.h>
#include <OpensslHelpers/KeyUtils.h>
#include <OpensslHelpers/SignatureVerification.h>
#include <Verifiers/PckCertVerifier.h>

namespace intel { namespace sgx { namespace dcap {

namespace {

constexpr int CPUSVN_LOWER = false;
constexpr int CPUSVN_EQUAL_OR_HIGHER = true;

bool isCpuSvnHigherOrEqual(const dcap::parser::x509::PckCertificate& pckCert,
                           const dcap::parser::json::TcbLevel& tcbLevel)
{
    for(uint32_t index = 0; index < constants::CPUSVN_BYTE_LEN; ++index)
    {
        const auto componentValue = pckCert.getTcb().getSgxTcbComponentSvn(index);
        const auto otherComponentValue = tcbLevel.getSgxTcbComponentSvn(index);
        if(componentValue < otherComponentValue)
        {
            // If *ANY* CPUSVN component is lower then CPUSVN is considered lower
            return CPUSVN_LOWER;
        }
    }
    // but for CPUSVN to be considered higher it requires that *EVERY* CPUSVN component to be higher or equal
    return CPUSVN_EQUAL_OR_HIGHER;
}

bool isTdxTcbHigherOrEqual(const dcap::quote::TDReport& tdReport,
                           const dcap::parser::json::TcbLevel& tcbLevel)
{
    for(uint32_t index = 0; index < constants::CPUSVN_BYTE_LEN; ++index)
    {
        const auto componentValue = tdReport.teeTcbSvn[index];
        const auto otherComponentValue = tcbLevel.getTdxTcbComponent(index);
        if(componentValue < otherComponentValue.getSvn())
        {
            // If *ANY* SVN is lower then TCB level is considered lower
            return false;
        }
    }
    // but for TCB level to be considered higher it requires *EVERY* SVN to be higher or equal
    return true;
}
const parser::json::TcbLevel& getMatchingTcbLevel(const dcap::parser::json::TcbInfo &tcbInfo,
                                       const dcap::parser::x509::PckCertificate &pckCert,
                                       const Quote &quote)
{
    const auto &tcbs = tcbInfo.getTcbLevels();
    const auto certPceSvn = pckCert.getTcb().getPceSvn();

    for (const auto& tcb : tcbs)
    {
        if(isCpuSvnHigherOrEqual(pckCert, tcb) && certPceSvn >= tcb.getPceSvn())
        {
            if (tcbInfo.getVersion() >= 3 &&
                tcbInfo.getId() == parser::json::TcbInfo::TDX_ID &&
                quote.getHeader().teeType == constants::TEE_TYPE_TDX)
            {
                if (isTdxTcbHigherOrEqual(quote.getTdReport(), tcb))
                {
                    return tcb;
                }
            }
            else
            {
                return tcb;
            }
        }
    }

    /// 4.1.2.4.17.3
    throw RuntimeException(STATUS_TCB_NOT_SUPPORTED);
}

Status checkTcbLevel(const dcap::parser::json::TcbInfo& tcbInfoJson, const dcap::parser::x509::PckCertificate& pckCert,
        const Quote& quote)
{
    /// 4.1.2.4.17.1 & 4.1.2.4.17.2
    const auto& tcbLevel = getMatchingTcbLevel(tcbInfoJson, pckCert, quote);

    if (tcbInfoJson.getVersion() >= 3 && tcbInfoJson.getId() == parser::json::TcbInfo::TDX_ID
        && tcbLevel.getTdxTcbComponent(1).getSvn() != quote.getTdReport().teeTcbSvn[1])
    {
        return STATUS_TCB_INFO_MISMATCH;
    }

    const auto& tcbLevelStatus = tcbLevel.getStatus();

    if (tcbLevelStatus == "OutOfDate")
    {
        return STATUS_TCB_OUT_OF_DATE;
    }

    if (tcbLevelStatus == "Revoked")
    {
        return STATUS_TCB_REVOKED;
    }

    if (tcbLevelStatus == "ConfigurationNeeded")
    {
        return STATUS_TCB_CONFIGURATION_NEEDED;
    }

    if (tcbLevelStatus == "ConfigurationAndSWHardeningNeeded")
    {
        return STATUS_TCB_CONFIGURATION_AND_SW_HARDENING_NEEDED;
    }

    if (tcbLevelStatus == "UpToDate")
    {
        return STATUS_OK;
    }

    if (tcbLevelStatus == "SWHardeningNeeded")
    {
        return STATUS_TCB_SW_HARDENING_NEEDED;
    }

    if(tcbInfoJson.getVersion() > 1 && tcbLevelStatus == "OutOfDateConfigurationNeeded")
    {
        return STATUS_TCB_OUT_OF_DATE_CONFIGURATION_NEEDED;
    }

    throw RuntimeException(STATUS_TCB_UNRECOGNIZED_STATUS);
}

Status convergeTcbStatus(Status tcbLevelStatus, Status qeTcbStatus)
{
    if (qeTcbStatus == STATUS_SGX_ENCLAVE_REPORT_ISVSVN_OUT_OF_DATE)
    {
        if (tcbLevelStatus == STATUS_OK ||
            tcbLevelStatus == STATUS_TCB_SW_HARDENING_NEEDED)
        {
            return STATUS_TCB_OUT_OF_DATE;
        }
        if (tcbLevelStatus == STATUS_TCB_CONFIGURATION_NEEDED ||
            tcbLevelStatus == STATUS_TCB_CONFIGURATION_AND_SW_HARDENING_NEEDED)
        {
            return STATUS_TCB_OUT_OF_DATE_CONFIGURATION_NEEDED;
        }
    }
    if (qeTcbStatus == STATUS_SGX_ENCLAVE_REPORT_ISVSVN_REVOKED)
    {
            return STATUS_TCB_REVOKED;
    }

    switch (tcbLevelStatus)
    {
        case STATUS_TCB_OUT_OF_DATE:
        case STATUS_TCB_REVOKED:
        case STATUS_TCB_CONFIGURATION_NEEDED:
        case STATUS_TCB_OUT_OF_DATE_CONFIGURATION_NEEDED:
        case STATUS_TCB_SW_HARDENING_NEEDED:
        case STATUS_TCB_CONFIGURATION_AND_SW_HARDENING_NEEDED:
        case STATUS_OK:
            return tcbLevelStatus;
        default:
            /// 4.1.2.4.16.4
            return STATUS_TCB_UNRECOGNIZED_STATUS;
    }
}

}//anonymous namespace

Status QuoteVerifier::verify(const Quote& quote,
                             const dcap::parser::x509::PckCertificate& pckCert,
                             const pckparser::CrlStore& crl,
                             const dcap::parser::json::TcbInfo& tcbInfoJson,
                             const EnclaveIdentityV2 *enclaveIdentity,
                             const EnclaveReportVerifier& enclaveReportVerifier)
{
    Status qeIdentityStatus = STATUS_QE_IDENTITY_MISMATCH;

    /// 4.1.2.4.4
    if (!_baseVerififer.commonNameContains(pckCert.getSubject(), constants::SGX_PCK_CN_PHRASE)) {
        return STATUS_INVALID_PCK_CERT;
    }

    /// 4.1.2.4.6
    if (!PckCrlVerifier{}.checkIssuer(crl) || crl.getIssuer().raw != pckCert.getIssuer().getRaw()) {
        return STATUS_INVALID_PCK_CRL;
    }

    /// 4.1.2.4.7
    if(crl.isRevoked(pckCert))
    {
        return STATUS_PCK_REVOKED;
    }

    /// 4.1.2.4.9
    if(tcbInfoJson.getVersion() >= 3)
    {
        if(tcbInfoJson.getId() == parser::json::TcbInfo::TDX_ID && quote.getHeader().teeType != dcap::constants::TEE_TYPE_TDX)
        {
            return STATUS_TCB_INFO_MISMATCH;
        }
        if(tcbInfoJson.getId() == parser::json::TcbInfo::SGX_ID && quote.getHeader().teeType != dcap::constants::TEE_TYPE_SGX)
        {
            return STATUS_TCB_INFO_MISMATCH;
        }
    }
    else
    {
        if(quote.getHeader().teeType == dcap::constants::TEE_TYPE_TDX)
        {
            return STATUS_TCB_INFO_MISMATCH;
        }
    }

    /// 4.1.2.4.10
    if(pckCert.getFmspc() != tcbInfoJson.getFmspc())
    {
        return STATUS_TCB_INFO_MISMATCH;
    }

    if(pckCert.getPceId() != tcbInfoJson.getPceId())
    {
        return STATUS_TCB_INFO_MISMATCH;
    }

    const auto certificationDataVerificationStatus = verifyCertificationData(quote.getCertificationData());
    if(certificationDataVerificationStatus != STATUS_OK)
    {
        return certificationDataVerificationStatus;
    }

    auto pubKey = crypto::rawToP256PubKey(pckCert.getPubKey());
    if (pubKey == nullptr)
    {
        return STATUS_INVALID_PCK_CERT; // if there were issues with parsing public key it means cert was invalid.
                                        // Probably it will never happen because parsing cert should fail earlier.
    }

    /// 4.1.2.4.11
    if (tcbInfoJson.getVersion() >= 3 && tcbInfoJson.getId() == parser::json::TcbInfo::TDX_ID)
    {
        const auto& tdxModule = tcbInfoJson.getTdxModule();

        const auto quoteMrSignerSeam = quote.getTdReport().mrSignerSeam;
        const auto& tdxModuleMrSigner = tdxModule.getMrSigner();

        if (quoteMrSignerSeam.size() != tdxModuleMrSigner.size())
        {
            return STATUS_TDX_MODULE_MISMATCH;
        }

        for(uint32_t i = 0; i < tdxModuleMrSigner.size(); i++)
        {
            if (tdxModuleMrSigner[i] != quoteMrSignerSeam[i])
            {
                return STATUS_TDX_MODULE_MISMATCH;
            }
        }

        std::vector<uint8_t> quoteSeamAttributes(quote.getTdReport().seamAttributes.begin(), quote.getTdReport().seamAttributes.end());
        if (applyMask(quoteSeamAttributes, tdxModule.getAttributesMask()) != tdxModule.getAttributes())
        {
            return STATUS_TDX_MODULE_MISMATCH;
        }
    }

    /// 4.1.2.4.12
    if (!crypto::verifySha256EcdsaSignature(quote.getQeReportSignature(), quote.getQeReport().rawBlob(), *pubKey))
    {
        return STATUS_INVALID_QE_REPORT_SIGNATURE;
    }

    /// 4.1.2.4.13
    const auto hashedConcatOfAttestKeyAndQeReportData = [&]() -> std::vector<uint8_t>
    {
        std::vector<uint8_t> ret;
        ret.reserve(quote.getAttestKeyData().size() + quote.getQeAuthData().size());
        std::copy(quote.getAttestKeyData().begin(), quote.getAttestKeyData().end(), std::back_inserter(ret));
        std::copy(quote.getQeAuthData().begin(), quote.getQeAuthData().end(), std::back_inserter(ret));

        return crypto::sha256Digest(ret);
    }();

    if(hashedConcatOfAttestKeyAndQeReportData.empty() || !std::equal(hashedConcatOfAttestKeyAndQeReportData.begin(),
                                                                     hashedConcatOfAttestKeyAndQeReportData.end(),
                                                                     quote.getQeReport().reportData.begin()))
    {
        return STATUS_INVALID_QE_REPORT_DATA;
    }

    if (enclaveIdentity)
    {
        /// 4.1.2.4.14
        if(quote.getHeader().teeType == dcap::constants::TEE_TYPE_TDX)
        {
            if(enclaveIdentity->getVersion() == 1)
            {
                return STATUS_QE_IDENTITY_MISMATCH;
            }
            else if(enclaveIdentity->getVersion() == 2)
            {
                if(enclaveIdentity->getID() != EnclaveID::TD_QE)
                {
                    return STATUS_QE_IDENTITY_MISMATCH;
                }
            }
        }
        else if(quote.getHeader().teeType == dcap::constants::TEE_TYPE_SGX)
        {
            if(enclaveIdentity->getID() != EnclaveID::QE)
            {
                return STATUS_QE_IDENTITY_MISMATCH;
            }
        }
        else
        {
            return STATUS_QE_IDENTITY_MISMATCH;
        }

        /// 4.1.2.4.15
        qeIdentityStatus = enclaveReportVerifier.verify(enclaveIdentity, quote.getQeReport());
        switch(qeIdentityStatus) {
            case STATUS_SGX_ENCLAVE_REPORT_UNSUPPORTED_FORMAT:
                return STATUS_UNSUPPORTED_QUOTE_FORMAT;
            case STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT:
            case STATUS_SGX_ENCLAVE_IDENTITY_INVALID:
            case STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_VERSION:
                return STATUS_UNSUPPORTED_QE_IDENTITY_FORMAT;
            case STATUS_SGX_ENCLAVE_REPORT_MISCSELECT_MISMATCH:
            case STATUS_SGX_ENCLAVE_REPORT_ATTRIBUTES_MISMATCH:
            case STATUS_SGX_ENCLAVE_REPORT_MRSIGNER_MISMATCH:
            case STATUS_SGX_ENCLAVE_REPORT_ISVPRODID_MISMATCH:
                return STATUS_QE_IDENTITY_MISMATCH;
            case STATUS_SGX_ENCLAVE_REPORT_ISVSVN_OUT_OF_DATE:
            case STATUS_SGX_ENCLAVE_REPORT_ISVSVN_REVOKED:
            default:
                break;
        }
    }

    const auto attestKey = crypto::rawToP256PubKey(quote.getAttestKeyData());
    if(!attestKey)
    {
        return STATUS_UNSUPPORTED_QUOTE_FORMAT;
    }

    /// 4.1.2.4.16
    if (!crypto::verifySha256EcdsaSignature(quote.getQuoteSignature(),
                                            quote.getSignedData(),
                                            *attestKey))
    {
        return STATUS_INVALID_QUOTE_SIGNATURE;
    }

    try
    {
        /// 4.1.2.4.17
        const auto tcbLevelStatus = checkTcbLevel(tcbInfoJson, pckCert, quote);

        if (tcbLevelStatus == STATUS_TCB_INFO_MISMATCH)
        {
            return STATUS_TCB_INFO_MISMATCH;
        }

        if (enclaveIdentity)
        {
            return convergeTcbStatus(tcbLevelStatus, qeIdentityStatus);
        }

        return tcbLevelStatus;
    }
    catch (const RuntimeException &ex)
    {
        return ex.getStatus();
    }
}

Status QuoteVerifier::verifyCertificationData(const CertificationData& certificationData) const
{
    if (certificationData.parsedDataSize != certificationData.data.size())
    {
        return STATUS_UNSUPPORTED_QUOTE_FORMAT;
    }

    return STATUS_OK;
}

}}} // namespace intel { namespace sgx { namespace dcap {

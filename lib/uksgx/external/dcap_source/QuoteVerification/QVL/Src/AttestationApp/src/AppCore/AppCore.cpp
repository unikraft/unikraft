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

#include "AppCore.h"
#include "AppOptions.h"
#include "IAttestationLibraryAdapter.h"
#include "StatusPrinter.h"

namespace intel { namespace sgx { namespace dcap {

namespace {
inline std::string bytesToHexString(const std::vector<uint8_t> &vector)
{
    std::string result;
    result.reserve(vector.size() * 2);   // two digits per character

    static constexpr char hex[] = "0123456789ABCDEF";

    for (const uint8_t c : vector)
    {
        result.push_back(hex[c / 16]);
        result.push_back(hex[c % 16]);
    }

    return result;
}

void outputResult(const std::string& step, Status status, std::ostream& logger)
{
    if (status != STATUS_OK)
    {
        logger << step << " verification failed with status: " << status << std::endl;
    }
    else
    {
        logger << step << " verification OK!" << std::endl;
    }
}
}

AppCore::AppCore(std::shared_ptr<IAttestationLibraryAdapter> libAdapter, std::shared_ptr<IFileReader> reader)
    : attestationLib(libAdapter), fileReader(reader)
{
}

std::string AppCore::version() const
{
    return attestationLib->getVersion();
}

bool AppCore::runVerification(const AppOptions& options, std::ostream& logger) const
{
    try
    {
        static constexpr char PEM_HEADER_STRING_X509_CRL[] = "-----BEGIN X509 CRL-----";
        const auto expirationDate = options.expirationDate;
        const auto pckCert = fileReader->readContent(options.pckCertificateFile);
        const auto pckSigningChain = fileReader->readContent(options.pckSigningChainFile);
        const auto pckCertChain = pckSigningChain + pckCert;
        auto rootCaCrl = fileReader->readContent(options.rootCaCrlFile);
        if (rootCaCrl.rfind(PEM_HEADER_STRING_X509_CRL, 0) == std::string::npos)
        {
            rootCaCrl = bytesToHexString(fileReader->readBinaryContent(options.rootCaCrlFile));
        }
        auto intermediateCaCrl = fileReader->readContent(options.intermediateCaCrlFile);
        if (intermediateCaCrl.rfind(PEM_HEADER_STRING_X509_CRL, 0) == std::string::npos)
        {
            intermediateCaCrl = bytesToHexString(fileReader->readBinaryContent(options.intermediateCaCrlFile));
        }
        const auto trustedRootCACert = fileReader->readContent(options.trustedRootCACertificateFile);
        const auto pckVerifyStatus = attestationLib->verifyPCKCertificate(pckCertChain, rootCaCrl, intermediateCaCrl, trustedRootCACert, expirationDate);
        outputResult("PCK certificate chain", pckVerifyStatus, logger);

        const auto tcbInfo = fileReader->readContent(options.tcbInfoFile);
        const auto tcbSigningCert = fileReader->readContent(options.tcbSigningChainFile);
        const auto tcbVerifyStatus = attestationLib->verifyTCBInfo(tcbInfo, tcbSigningCert, rootCaCrl, trustedRootCACert, expirationDate);
        outputResult("TCB info", tcbVerifyStatus, logger);

        const auto qeIdentityPresent = !options.qeIdentityFile.empty();
        std::string qeIdentity = std::string{};
        Status qeIdentityVerifyStatus = STATUS_OK;
        if (qeIdentityPresent)
        {
            qeIdentity = fileReader->readContent(options.qeIdentityFile);
            qeIdentityVerifyStatus = attestationLib->verifyQeIdentity(qeIdentity, tcbSigningCert, rootCaCrl, trustedRootCACert, expirationDate);
            outputResult("QeIdentity", qeIdentityVerifyStatus, logger);
        }

        const auto qveIdentityPresent = !options.qveIdentityFile.empty();
        std::string qveIdentity = std::string{};
        Status qveIdentityVerifyStatus = STATUS_OK;
        if (qveIdentityPresent)
        {
            qveIdentity = fileReader->readContent(options.qveIdentityFile);
            qveIdentityVerifyStatus = attestationLib->verifyQeIdentity(qveIdentity, tcbSigningCert, rootCaCrl, trustedRootCACert, expirationDate);
            outputResult("QveIdentity", qveIdentityVerifyStatus, logger);
        }

        const auto quote = fileReader->readBinaryContent(options.quoteFile);
        const auto quoteVerifyStatus = attestationLib->verifyQuote(quote, pckCert, intermediateCaCrl, tcbInfo, qeIdentity);
        outputResult("Quote", quoteVerifyStatus, logger);

        return (pckVerifyStatus == STATUS_OK) && (tcbVerifyStatus == STATUS_OK) && (quoteVerifyStatus == STATUS_OK) &&
                    (qeIdentityVerifyStatus == STATUS_OK) && (qveIdentityVerifyStatus == STATUS_OK);
    }
    catch (const IFileReader::ReadFileException& e)
    {
        logger << "ERROR while trying to read input files: " << e.what();
        return false;
    }
}

}}}

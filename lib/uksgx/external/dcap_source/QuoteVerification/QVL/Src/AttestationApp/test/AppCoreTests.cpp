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


#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "AppCore/AppCore.h"
#include "AppCore/AppOptions.h"
#include "Mocks/AttestationLibraryAdapterMock.h"
#include "Mocks/FileReaderMock.h"

using namespace ::testing;
using namespace intel::sgx::dcap;

struct AppCoreTests: public Test
{
    std::shared_ptr<StrictMock<test::AttestationLibraryAdapterMock>> attestationLibraryMock =
        std::make_shared<StrictMock<test::AttestationLibraryAdapterMock>>();
    std::shared_ptr<StrictMock<test::FileReaderMock>> fileReaderMock =
        std::make_shared<StrictMock<test::FileReaderMock>>();
    std::stringstream log;
    AppCore app{attestationLibraryMock, fileReaderMock};

    AppOptions options{
        "PCK/certificate/file/path",
        "PCKSigning/chain/file/path",
        "rootCA/crl/file/path",
        "intermediateCA/crl/file/path",
        "TrustedRoot/file/path",
        "TCBInfo/file/path",
        "Tcb/signing/chain/file/path",
        "Quote/file/path",
        "QeIdentity/file/path/",
        "QveIdentity/file/path/",
        0};

    std::vector<uint8_t> quoteContent = {1, 2, 255, 0, 0, 43, 58};
    std::string pckCertContent = "pckCert content";
    std::string pckSigningChainContent = "pckSigningChain content";
    std::string rootCaCrlContent = "-----BEGIN X509 CRL-----rootCaCrl content";
    std::string intermediateCaCrlContent = "-----BEGIN X509 CRL-----intermediateCaCrl content";
    std::string trustedRootCertContent = "trustedRootCA content";
    std::string tcbInfoContent = "tcbInfo content";
    std::string qeIdentityContent = "qeIdentity content";
    std::string qveIdentityContent = "qveIdentity content";
    std::string tcbSigningChainContent = "tcb signing chain content";
};

TEST_F(AppCoreTests, shouldProvideVersionStringFromLibrary)
{
    std::string version = "Version string";
    EXPECT_CALL(*attestationLibraryMock, getVersion()).WillRepeatedly(Return(version));
    ASSERT_EQ(version, app.version());
}

TEST_F(AppCoreTests, shouldVerifyInputDataFromProvidedFiles)
{
    EXPECT_CALL(*fileReaderMock, readContent(options.pckCertificateFile)).WillOnce(Return(pckCertContent));
    EXPECT_CALL(*fileReaderMock, readContent(options.pckSigningChainFile)).WillOnce(Return(pckSigningChainContent));
    EXPECT_CALL(*fileReaderMock, readContent(options.rootCaCrlFile)).WillOnce(Return(rootCaCrlContent));
    EXPECT_CALL(*fileReaderMock, readContent(options.intermediateCaCrlFile)).WillOnce(Return(intermediateCaCrlContent));
    EXPECT_CALL(*fileReaderMock, readContent(options.trustedRootCACertificateFile)).WillOnce(Return(trustedRootCertContent));
    EXPECT_CALL(*fileReaderMock, readContent(options.tcbInfoFile)).WillOnce(Return(tcbInfoContent));
    EXPECT_CALL(*fileReaderMock, readContent(options.qeIdentityFile)).WillOnce(Return(qeIdentityContent));
    EXPECT_CALL(*fileReaderMock, readContent(options.qveIdentityFile)).WillOnce(Return(qveIdentityContent));
    EXPECT_CALL(*fileReaderMock, readContent(options.tcbSigningChainFile)).WillOnce(Return(tcbSigningChainContent));
    EXPECT_CALL(*fileReaderMock, readBinaryContent(options.quoteFile)).WillOnce(Return(quoteContent));

    EXPECT_CALL(*attestationLibraryMock, verifyPCKCertificate(AllOf(HasSubstr(pckCertContent), HasSubstr(pckSigningChainContent)),
        rootCaCrlContent, intermediateCaCrlContent, trustedRootCertContent, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyTCBInfo(tcbInfoContent, tcbSigningChainContent, rootCaCrlContent, trustedRootCertContent, _))
        .WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(qeIdentityContent, tcbSigningChainContent, rootCaCrlContent, trustedRootCertContent, _))
            .WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(qveIdentityContent, tcbSigningChainContent, rootCaCrlContent, trustedRootCertContent, _))
            .WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQuote(quoteContent, pckCertContent, intermediateCaCrlContent, tcbInfoContent, qeIdentityContent))
        .WillOnce(Return(STATUS_OK));

    EXPECT_TRUE(app.runVerification(options, log));
}

TEST_F(AppCoreTests, shouldFailWhenFileOperationFailed)
{
    std::string exceptionMessage = "Exception message";
    EXPECT_CALL(*fileReaderMock, readContent(_)).WillOnce(Throw(IFileReader::ReadFileException(exceptionMessage)));

    EXPECT_FALSE(app.runVerification(options, log));
    EXPECT_THAT(log.str(), HasSubstr(exceptionMessage));
}

TEST_F(AppCoreTests, shouldFailWhenBinaryFileOperationFailed)
{
    std::string exceptionMessage = "Exception message";
    EXPECT_CALL(*fileReaderMock, readContent(_)).WillRepeatedly(Return("-----BEGIN X509 CRL-----content"));
    EXPECT_CALL(*fileReaderMock, readBinaryContent(_)).WillOnce(Throw(IFileReader::ReadFileException(exceptionMessage)));
    EXPECT_CALL(*attestationLibraryMock, verifyTCBInfo(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(_, _, _, _, _)).Times(2).WillRepeatedly(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyPCKCertificate(_, _, _, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_FALSE(app.runVerification(options, log));
    EXPECT_THAT(log.str(), HasSubstr(exceptionMessage));
}

TEST_F(AppCoreTests, shouldFailWhenQuoteValidationFailed)
{
    EXPECT_CALL(*fileReaderMock, readContent(_)).WillRepeatedly(Return("content"));
    EXPECT_CALL(*fileReaderMock, readBinaryContent(_)).WillRepeatedly(Return(quoteContent));
    EXPECT_CALL(*attestationLibraryMock, verifyTCBInfo(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(_, _, _, _, _)).Times(2).WillRepeatedly(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyPCKCertificate(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQuote(_, _, _, _, _)).WillOnce(Return(STATUS_MISSING_PARAMETERS));
    EXPECT_FALSE(app.runVerification(options, log));
    EXPECT_THAT(log.str(), HasSubstr("STATUS_MISSING_PARAMETERS"));
}

TEST_F(AppCoreTests, shouldFailWhenPCKCertificateValidationFailed)
{
    EXPECT_CALL(*fileReaderMock, readContent(_)).WillRepeatedly(Return("content"));
    EXPECT_CALL(*fileReaderMock, readBinaryContent(_)).WillRepeatedly(Return(quoteContent));
    EXPECT_CALL(*attestationLibraryMock, verifyQuote(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyTCBInfo(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(_, _, _, _, _)).Times(2).WillRepeatedly(Return(STATUS_OK));

    EXPECT_CALL(*attestationLibraryMock, verifyPCKCertificate(_, _, _, _, _)).WillOnce(Return(STATUS_SGX_PCK_INVALID));

    EXPECT_FALSE(app.runVerification(options, log));
    EXPECT_THAT(log.str(), HasSubstr("STATUS_SGX_PCK_INVALID"));
}

TEST_F(AppCoreTests, shouldFailWhenTCBInfoValidationFailed)
{
    EXPECT_CALL(*fileReaderMock, readContent(_)).WillRepeatedly(Return("content"));
    EXPECT_CALL(*fileReaderMock, readBinaryContent(_)).WillRepeatedly(Return(quoteContent));
    EXPECT_CALL(*attestationLibraryMock, verifyQuote(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyPCKCertificate(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(_, _, _, _, _)).Times(2).WillRepeatedly(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyTCBInfo(_, _, _, _, _)).WillOnce(Return(STATUS_TCB_INFO_INVALID_SIGNATURE));

    EXPECT_FALSE(app.runVerification(options, log));
    EXPECT_THAT(log.str(), HasSubstr("STATUS_TCB_INFO_INVALID_SIGNATURE"));
}

TEST_F(AppCoreTests, shouldFailWhenQeIdentityValidationFailed)
{
    EXPECT_CALL(*fileReaderMock, readContent(_)).WillRepeatedly(Return("content"));
    EXPECT_CALL(*fileReaderMock, readBinaryContent(_)).WillRepeatedly(Return(quoteContent));
    EXPECT_CALL(*attestationLibraryMock, verifyQuote(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyPCKCertificate(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(_, _, _, _, _))
       .WillOnce(Return(STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT))
       .WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyTCBInfo(_, _, _, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_FALSE(app.runVerification(options, log));
    EXPECT_THAT(log.str(), HasSubstr("STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT"));
}

TEST_F(AppCoreTests, shouldFailWhenQveIdentityValidationFailed)
{
    EXPECT_CALL(*fileReaderMock, readContent(_)).WillRepeatedly(Return("content"));
    EXPECT_CALL(*fileReaderMock, readBinaryContent(_)).WillRepeatedly(Return(quoteContent));
    EXPECT_CALL(*attestationLibraryMock, verifyQuote(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyPCKCertificate(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(_, _, _, _, _))
            .WillOnce(Return(STATUS_OK))
            .WillOnce(Return(STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT));
    EXPECT_CALL(*attestationLibraryMock, verifyTCBInfo(_, _, _, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_FALSE(app.runVerification(options, log));
    EXPECT_THAT(log.str(), HasSubstr("STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT"));
}

TEST_F(AppCoreTests, shouldVerifyWhenQeIdentityAndQveIdentityFileWasNotGiven)
{
    AppOptions noQeIdentityOptions{
            "PCK/certificate/file/path",
            "PCKSigning/chain/file/path",
            "rootCA/crl/file/path",
            "intermediateCA/crl/file/path",
            "TrustedRoot/file/path",
            "TCBInfo/file/path",
            "Tcb/signing/chain/file/path"
            "Quote/file/path",
            "",
            "",
            "",
            0
    };

    EXPECT_CALL(*fileReaderMock, readContent(_)).WillRepeatedly(Return("content"));
    EXPECT_CALL(*fileReaderMock, readContent(options.qeIdentityFile)).Times(0);
    EXPECT_CALL(*fileReaderMock, readContent(options.qveIdentityFile)).Times(0);
    EXPECT_CALL(*fileReaderMock, readBinaryContent(_)).WillRepeatedly(Return(quoteContent));
    EXPECT_CALL(*attestationLibraryMock, verifyQuote(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyPCKCertificate(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(_, _, _, _, _)).Times(0);
    EXPECT_CALL(*attestationLibraryMock, verifyTCBInfo(_, _, _, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_TRUE(app.runVerification(noQeIdentityOptions, log));
}

TEST_F(AppCoreTests, shouldVerifyWhenQeIdentityFileWasNotGiven)
{
    AppOptions noQeIdentityOptions{
            "PCK/certificate/file/path",
            "PCKSigning/chain/file/path",
            "rootCA/crl/file/path",
            "intermediateCA/crl/file/path",
            "TrustedRoot/file/path",
            "TCBInfo/file/path",
            "Tcb/signing/chain/file/path"
            "Quote/file/path",
            "",
            "",
            "QveIdentity/file/path/",
            0
    };

    EXPECT_CALL(*fileReaderMock, readContent(_)).WillRepeatedly(Return("content"));
    EXPECT_CALL(*fileReaderMock, readContent(options.qeIdentityFile)).Times(0);
    EXPECT_CALL(*fileReaderMock, readBinaryContent(_)).WillRepeatedly(Return(quoteContent));
    EXPECT_CALL(*attestationLibraryMock, verifyQuote(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyPCKCertificate(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyTCBInfo(_, _, _, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_TRUE(app.runVerification(noQeIdentityOptions, log));
}

TEST_F(AppCoreTests, shouldVerifyWhenQveIdentityFileWasNotGiven)
{
    AppOptions noQveIdentityOptions{
            "PCK/certificate/file/path",
            "PCKSigning/chain/file/path",
            "rootCA/crl/file/path",
            "intermediateCA/crl/file/path",
            "TrustedRoot/file/path",
            "TCBInfo/file/path",
            "Tcb/signing/chain/file/path"
            "Quote/file/path",
            "",
            "QeIdentity/file/path/",
            "",
            0
    };

    EXPECT_CALL(*fileReaderMock, readContent(_)).WillRepeatedly(Return("content"));
    EXPECT_CALL(*fileReaderMock, readContent(options.qveIdentityFile)).Times(0);
    EXPECT_CALL(*fileReaderMock, readBinaryContent(_)).WillRepeatedly(Return(quoteContent));
    EXPECT_CALL(*attestationLibraryMock, verifyQuote(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyPCKCertificate(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyQeIdentity(_, _, _, _, _)).WillOnce(Return(STATUS_OK));
    EXPECT_CALL(*attestationLibraryMock, verifyTCBInfo(_, _, _, _, _)).WillOnce(Return(STATUS_OK));

    EXPECT_TRUE(app.runVerification(noQveIdentityOptions, log));
}

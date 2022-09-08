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
#include "AppCore/AppOptionsParser.h"
#include <chrono>
#include <ctime>

using namespace ::testing;
using namespace intel::sgx::dcap;

struct AppOptionsParserTests: public Test
{
    AppOptionsParser parser;

    const std::string quoteDefaultPath = "quote.dat";
    const std::string trustedRootDefaultPath = "trustedRootCaCert.pem";
    const std::string pckCertDefaultPath = "pckCert.pem";
    const std::string pckSigningChainDefaultPath = "pckSignChain.pem";
    const std::string tcbSignChainDefaultPath = "tcbSignChain.pem";
    const std::string tcbInfoDefaultPath = "tcbInfo.json";
    const std::string rootCaCrlDefaultPath = "rootCaCrl.der";
    const std::string intermediateCaCrlDefaultPath = "intermediateCaCrl.der";
    const std::string qeIdentityDefaultPath = "qeIdentity.json";

    const std::string helpOutput = "Usage: [-h] [--trustedRootCaCert=<string>] [--pckSignChain=<string>] [--pckCert=<string>] [--tcbSignChain=<string>] [--tcbInfo=<string>] [--qeIdentity=<string>] [--qveIdentity=<string>] [--rootCaCrl=<string>] [--intermediateCaCrl=<string>] [--quote=<string>] [--expirationDate=<string>]\n\n"
            "--trustedRootCaCert=<string>             Trusted root CA Certificate file path, PEM format [=trustedRootCaCert.pem]\n"
            "--pckSignChain=<string>                  PCK Signing Certificate chain file path, PEM format [=pckSignChain.pem]\n"
            "--pckCert=<string>                       PCK Certificate file path, PEM format [=pckCert.pem]\n"
            "--tcbSignChain=<string>                  TCB Signing Certificate chain file path, PEM format [=tcbSignChain.pem]\n"
            "--tcbInfo=<string>                       TCB Info file path, JSON format [=tcbInfo.json]\n"
            "--qeIdentity=<string>                    QeIdentity file path, JSON format. QeIdentity verification is optional, will not run by default [=]\n"
            "--qveIdentity=<string>                   QveIdentity file path, JSON format. QveIdentity verification is optional, will not run by default [=]\n"
            "--rootCaCrl=<string>                     Root Ca CRL file path, PEM or DER format [=rootCaCrl.der]\n"
            "--intermediateCaCrl=<string>             Intermediate Ca CRL file path, PEM or DER format [=intermediateCaCrl.der]\n"
            "--quote=<string>                         Quote file path, binary format [=quote.dat]\n"
            "--expirationDate=<string>                Expiration date in timestamp seconds [=seconds]\n"
            "-h, --help                               Print this message\n";

    // return true if difference between input time and current time is less than 3 seconds
    bool checkTimeWithHysteresis(time_t input)
    {
        time_t hysteresis = 3;
        time_t expTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        return input <= expTime + hysteresis && input >= expTime - hysteresis;
    }
};

TEST_F(AppOptionsParserTests, ReturnsDefaultValuesWhenNoArgumentsGivenPrintsNothing)
{
    std::vector<const char*> vec {"./AppCommand"};
    std::ostringstream logger;

    auto options = parser.parse((int32_t) vec.size(), const_cast<char**>(vec.data()), logger);

    EXPECT_TRUE(options != nullptr);
    EXPECT_TRUE(logger.str().empty());
    EXPECT_EQ(options->pckCertificateFile, pckCertDefaultPath);
    EXPECT_EQ(options->pckSigningChainFile, pckSigningChainDefaultPath);
    EXPECT_EQ(options->rootCaCrlFile, rootCaCrlDefaultPath);
    EXPECT_EQ(options->intermediateCaCrlFile, intermediateCaCrlDefaultPath);
    EXPECT_EQ(options->trustedRootCACertificateFile, trustedRootDefaultPath);
    EXPECT_EQ(options->tcbInfoFile, tcbInfoDefaultPath);
    EXPECT_EQ(options->tcbSigningChainFile, tcbSignChainDefaultPath);
    EXPECT_EQ(options->quoteFile, quoteDefaultPath);
    EXPECT_TRUE(checkTimeWithHysteresis(options->expirationDate));
}

TEST_F(AppOptionsParserTests, ReturnsGivenValuesWhenSomeParametersPassedOtherAsDefaultsPrintsNothing)
{
    const std::string pckCert = "pckCert";
    const std::string pckCertArg = "--pckCert=" + pckCert;
    const std::string rootCaCrl = "rootCaCrl";
    const std::string rootCaCrlArg = "--rootCaCrl=" + rootCaCrl;
    const std::string tcbSignChain = "tcbSignChain";
    const std::string tcbSignChainArg = "--tcbSignChain=" + tcbSignChain;

    std::vector<const char*> vec {"./AppCommand",
                                  pckCertArg.c_str(),
                                  rootCaCrlArg.c_str(),
                                  tcbSignChainArg.c_str()
    };
    std::ostringstream logger;

    auto options = parser.parse((int32_t) vec.size(), const_cast<char**>(vec.data()), logger);

    EXPECT_TRUE(options != nullptr);
    EXPECT_TRUE(logger.str().empty());
    EXPECT_EQ(options->pckCertificateFile, pckCert);
    EXPECT_EQ(options->pckSigningChainFile, pckSigningChainDefaultPath);
    EXPECT_EQ(options->rootCaCrlFile, rootCaCrl);
    EXPECT_EQ(options->intermediateCaCrlFile, intermediateCaCrlDefaultPath);
    EXPECT_EQ(options->trustedRootCACertificateFile, trustedRootDefaultPath);
    EXPECT_EQ(options->tcbInfoFile, tcbInfoDefaultPath);
    EXPECT_EQ(options->tcbSigningChainFile, tcbSignChain);
    EXPECT_EQ(options->quoteFile, quoteDefaultPath);
    EXPECT_TRUE(checkTimeWithHysteresis(options->expirationDate));

}

TEST_F(AppOptionsParserTests, ReturnsGivenValuesWhenParametersPassedPrintsNothing)
{
    const std::string pckCert = "pckCert";
    const std::string pckCertArg = "--pckCert=" + pckCert;
    const std::string pckSigningChain = "pckSignChain";
    const std::string pckSigningChainArg = "--pckSignChain=" + pckSigningChain;
    const std::string rootCaCrl = "rootCaCrl";
    const std::string rootCaCrlArg = "--rootCaCrl=" + rootCaCrl;
    const std::string intermediateCaCrl = "intermediateCaCrl";
    const std::string intermediateCaCrlArg = "--intermediateCaCrl=" + intermediateCaCrl;
    const std::string trustedRootCaCert = "trustedRootCaCert";
    const std::string trustedRootCaCertArg = "--trustedRootCaCert=" + trustedRootCaCert;
    const std::string tcbInfo = "tcbInfo";
    const std::string tcbInfoArg = "--tcbInfo=" + tcbInfo;
    const std::string tcbSignChain = "tcbSignChain";
    const std::string tcbSignChainArg = "--tcbSignChain=" + tcbSignChain;
    const std::string quote = "quote123";
    const std::string quoteArg = "--quote=" + quote;
    const std::string expirationDate = "9557826584";
    const std::string expirationDateArg = "--expirationDate=" + expirationDate;

    std::vector<const char*> vec {"./AppCommand",
                                  pckCertArg.c_str(),
                                  pckSigningChainArg.c_str(),
                                  rootCaCrlArg.c_str(),
                                  intermediateCaCrlArg.c_str(),
                                  trustedRootCaCertArg.c_str(),
                                  tcbSignChainArg.c_str(),
                                  tcbInfoArg.c_str(),
                                  quoteArg.c_str(),
                                  expirationDateArg.c_str()
                                 };
    std::ostringstream logger;

    auto options = parser.parse((int32_t) vec.size(), const_cast<char**>(vec.data()), logger);

    EXPECT_TRUE(options != nullptr);
    EXPECT_TRUE(logger.str().empty());
    EXPECT_EQ(options->pckCertificateFile, pckCert);
    EXPECT_EQ(options->pckSigningChainFile, pckSigningChain);
    EXPECT_EQ(options->rootCaCrlFile, rootCaCrl);
    EXPECT_EQ(options->intermediateCaCrlFile, intermediateCaCrl);
    EXPECT_EQ(options->trustedRootCACertificateFile, trustedRootCaCert);
    EXPECT_EQ(options->tcbInfoFile, tcbInfo);
    EXPECT_EQ(options->tcbSigningChainFile, tcbSignChain);
    EXPECT_EQ(options->quoteFile, quote);
    EXPECT_EQ(options->expirationDate, std::stol(expirationDate));
}

TEST_F(AppOptionsParserTests, ReturnsNothingWhenHelpTypedPrintsHelp)
{
    std::vector<const char*> vec {"./AppCommand", "--help"};
    std::ostringstream logger;

    testing::internal::CaptureStdout();

    auto options = parser.parse((int32_t) vec.size(), const_cast<char**>(vec.data()), logger);

    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(options == nullptr);
    EXPECT_TRUE(logger.str().empty());
    EXPECT_EQ(output, helpOutput);
}

TEST_F(AppOptionsParserTests, ReturnsNothingWhenUnsupportedParamPrintsErrorAndHelp)
{
    std::vector<const char*> vec {"./AppCommand", "--unsupportedParam"};
    std::ostringstream logger;

    testing::internal::CaptureStdout();

    auto options = parser.parse((int32_t) vec.size(), const_cast<char**>(vec.data()), logger);

    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(options == nullptr);
    EXPECT_TRUE(logger.str().empty());
    EXPECT_TRUE(output.find("Sample app: invalid option \"--unsupportedParam\"") != std::string::npos);
    EXPECT_TRUE(output.find(helpOutput) != std::string::npos);
}

TEST_F(AppOptionsParserTests, ReturnsCantParseExpirationDateWhenProvidedInvalidTimestamp)
{
    std::vector<const char*> vec {"./AppCommand", "--expirationDate=test"};
    std::ostringstream logger;

    testing::internal::CaptureStdout();

    auto options = parser.parse((int32_t) vec.size(), const_cast<char**>(vec.data()), logger);

    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(options == nullptr);
    EXPECT_TRUE(logger.str().empty());
    EXPECT_TRUE(output.find("Can't parse expirationDate:") != std::string::npos);
    EXPECT_TRUE(output.find(helpOutput) != std::string::npos);
}

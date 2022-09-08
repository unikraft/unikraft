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

#include "AppOptionsParser.h"
#include <chrono>

namespace intel { namespace sgx { namespace dcap {

namespace {
    static const std::string quoteDefaultPath = "quote.dat";
    static const std::string trustedRootDefaultPath = "trustedRootCaCert.pem";
    static const std::string pckCertDefaultPath = "pckCert.pem";
    static const std::string pckSigningChainDefaultPath = "pckSignChain.pem";
    static const std::string tcbSignChainDefaultPath = "tcbSignChain.pem";
    static const std::string tcbInfoDefaultPath = "tcbInfo.json";
    static const std::string rootCaCrlDefaultPath = "rootCaCrl.der";
    static const std::string intermediateCaCrlDefaultPath = "intermediateCaCrl.der";
    static const std::string qeIdentityDefaultPath = "";
    static const std::string qveIdentityDefaultPath = "";
    static const std::string expirationDateDefault = std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
}

std::unique_ptr<AppOptions> AppOptionsParser::parse(int argc, char **argv, std::ostream& logger)
{
    auto trustedRootCACertificateFile = arg_str0(NULL, "trustedRootCaCert", NULL, "Trusted root CA Certificate file path, PEM format [=trustedRootCaCert.pem]");
    auto pckSigningChainFile = arg_str0(NULL, "pckSignChain", NULL, "PCK Signing Certificate chain file path, PEM format [=pckSignChain.pem]");
    auto pckCertificateFile = arg_str0(NULL, "pckCert", NULL, "PCK Certificate file path, PEM format [=pckCert.pem]");
    auto tcbSigningChainFile = arg_str0(NULL, "tcbSignChain", NULL, "TCB Signing Certificate chain file path, PEM format [=tcbSignChain.pem]");
    auto tcbInfoFile = arg_str0(NULL, "tcbInfo", NULL, "TCB Info file path, JSON format [=tcbInfo.json]");
    auto qeIdentityFile = arg_str0(NULL, "qeIdentity", NULL, "QeIdentity file path, JSON format. QeIdentity verification is optional, will not run by default [=]");
    auto qveIdentityFile = arg_str0(NULL, "qveIdentity", NULL, "QveIdentity file path, JSON format. QveIdentity verification is optional, will not run by default [=]");
    auto rootCaCrlFile = arg_str0(NULL, "rootCaCrl", NULL, "Root Ca CRL file path, PEM or DER format [=rootCaCrl.der]");
    auto intermediateCaCrlFile = arg_str0(NULL, "intermediateCaCrl", NULL, "Intermediate Ca CRL file path, PEM or DER format [=intermediateCaCrl.der]");
    auto quoteFile = arg_str0(NULL, "quote", NULL, "Quote file path, binary format [=quote.dat]");
    auto expirationDate = arg_str0(NULL, "expirationDate", NULL, "Expiration date in timestamp seconds [=seconds]");
    struct arg_lit* help = arg_lit0("h", "help", "Print this message");
    auto end = arg_end(20);

    void *argtable[] = {trustedRootCACertificateFile, pckSigningChainFile, pckCertificateFile,
                        tcbSigningChainFile, tcbInfoFile, qeIdentityFile, qveIdentityFile,
                        rootCaCrlFile, intermediateCaCrlFile, quoteFile, expirationDate, help, end};

    if (arg_nullcheck(argtable) != 0)
    {
        logger << "Can't create argtable" << std::endl;
        return nullptr;
    }

    trustedRootCACertificateFile->sval[0] = trustedRootDefaultPath.c_str();
    pckSigningChainFile->sval[0] = pckSigningChainDefaultPath.c_str();
    pckCertificateFile->sval[0] = pckCertDefaultPath.c_str();
    tcbSigningChainFile->sval[0] = tcbSignChainDefaultPath.c_str();
    tcbInfoFile->sval[0] = tcbInfoDefaultPath.c_str();
    qeIdentityFile->sval[0] = qeIdentityDefaultPath.c_str();
    qveIdentityFile->sval[0] = qveIdentityDefaultPath.c_str();
    rootCaCrlFile->sval[0] = rootCaCrlDefaultPath.c_str();
    intermediateCaCrlFile->sval[0] = intermediateCaCrlDefaultPath.c_str();
    quoteFile->sval[0] = quoteDefaultPath.c_str();
    expirationDate->sval[0] = expirationDateDefault.c_str();


    auto nerrors = arg_parse(argc, argv, argtable);
    if (end && nerrors > 0)
    {
        arg_print_errors(stdout, end, "Sample app");
        printHelp(argtable);
        arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return nullptr;
    }
    if (help && help->count > 0)
    {
        printHelp(argtable);
        arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return nullptr;
    }

    auto options = std::make_unique<AppOptions>();
    options->trustedRootCACertificateFile = std::string(trustedRootCACertificateFile->sval[0]);
    options->pckSigningChainFile = std::string(pckSigningChainFile->sval[0]);
    options->pckCertificateFile = std::string(pckCertificateFile->sval[0]);
    options->tcbSigningChainFile = std::string(tcbSigningChainFile->sval[0]);
    options->tcbInfoFile = std::string(tcbInfoFile->sval[0]);
    options->qeIdentityFile = std::string(qeIdentityFile->sval[0]);
    options->qveIdentityFile = std::string(qveIdentityFile->sval[0]);
    options->rootCaCrlFile = std::string(rootCaCrlFile->sval[0]);
    options->intermediateCaCrlFile = std::string(intermediateCaCrlFile->sval[0]);
    options->quoteFile = std::string(quoteFile->sval[0]);

    try
    {
        options->expirationDate = std::stol(expirationDate->sval[0]);
    }
    catch(std::exception& ex)
    {
        printf("Can't parse expirationDate: %s\n\n", ex.what());
        printHelp(argtable);
        arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return nullptr;
    }
    arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));

    return options;
}

void AppOptionsParser::printHelp(void** argtable)
{
    printf("Usage:");
    arg_print_syntax(stdout, argtable, "\n\n");
    arg_print_glossary(stdout, argtable, "%-40s %s\n");
}

}}}


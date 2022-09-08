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

#ifndef SGX_DCAP_PARSERS_TEST_TCB_INFO_GENERATOR_H
#define SGX_DCAP_PARSERS_TEST_TCB_INFO_GENERATOR_H

#include <string>
#include <vector>
#include <ctime>

namespace intel { namespace sgx { namespace dcap {

extern const std::string validSgxTcb;
extern const std::string validSgxTcbV3;
extern const std::string validTdxTcbV3;
extern const std::string validUpToDateStatus;
extern const std::string validOutOfDateStatus;
extern const std::string validRevokedStatus;
extern const std::string validConfigurationNeededStatus;
extern const std::string validTcbLevelV2Template;
extern const std::string validTcbLevelV3Template;
extern const std::string validTcbInfoV2Template;
extern const std::string validSgxTcbInfoV3Template;
extern const std::string validTdxTcbInfoV3Template;
extern const std::string validSignatureTemplate;

extern const std::vector<uint8_t> DEFAULT_CPUSVN;
extern const int DEFAULT_PCESVN;
extern const std::vector<uint8_t> DEFAULT_FMSPC;
extern const std::vector<uint8_t> DEFAULT_PCEID;
extern const std::vector<uint8_t> DEFAULT_SIGNATURE;
extern const std::vector<uint8_t> DEFAULT_INFO_BODY;
extern const std::string DEFAULT_ISSUE_DATE;
extern const std::string DEFAULT_NEXT_UPDATE;
extern const std::string DEFAULT_TCB_DATE;
extern const int DEFAULT_TCB_TYPE;
extern const int DEFAULT_TCB_EVALUATION_DATA_NUMBER;
extern const std::vector<uint8_t> DEFAULT_TDXMODULE_MRSIGNER;
extern const std::vector<uint8_t> DEFAULT_TDXMODULE_ATTRIBUTES;
extern const std::vector<uint8_t> DEFAULT_TDXMODULE_ATTRIBUTESMASK;

class TcbInfoGenerator
{
public:
/**
 * Generates tcbInfo json based on given template and tcb json
 * @param tcbLevelTemplate template which contains two %s which will be replaced with tcb and status
 * @param tcb json of tcb content
 * @param status tcb status json
 * @return TcbInfo as jsons string
 */
    static std::string generateTcbLevelV2(const std::string &tcbLevelTemplate = validTcbLevelV2Template,
                                          const std::string &tcb = validSgxTcb,
                                          const std::string &status = R"("tcbStatus": "UpToDate")",
                                          const std::string &tcbDate = R"("tcbDate": "2019-05-23T10:36:02Z")",
                                          const std::string &advisoryIDs = R"("advisoryIDs": ["INTEL-SA-00079","INTEL-SA-00076"])");

    static std::string generateTcbLevelV3(const std::string &tcbLevelTemplate = validTcbLevelV3Template,
                                          const std::string &tcb = validSgxTcbV3,
                                          const std::string &status = R"("tcbStatus": "UpToDate")",
                                          const std::string &tcbDate = R"("tcbDate": "2019-05-23T10:36:02Z")",
                                          const std::string &advisoryIDs = R"("advisoryIDs": ["INTEL-SA-00079","INTEL-SA-00076"])");

/**
 * Generates tcbInfo json based on given template and tcb json
 * @param tcbInfoTemplate template which contains two %s which will be replaced with tcbJson and signature
 * @param tcbLevelsJson json of TcbLevels array content
 * @param signature signature over tcbInfo body
 * @return TcbInfo as json string
 */
    static std::string generateTcbInfo(const std::string &tcbInfoTemplate = validTcbInfoV2Template,
                                       const std::string &tcbLevelsJson = generateTcbLevelV2(),
                                       const std::string &signature = validSignatureTemplate);
};

}}}

#endif //SGX_DCAP_PARSERS_TEST_TCB_INFO_GENERATOR_H

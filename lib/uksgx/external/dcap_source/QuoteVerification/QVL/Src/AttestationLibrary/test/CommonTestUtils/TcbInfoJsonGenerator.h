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

#ifndef SGX_DCAP_PARSERS_TEST_TCB_INFO_JSON_GENERATOR_H
#define SGX_DCAP_PARSERS_TEST_TCB_INFO_JSON_GENERATOR_H

#include <string>
#include <array>
#include <vector>

struct TcbComponent
{
    uint8_t svn;
    std::string category;
    std::string type;
};

struct TcbLevelV3
{
    std::array<TcbComponent, 16> sgxTcbComponents;
    std::array<TcbComponent, 16> tdxTcbComponents;
    int pcesvn;
    std::string tcbStatus;
    std::string tcbDate;
};

struct TdxModule
{
    std::vector<uint8_t> mrsigner;
    std::vector<uint8_t> attributes;
    std::vector<uint8_t> attributesMask;
};

std::string bytesToHexString(const std::vector<uint8_t> &vector);

std::string tcbInfoJsonGenerator(int version, std::string issueDate, std::string nextUpdate, std::string fmspc,
                                 std::string pceId, std::array<int, 16> tcb, int pcesvn, std::string status,
                                 std::string signature);

std::string tcbInfoJsonGenerator(std::string tcbInfoBody, std::string signature);

std::string tcbInfoJsonV2Body(uint32_t version, std::string issueDate, std::string nextUpdate, std::string fmspc,
                              std::string pceId, std::array<int, 16> tcb, uint16_t pcesvn, std::string tcbStatus,
                              uint32_t tcbType, uint32_t tcbEvaluationDataNumber, std::string tcbDate);

std::string tcbInfoJsonV3Body(std::string id, uint32_t version, std::string issueDate, std::string nextUpdate,
                              std::string fmspc, std::string pceId, uint32_t tcbType, uint32_t tcbEvaluationDataNumber,
                              std::vector<TcbLevelV3> tcbLevels, bool includeTdxModule, TdxModule tdxModule);

std::array<int, 16> getRandomTcb();
std::array<TcbComponent, 16> getRandomTcbComponent();

#endif //SGX_DCAP_PARSERS_TEST_TCB_INFO_JSON_GENERATOR_H

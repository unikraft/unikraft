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

#include <sstream>
#include <fstream>
#include "FileReader.h"


namespace intel { namespace sgx { namespace dcap {


std::string FileReader::readContent(const std::string& filePath) const
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        throw ReadFileException(std::string("FileReader: failed to open \"") + filePath + "\" file!");
    }

    std::stringstream content;
    content << file.rdbuf();
    return content.str();
}

std::vector<uint8_t> FileReader::readBinaryContent(const std::string& filePath) const
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        throw ReadFileException(std::string("FileReader: failed to open \"") + filePath + "\" binary file!");
    }

    file.seekg(0, std::ios_base::end);
    std::streampos fileSize = file.tellg();

    file.seekg(0, std::ios_base::beg);
    std::vector<uint8_t> retVal((uint32_t) fileSize);
    file.read(reinterpret_cast<char*>(retVal.data()), fileSize);
    return retVal;
}
}}}

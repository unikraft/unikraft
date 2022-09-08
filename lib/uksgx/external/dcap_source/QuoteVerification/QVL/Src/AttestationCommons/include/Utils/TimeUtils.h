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

#ifndef SGX_DCAP_PARSERS_TIMEUTILS_H
#define SGX_DCAP_PARSERS_TIMEUTILS_H


#include <ctime>
#include <string>

namespace intel { namespace sgx { namespace dcap {

struct tm * gmtime(const time_t * timep);
time_t mktime(struct tm* tmp);
time_t getCurrentTime(const time_t *input);
struct tm getTimeFromString(const std::string& date);
time_t getEpochTimeFromString(const std::string& date);
bool isValidTimeString(const std::string& timeString);

#ifndef SGX_TRUSTED
namespace standard
{
    struct tm * gmtime(const time_t * timep);
    time_t mktime(struct tm* tmp);
    time_t getCurrentTime(const time_t *in_time);
    struct tm getTimeFromString(const std::string& date);
    bool isValidTimeString(const std::string& timeString);
}
#endif // SGX_TRUSTED

namespace enclave
{
    struct tm * gmtime(const time_t * timep);
    time_t mktime(struct tm* tmp);

    /**
        calling this function with in_time != 0, will set the static parameter current_time and return its value.
        calling this function with in_time == 0, will return the value of current_time (which should be initialized with in_time!=0).
    */
    time_t getCurrentTime(const time_t *in_time);
    struct tm getTimeFromString(const std::string& date);
    bool isValidTimeString(const std::string& timeString);
}

}}}


#endif //SGX_DCAP_PARSERS_TIMEUTILS_H

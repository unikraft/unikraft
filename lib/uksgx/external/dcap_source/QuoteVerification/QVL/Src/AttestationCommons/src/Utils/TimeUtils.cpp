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

#include "TimeUtils.h"

#include <chrono>

#ifndef SGX_TRUSTED
#include <sstream>
#include <iomanip>
#include <regex>
#include <time.h>

#endif

extern struct tm *
sgxssl__gmtime64(const time_t * timep);
time_t sgxssl_mktime(struct tm*);

namespace intel { namespace sgx { namespace dcap {

struct tm * gmtime(const time_t * timep)
{
#ifdef SGX_TRUSTED
    return enclave::gmtime(timep);
#else // SGX_TRUSTED
    return standard::gmtime(timep);
#endif
}

time_t mktime(struct tm* tmp)
{
#ifdef SGX_TRUSTED
    return enclave::mktime(tmp);
#else // SGX_TRUSTED
    return standard::mktime(tmp);
#endif
}

time_t getCurrentTime(const time_t *in_time)
{
#ifdef SGX_TRUSTED
    return enclave::getCurrentTime(in_time);
#else // SGX_TRUSTED
    return standard::getCurrentTime(in_time);
#endif
}

tm getTimeFromString(const std::string& date)
{
#ifdef SGX_TRUSTED
    return enclave::getTimeFromString(date);
#else // SGX_TRUSTED
    return standard::getTimeFromString(date);
#endif
}

time_t getEpochTimeFromString(const std::string& date)
{
    auto time = getTimeFromString(date);
#ifdef SGX_TRUSTED
    return enclave::mktime(&time);
#else // SGX_TRUSTED
    return standard::mktime(&time);
#endif
}

bool isValidTimeString(const std::string& timeString)
{
#ifdef SGX_TRUSTED
    return enclave::isValidTimeString(timeString);
#else
    return standard::isValidTimeString(timeString);
#endif // SGX_TRUSTED
}

#ifndef SGX_TRUSTED
namespace standard
{
    struct tm * gmtime(const time_t * timep)
    {
        if (timep == nullptr) // avoid undefined behaviour
        {
            throw std::runtime_error("Timestamp has invalid value");
        }
        #ifdef _MSC_VER
            #pragma warning(push)
            #pragma warning(disable: 4996)
        #endif
        return std::gmtime(timep);
        #ifdef _MSC_VER
            #pragma warning(pop)
        #endif
    }
    time_t mktime(struct tm* tmp)
    {
#ifdef _MSC_VER
        return _mkgmtime(tmp);
#else
        return timegm(tmp);
#endif
    }

    time_t getCurrentTime(const time_t *in_time)
    {
        if(in_time == nullptr)
        {
            return std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        }

        return (*in_time);
    }

    bool isValidTimeString(const std::string& timeString)
    {
        // to make sure that this doesn't happen on windows:
        // https://developercommunity.visualstudio.com/content/problem/18311/stdget-time-asserts-with-istreambuf-iterator-is-no.html
        // first check with regex
        std::regex timeRegex("[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z");
        if (!std::regex_match(timeString, timeRegex))
        {
            return false;
        }
        std::tm time{};
        std::istringstream input(timeString);
        input >> std::get_time(&time, "%Y-%m-%dT%H:%M:%SZ");

        //if tm format is incorrect, mktime will modify it. If it's correct time format, it'll keep it.
        //e.g. giving mktime a date of 32/may/2019, it will change it to 2/june/2019, this way we know if the input time is correct or not.
        auto validate = time;
        standard::mktime(&time);
        if (validate.tm_year != time.tm_year || validate.tm_mon != time.tm_mon ||
            validate.tm_mday != time.tm_mday || validate.tm_hour != time.tm_hour ||
            validate.tm_min != time.tm_min || validate.tm_sec != time.tm_sec)
        {
            return false;
        }
        return !input.fail();
    }

    struct tm getTimeFromString(const std::string& date)
    {
        struct tm date_c{};
        std::istringstream input(date);
        input >> std::get_time(&date_c, "%Y-%m-%dT%H:%M:%SZ");
        return date_c;
    }
} // namespace standard
#endif // SGX_TRUSTED

namespace enclave
{
    struct tm * gmtime(const time_t * timep)
    {
        if (timep == nullptr) // avoid undefined behaviour
        {
            throw std::runtime_error("Timestamp has invalid value");
        }
        return sgxssl__gmtime64(timep);
    }

    time_t mktime(struct tm* tmp)
    {
        return sgxssl_mktime(tmp);
    }

    /**
     * Convert an input string to number.
     *
     * @param time_str[IN] - String to be converted.
     * @param count[IN] - Number of chars to be parsed.
     * @param num[OUT] - Pointer to write the result to.
     *
     * @return number of parsed chars
     *
     **/
    uint8_t qvlStringToNum(const char *time_str, uint8_t count, uint32_t *num)
    {
        *num = (uint32_t) (time_str[0] - '0');
        for (int32_t i = 1; i < count; ++i)
        {
            *num = *num * 10 + (uint32_t) (time_str[i] - '0');
        }
        return count;
    }

    /**
     * Convert a string to time.
     *
     * @param str[IN] - String to be converted.
     * @param str_length[IN] - String length.
     * @param datetime[OUT] - Pointer to tm struct to write the result into.
     *
     * @return time_t representation of input time string.
     *
     **/
    time_t qvlStringToTime(const char *str, size_t str_length, struct tm *datetime)
    {
        const char *tmp = str;
        time_t result = -1;
        struct tm validate;
        do {
            if (str == NULL || str_length < 20 || datetime == NULL)
            {
                break;
            }

            tmp += qvlStringToNum(tmp, 4, (uint32_t *) &datetime->tm_year);
            datetime->tm_year = datetime->tm_year - 1900;
            if (datetime->tm_year < 0)
            {
                break;
            }

            if (*tmp != '-')
            {
                break;
            }
            tmp++;

            tmp += qvlStringToNum(tmp, 2, (uint32_t *) &datetime->tm_mon);
            datetime->tm_mon = datetime->tm_mon - 1;
            if (datetime->tm_mon < 0 || datetime->tm_mon > 11)
            {
                break;
            }

            if (*tmp != '-')
            {
                break;
            }
            tmp++;

            tmp += qvlStringToNum(tmp, 2, (uint32_t *) &datetime->tm_mday);
            if (datetime->tm_mday < 1 || datetime->tm_mday > 31)
            {
                break;
            }

            if (*tmp != 'T')
            {
                break;
            }
            tmp++;

            tmp += qvlStringToNum(tmp, 2, (uint32_t *) &datetime->tm_hour);
            if (datetime->tm_hour < 0 || datetime->tm_hour > 23)
            {
                break;
            }

            if (*tmp != ':')
            {
                break;
            }
            tmp++;

            tmp += qvlStringToNum(tmp, 2, (uint32_t *) &datetime->tm_min);
            if (datetime->tm_min < 0 || datetime->tm_min > 59)
            {
                break;
            }

            if (*tmp != ':')
            {
                break;
            }
            tmp++;

            tmp += qvlStringToNum(tmp, 2, (uint32_t *) &datetime->tm_sec);

            if (datetime->tm_sec < 0 || datetime->tm_sec > 59)
            {
                break;
            }

            if (*tmp != 'Z')
            {
                break;
            }
            tmp++;

            datetime->tm_wday = 0;
            datetime->tm_yday = 0;
            datetime->tm_isdst = -1;
            validate = *datetime;
            result = sgxssl_mktime(datetime);

            //if tm format is incorrect, mktime will modify it. If it's correct time format, it'll keep it.
            //e.g. giving mktime a date of 32/may/2019, it will change it to 2/june/2019, this way we know if the input time is correct or not.
            //
            if (validate.tm_year != datetime->tm_year || validate.tm_mon != datetime->tm_mon ||
                validate.tm_mday != datetime->tm_mday || validate.tm_hour != datetime->tm_hour ||
                validate.tm_min != datetime->tm_min || validate.tm_sec != datetime->tm_sec)
            {
                result = -1;
            }
        } while (0);

        return result;
    }

    time_t getCurrentTime(const time_t *in_time)
    {
        if (in_time != nullptr && *in_time != 0)
        {
            return *in_time;
        }

        throw std::runtime_error("Timestamp has invalid value");
    }

    bool isValidTimeString(const std::string& timeString)
    {

        struct tm tmp;
        if (enclave::qvlStringToTime(timeString.c_str(), timeString.length(), &tmp) != -1)
        {
            return true;
        }
        return false;
    }

    struct tm getTimeFromString(const std::string& date)
    {
        struct tm date_c{};
        if (enclave::qvlStringToTime(date.c_str(), date.length(), &date_c) != -1)
        {
            return date_c;
        }
        else
        {
            return {};
        }
    }

} // namespace enclave

}}} // namespace intel { namespace sgx { namespace dcap {

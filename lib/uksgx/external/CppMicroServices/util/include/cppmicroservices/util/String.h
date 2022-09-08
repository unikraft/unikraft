/*=============================================================================

  Library: CppMicroServices

  Copyright (c) The CppMicroServices developers. See the COPYRIGHT
  file at the top-level directory of this distribution and at
  https://github.com/CppMicroServices/CppMicroServices/COPYRIGHT .

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/

#ifndef CPPMICROSERVICES_UTIL_STRING_H
#define CPPMICROSERVICES_UTIL_STRING_H

#include <string>

#if defined(__ANDROID__)
#  include <sstream>
#endif

namespace cppmicroservices {

namespace util {

//-------------------------------------------------------------------
// Android Compatibility functions
//-------------------------------------------------------------------

/**
 * Compatibility functions to replace "std::to_string(...)" functions
 * on Android, since the latest Android NDKs lack "std::to_string(...)"
 * support.
 */

template<typename T>
std::string ToString(T val)
{
#if defined(__ANDROID__)
  std::ostringstream os;
  os << val;
  return os.str();
#else
  return std::to_string(val);
#endif
}

//-------------------------------------------------------------------
// Unicode Utility functions
//-------------------------------------------------------------------

#ifdef US_PLATFORM_WINDOWS
// method to convert UTF8 std::string to std::wstring
// throws std::invalid_argument if input string contains invalid UTF8 characters
std::wstring ToWString(const std::string& inStr);
// method to convert a std::wstring to UTF8 std::string
// throws std::invalid_argument if input string cannot be converted to UTF8
std::string ToUTF8String(const std::wstring& inWStr);
#endif

} // namespace util
} // namespace cppmicroservices

#endif // CPPMICROSERVICES_UTIL_STRING_H

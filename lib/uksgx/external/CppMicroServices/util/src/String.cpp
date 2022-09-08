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

#include "cppmicroservices/util/String.h"
#include "cppmicroservices/util/Error.h"
#include <cppmicroservices/GlobalConfig.h>
#include <exception>
#include <memory>

#ifdef US_PLATFORM_WINDOWS
#  include <windows.h>

namespace cppmicroservices {
namespace util {
//-------------------------------------------------------------------
// Unicode Utility functions
//-------------------------------------------------------------------

inline void ThrowInvalidArgument(const std::string& msgprefix)
{
  throw std::invalid_argument(msgprefix + " Error: " + GetLastWin32ErrorStr());
}

// function returns empty string if inStr is empty or if the conversion failed
std::wstring ToWString(const std::string& inStr)
{
  if (inStr.empty()) {
    return std::wstring();
  }
  int wchar_count = MultiByteToWideChar(CP_UTF8, 0, inStr.c_str(), -1, NULL, 0);
  std::unique_ptr<wchar_t[]> wBuf(new wchar_t[wchar_count]);
  wchar_count =
    MultiByteToWideChar(CP_UTF8, 0, inStr.c_str(), -1, wBuf.get(), wchar_count);
  if (wchar_count == 0) {
    ThrowInvalidArgument("Failed to convert " + inStr + " to UTF16.");
  }
  return wBuf.get();
}

// function return empty string if inWStr is empty or if the conversion failed
std::string ToUTF8String(const std::wstring& inWStr)
{
  if (inWStr.empty()) {
    return std::string();
  }
  int char_count =
    WideCharToMultiByte(CP_UTF8, 0, inWStr.c_str(), -1, NULL, 0, NULL, NULL);
  std::unique_ptr<char[]> str(new char[char_count]);
  char_count = WideCharToMultiByte(
    CP_UTF8, 0, inWStr.c_str(), -1, str.get(), char_count, NULL, NULL);
  if (char_count == 0) {
    ThrowInvalidArgument("Failed to convert UTF16 string to UTF8.");
  }
  return str.get();
}
}
}
#endif

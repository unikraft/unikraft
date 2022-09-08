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

#ifndef CPPMICROSERVICES_UTIL_ERROR_H
#define CPPMICROSERVICES_UTIL_ERROR_H

#include <cppmicroservices/GlobalConfig.h>

#include <exception>
#include <string>

namespace cppmicroservices {

namespace util {

std::string GetLastCErrorStr();
#ifdef US_PLATFORM_WINDOWS
std::string GetLastWin32ErrorStr();
#endif

std::string GetExceptionStr(const std::exception_ptr& exc);
std::string GetLastExceptionStr();

} // namespace util
} // namespace cppmicroservices

#endif // CPPMICROSERVICES_UTIL_ERROR_H

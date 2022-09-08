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

#include "BundleUtils.h"

#include "cppmicroservices/GetBundleContext.h"
#include "cppmicroservices/detail/Log.h"

#include "BundleContextPrivate.h"
#include "BundlePrivate.h"
#include "CoreBundleContext.h"

#ifdef US_PLATFORM_WINDOWS

#  include "cppmicroservices/util/Error.h"
#  include "cppmicroservices/util/String.h"
#  include <windows.h>

#  define RTLD_LAZY 0 // unused

const char* dlerror(void)
{
  static std::string errStr;
  errStr = cppmicroservices::util::GetLastWin32ErrorStr();
  return errStr.c_str();
}

void* dlopen(const char* path, int mode)
{
  (void)mode; // ignored
  auto loadLibrary = [](const std::string& path) -> HANDLE {
    std::wstring wpath(cppmicroservices::util::ToWString(path));
    return LoadLibraryW(wpath.c_str());
  };
  return reinterpret_cast<void*>(path == nullptr ? GetModuleHandleW(nullptr)
                                                 : loadLibrary(path));
}

void* dlsym(void* handle, const char* symbol)
{
  return reinterpret_cast<void*>(
    GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol));
}

#elif defined(__GNUC__)

#  ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#  endif

#  include <dlfcn.h>

#  if defined(__APPLE__)
#    include <mach-o/dyld.h>
#    include <sys/param.h>
#  endif

#  include <unistd.h>

#endif

namespace cppmicroservices {

// Private util function to return system bundle's log sink
std::shared_ptr<detail::LogSink> GetFrameworkLogSink()
{
  // The following is a hack, we need a cleaner solution in the future
  return GetPrivate(GetBundleContext())->bundle->coreCtx->sink;
}

namespace BundleUtils {

void* GetExecutableHandle()
{
  return dlopen(0, RTLD_LAZY);
  ;
}

void* GetSymbol(void* libHandle, const char* symbol)
{
  void* addr = libHandle ? dlsym(libHandle, symbol) : nullptr;
  if (!addr) {
    const char* dlerrorMsg = dlerror();
    DIAG_LOG(*GetFrameworkLogSink())
      << "GetSymbol() failed to find (" << symbol
      << ") with error : " << (dlerrorMsg ? dlerrorMsg : "unknown");
  }
  return addr;
}

} // namespace BundleUtils

} // namespace cppmicroservices

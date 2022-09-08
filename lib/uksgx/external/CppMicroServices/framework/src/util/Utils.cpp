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

#include "Utils.h"

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleContext.h"

#include "cppmicroservices/util/Error.h"
#include "cppmicroservices/util/FileSystem.h"

#include "BundleResourceContainer.h"
#include "CoreBundleContext.h"

#include <string>
#include <typeinfo>
#include <vector>

#ifdef US_HAVE_CXXABI_H
#  include <cxxabi.h>
#endif

namespace {
std::string library_suffix()
{
#ifdef US_PLATFORM_WINDOWS
  return ".dll";
#elif defined(US_PLATFORM_APPLE)
  return ".dylib";
#else
  return ".so";
#endif
}
}

namespace cppmicroservices {

//-------------------------------------------------------------------
// Bundle name and location parsing
//-------------------------------------------------------------------

bool IsSharedLibrary(const std::string& location)
{ // Testing for file extension isn't the most robust way to test
  // for file type.
  return (location.find(library_suffix()) != std::string::npos);
}

bool IsBundleFile(const std::string& location)
{
  if (location.empty()) {
    return false;
  }

  // We require a zip file with at least one top-level directory
  // containing a manifest.json file for a file to be a valid bundle.
  try {
    // If this location is a bundle, a top level directory will
    // contain a manifest.json file at its root. There is no need
    // to recursively search nested directories.
    BundleResourceContainer resContainer(location);
    auto topLevelDirs = resContainer.GetTopLevelDirs();
    return std::any_of(
      topLevelDirs.begin(),
      topLevelDirs.end(),
      [&resContainer](const std::string& dir) -> bool {
        std::vector<std::string> names;
        std::vector<uint32_t> indices;

        resContainer.GetChildren(dir + "/", true, names, indices);
        return std::any_of(names.begin(),
                           names.end(),
                           [](const std::string& resourceName) -> bool {
                             return resourceName ==
                                    std::string("manifest.json");
                           });
      });
  } catch (...) {
    return false;
  }
}

bool OnlyContainsManifest(const std::string& location)
{
  if (location.empty()) {
    throw std::runtime_error("Invalid (empty) location provided.");
  }

  BundleResourceContainer resContainer(location);
  auto topLevelDirs = resContainer.GetTopLevelDirs();
  return std::all_of(
      topLevelDirs.begin(),
      topLevelDirs.end(),
      [&resContainer](const std::string& dir) -> bool {
        std::vector<std::string> names;
        std::vector<uint32_t> indices;

        resContainer.GetChildren(dir + "/", true, names, indices);
        return std::all_of(names.begin(),
                           names.end(),
                           [](const std::string& resourceName) -> bool {
                             return resourceName ==
                                    std::string("manifest.json");
                           });
      });
}

//-------------------------------------------------------------------
// Framework storage
//-------------------------------------------------------------------

const std::string FWDIR_DEFAULT = "fwdir";

std::string GetFrameworkDir(CoreBundleContext* ctx)
{
  auto it = ctx->frameworkProperties.find(Constants::FRAMEWORK_STORAGE);
  if (it == ctx->frameworkProperties.end() ||
      it->second.Type() != typeid(std::string)) {
    return FWDIR_DEFAULT;
  }
  return any_cast<std::string>(it->second);
}

std::string GetPersistentStoragePath(CoreBundleContext* ctx,
                                     const std::string& leafDir,
                                     bool create)
{
  // See if we have a storage directory
  const std::string fwdir(GetFrameworkDir(ctx));
  if (fwdir.empty()) {
    return fwdir;
  }
  const std::string dir =
    util::GetAbsolute(fwdir, ctx->workingDir) + util::DIR_SEP + leafDir;
  if (!dir.empty()) {
    if (util::Exists(dir)) {
      if (!util::IsDirectory(dir)) {
        throw std::runtime_error("Not a directory: " + dir);
      }
    } else {
      if (create) {
        try {
          util::MakePath(dir);
        } catch (const std::exception& e) {
          throw std::runtime_error("Cannot create directory: " + dir + " (" +
                                   e.what() + ")");
        }
      }
    }
  }
  return dir;
}

void TerminateForDebug(const std::exception_ptr ex)
{
#if defined(_MSC_VER) && !defined(NDEBUG) && defined(_DEBUG) &&                \
  defined(_CRT_ERROR)
  std::string message = util::GetLastExceptionStr();

  // get the current report mode
  int reportMode = _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
  _CrtSetReportMode(_CRT_ERROR, reportMode);
  int ret = _CrtDbgReport(_CRT_ERROR,
                          __FILE__,
                          __LINE__,
                          CppMicroServices_VERSION_STR,
                          message.c_str());
  if (ret == 0 && reportMode & _CRTDBG_MODE_WNDW)
    return; // ignore
  else if (ret == 1)
    _CrtDbgBreak();
#else
  (void)ex;
#endif

#ifdef US_PLATFORM_POSIX
  abort(); // trap; generates core dump
#else
  exit(1); // goodbye cruel world
#endif
}

namespace detail {
std::string GetDemangledName(const std::type_info& typeInfo)
{
  std::string result;
#ifdef US_HAVE_CXXABI_H
  int status = 0;
  char* demangled = abi::__cxa_demangle(typeInfo.name(), 0, 0, &status);
  if (demangled && status == 0) {
    result = demangled;
    free(demangled);
  }
#elif defined(US_PLATFORM_WINDOWS)
  const char* demangled = typeInfo.name();
  if (demangled != nullptr) {
    result = demangled;
    // remove "struct" qualifiers
    std::size_t pos = 0;
    while (pos != std::string::npos) {
      if ((pos = result.find("struct ", pos)) != std::string::npos) {
        result = result.substr(0, pos) + result.substr(pos + 7);
        pos += 8;
      }
    }
    // remove "class" qualifiers
    pos = 0;
    while (pos != std::string::npos) {
      if ((pos = result.find("class ", pos)) != std::string::npos) {
        result = result.substr(0, pos) + result.substr(pos + 6);
        pos += 7;
      }
    }
  }
#else
  (void)typeInfo;
#endif
  return result;
}
}

} // namespace cppmicroservices

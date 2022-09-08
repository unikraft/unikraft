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

#ifndef CPPMICROSERVICES_UTIL_FILESYSTEM_H
#define CPPMICROSERVICES_UTIL_FILESYSTEM_H

#include <string>

namespace cppmicroservices {

namespace util {

const char DIR_SEP_WIN32 = '\\';
const char DIR_SEP_POSIX = '/';

extern const char DIR_SEP;

// Get the path of the calling executable.
// Throws std::runtime_error if the path cannot be
// determined.
std::string GetExecutablePath();

// Platform agnostic way to get the current working directory.
// Supports Linux, Mac, and Windows.
std::string GetCurrentWorkingDirectory();
bool Exists(const std::string& path);

bool IsDirectory(const std::string& path);
bool IsFile(const std::string& path);
bool IsRelative(const std::string& path);

std::string GetAbsolute(const std::string& path, const std::string& base);

void MakePath(const std::string& path);

void RemoveDirectoryRecursive(const std::string& path);

} // namespace util
} // namespace cppmicroservices

#endif // CPPMICROSERVICES_UTIL_FILESYSTEM_H

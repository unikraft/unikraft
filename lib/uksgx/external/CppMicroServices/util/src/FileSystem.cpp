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

#include "cppmicroservices/util/FileSystem.h"
#include "cppmicroservices/util/Error.h"
#include "cppmicroservices/util/String.h"
#include <cppmicroservices/GlobalConfig.h>

#include <stdexcept>
#include <string>
#include <vector>

#ifdef US_PLATFORM_POSIX
#  include <dirent.h>
#  include <dlfcn.h>
#  include <errno.h>
#  include <string.h>
#  include <unistd.h> // getcwd

#  define US_STAT struct stat
#  define us_stat stat
#  define us_mkdir mkdir
#  define us_rmdir rmdir
#  define us_unlink unlink
#else
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <Shlwapi.h>
#  include <crtdbg.h>
#  include <direct.h>
#  include <stdint.h>
#  include <windows.h>
#  ifdef __MINGW32__
#    include <dirent.h>
#  else
#    include "dirent_win32.h"
#  endif

#  define US_STAT struct _stat
#  define us_stat _stat
#  define us_mkdir _mkdir
#  define us_rmdir _rmdir
#  define us_unlink _unlink
#endif

#ifdef US_PLATFORM_APPLE
#  include <mach-o/dyld.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

namespace cppmicroservices {

namespace util {

#ifdef US_PLATFORM_WINDOWS
bool not_found_win32_error(int errval)
{
  return errval == ERROR_FILE_NOT_FOUND || errval == ERROR_PATH_NOT_FOUND ||
         errval == ERROR_INVALID_NAME // "//foo"
         ||
         errval == ERROR_INVALID_DRIVE // USB card reader with no card inserted
         || errval == ERROR_NOT_READY  // CD/DVD drive with no disc inserted
         || errval == ERROR_INVALID_PARAMETER // ":sys:stat.h"
         || errval == ERROR_BAD_PATHNAME      // "//nosuch" on Win64
         || errval == ERROR_BAD_NETPATH;      // "//nosuch" on Win32
}
#endif

#ifdef US_PLATFORM_WINDOWS
const char DIR_SEP = DIR_SEP_WIN32;
#else
const char DIR_SEP = DIR_SEP_POSIX;
#endif

bool not_found_c_error(int errval)
{
  return errval == ENOENT || errval == ENOTDIR;
}

std::vector<std::string> SplitString(const std::string& str,
                                     const std::string& delim)
{
  std::vector<std::string> token;
  std::size_t b = str.find_first_not_of(delim);
  std::size_t e = str.find_first_of(delim, b);
  while (e > b) {
    token.emplace_back(str.substr(b, e - b));
    b = str.find_first_not_of(delim, e);
    e = str.find_first_of(delim, b);
  }
  return token;
}

#ifndef MAXPATHLEN
#  define MAXPATHLEN 1024
#endif

std::string GetExecutablePath()
{
  uint32_t bufsize = MAXPATHLEN;
#ifdef US_PLATFORM_WINDOWS
  std::vector<wchar_t> wbuf(bufsize + 1, '\0');
  if (GetModuleFileNameW(nullptr, wbuf.data(), bufsize) == 0 ||
      GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    throw std::runtime_error("GetModuleFileName failed" +
                             GetLastWin32ErrorStr());
  }
  return ToUTF8String(wbuf.data());
#elif defined(US_PLATFORM_APPLE)
  std::vector<char> buf(bufsize + 1, '\0');
  int status = _NSGetExecutablePath(buf.data(), &bufsize);
  if (status == -1) {
    buf.assign(bufsize + 1, '\0');
    status = _NSGetExecutablePath(buf.data(), &bufsize);
  }
  if (status != 0) {
    throw std::runtime_error("_NSGetExecutablePath() failed");
  }
  // the returned path may not be an absolute path
  return buf.data();
#elif defined(US_PLATFORM_LINUX)
  std::vector<char> buf(bufsize + 1, '\0');
  ssize_t len = ::readlink("/proc/self/exe", buf.data(), bufsize);
  if (len == -1 || len == static_cast<ssize_t>(bufsize)) {
    throw std::runtime_error("Could not read /proc/self/exe into buffer");
  }
  return buf.data();
#else
  // 'dlsym' does not work with symbol name 'main'
  throw std::runtime_error("GetExecutablePath failed");
  return "";
#endif
}

std::string InitCurrentWorkingDirectory()
{
#ifdef US_PLATFORM_WINDOWS
  DWORD bufSize = ::GetCurrentDirectoryW(0, NULL);
  if (bufSize == 0)
    bufSize = 1;
  std::vector<wchar_t> buf(bufSize, L'\0');
  if (::GetCurrentDirectoryW(bufSize, buf.data()) != 0) {
    return util::ToUTF8String(buf.data());
  }
#else
  std::size_t bufSize = PATH_MAX;
  for (;; bufSize *= 2) {
    std::vector<char> buf(bufSize, '\0');
    errno = 0;
    if (getcwd(buf.data(), bufSize) != 0 && errno != ERANGE) {
      return std::string(buf.data());
    }
  }
#endif
  return std::string();
}

static const std::string s_CurrWorkingDir = InitCurrentWorkingDirectory();

std::string GetCurrentWorkingDirectory()
{
  return s_CurrWorkingDir;
}

bool Exists(const std::string& path)
{
#ifdef US_PLATFORM_POSIX
  US_STAT s;
  errno = 0;
  if (us_stat(path.c_str(), &s)) {
    if (not_found_c_error(errno))
      return false;
    else
      throw std::invalid_argument(GetLastCErrorStr());
  }
#else
  std::wstring wpath(ToWString(path));
  DWORD attr(::GetFileAttributesW(wpath.c_str()));
  if (attr == INVALID_FILE_ATTRIBUTES) {
    if (not_found_win32_error(::GetLastError()))
      return false;
    else
      throw std::invalid_argument(GetLastWin32ErrorStr());
  }
#endif
  return true;
}

bool IsDirectory(const std::string& path)
{
  US_STAT s;
  errno = 0;
  if (us_stat(path.c_str(), &s)) {
    if (not_found_c_error(errno))
      return false;
    else
      throw std::invalid_argument(GetLastCErrorStr());
  }
  return S_ISDIR(s.st_mode);
}

bool IsFile(const std::string& path)
{
  US_STAT s;
  errno = 0;
  if (us_stat(path.c_str(), &s)) {
    if (not_found_c_error(errno))
      return false;
    else
      throw std::invalid_argument(GetLastCErrorStr());
  }
  return S_ISREG(s.st_mode);
}

bool IsRelative(const std::string& path)
{
#ifdef US_PLATFORM_WINDOWS
  if (path.size() > MAX_PATH)
    return false;
  std::wstring wpath(ToWString(path));
  return (TRUE == ::PathIsRelativeW(wpath.c_str())) ? true : false;
#else
  return path.empty() || path[0] != DIR_SEP;
#endif
}

std::string GetAbsolute(const std::string& path, const std::string& base)
{
  if (IsRelative(path))
    return base + DIR_SEP + path;
  return path;
}

void MakePath(const std::string& path)
{
  std::string subPath;
  auto dirs = SplitString(path, std::string() + DIR_SEP_WIN32 + DIR_SEP_POSIX);
  if (dirs.empty())
    return;

  auto iter = dirs.begin();
#ifdef US_PLATFORM_POSIX
  // Start with the root '/' directory
  subPath = DIR_SEP;
#else
  // Start with the drive letter`
  subPath = *iter + DIR_SEP;
  ++iter;
#endif
  for (; iter != dirs.end(); ++iter) {
    subPath += *iter;
    errno = 0;
#ifdef US_PLATFORM_WINDOWS
    if (us_mkdir(subPath.c_str()))
#else
    if (us_mkdir(subPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
#endif
    {
      if (errno != EEXIST)
        throw std::invalid_argument(GetLastCErrorStr());
    }
    subPath += DIR_SEP;
  }
}

void RemoveDirectoryRecursive(const std::string& path)
{
  int res = -1;
  errno = 0;
  DIR* dir = opendir(path.c_str());
  if (dir != nullptr) {
    res = 0;

    struct dirent* ent = nullptr;
    while (!res && (ent = readdir(dir)) != nullptr) {
      // Skip the names "." and ".." as we don't want to recurse on them.
      if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
        continue;
      }

      std::string child = path + DIR_SEP + ent->d_name;
      if
#ifdef _DIRENT_HAVE_D_TYPE
        (ent->d_type == DT_DIR)
#else
        (IsDirectory(child))
#endif
      {
        RemoveDirectoryRecursive(child);
      } else {
        res = us_unlink(child.c_str());
      }
    }
    int old_err = errno;
    errno = 0;
    closedir(dir); // error ignored
    if (old_err) {
      errno = old_err;
    }
  }

  if (!res) {
    errno = 0;
    res = us_rmdir(path.c_str());
  }

  if (res)
    throw std::invalid_argument(GetLastCErrorStr());
}

} // namespace util
} // namespace cppmicroservices

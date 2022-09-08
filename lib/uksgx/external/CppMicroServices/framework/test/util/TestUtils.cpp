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

#include "TestUtils.h"

#include "cppmicroservices/GetBundleContext.h"
#include "cppmicroservices/GlobalConfig.h"

#include "cppmicroservices/util/Error.h"
#include "cppmicroservices/util/FileSystem.h"

#include <TestingConfig.h>

#ifndef US_PLATFORM_WINDOWS
#  include <sys/time.h> // gettimeofday etc.
#else
#  include <direct.h>
#  include <io.h>
#endif

#ifdef US_PLATFORM_APPLE
#  include <unistd.h> // chdir, getpid, close, etc.
#endif

#include <fcntl.h>
#include <sys/stat.h> // mkdir, _S_IREAD, etc.

namespace cppmicroservices {

namespace testing {

#if defined(US_PLATFORM_APPLE)

double HighPrecisionTimer::timeConvert = 0.0;

HighPrecisionTimer::HighPrecisionTimer()
  : startTime(0)
{
  if (timeConvert == 0) {
    mach_timebase_info_data_t timeBase;
    mach_timebase_info(&timeBase);
    timeConvert = static_cast<double>(timeBase.numer) /
                  static_cast<double>(timeBase.denom) / 1000.0;
  }
}

void HighPrecisionTimer::Start()
{
  startTime = mach_absolute_time();
}

long long HighPrecisionTimer::ElapsedMilli()
{
  uint64_t current = mach_absolute_time();
  return static_cast<double>(current - startTime) * timeConvert / 1000.0;
}

long long HighPrecisionTimer::ElapsedMicro()
{
  uint64_t current = mach_absolute_time();
  return static_cast<double>(current - startTime) * timeConvert;
}

#elif defined(US_PLATFORM_POSIX)

HighPrecisionTimer::HighPrecisionTimer()
{
  startTime.tv_nsec = 0;
  startTime.tv_sec = 0;
}

void HighPrecisionTimer::Start()
{
  clock_gettime(CLOCK_MONOTONIC, &startTime);
}

long long HighPrecisionTimer::ElapsedMilli()
{
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  return (static_cast<long long>(current.tv_sec) * 1000 +
          current.tv_nsec / 1000 / 1000) -
         (static_cast<long long>(startTime.tv_sec) * 1000 +
          startTime.tv_nsec / 1000 / 1000);
}

long long HighPrecisionTimer::ElapsedMicro()
{
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  return (static_cast<long long>(current.tv_sec) * 1000 * 1000 +
          current.tv_nsec / 1000) -
         (static_cast<long long>(startTime.tv_sec) * 1000 * 1000 +
          startTime.tv_nsec / 1000);
}

#elif defined(US_PLATFORM_WINDOWS)

HighPrecisionTimer::HighPrecisionTimer()
{
  if (!QueryPerformanceFrequency(&timerFrequency))
    throw std::runtime_error("QueryPerformanceFrequency() failed");
}

void HighPrecisionTimer::Start()
{
  //DWORD_PTR oldmask = SetThreadAffinityMask(GetCurrentThread(), 0);
  QueryPerformanceCounter(&startTime);
  //SetThreadAffinityMask(GetCurrentThread(), oldmask);
}

long long HighPrecisionTimer::ElapsedMilli()
{
  LARGE_INTEGER current;
  QueryPerformanceCounter(&current);
  return (current.QuadPart - startTime.QuadPart) /
         (timerFrequency.QuadPart / 1000);
}

long long HighPrecisionTimer::ElapsedMicro()
{
  LARGE_INTEGER current;
  QueryPerformanceCounter(&current);
  return (current.QuadPart - startTime.QuadPart) /
         (timerFrequency.QuadPart / (1000 * 1000));
}

#endif

Bundle InstallLib(BundleContext frameworkCtx, const std::string& libName)
{
  std::vector<Bundle> bundles;

#if defined(US_BUILD_SHARED_LIBS)
  bundles = frameworkCtx.InstallBundles(LIB_PATH + util::DIR_SEP +
                                        US_LIB_PREFIX + libName + US_LIB_EXT);
#else
  bundles = frameworkCtx.GetBundles();
#endif

  for (auto b : bundles) {
    if (b.GetSymbolicName() == libName)
      return b;
  }
  return {};
}

void ChangeDirectory(const std::string& destdir)
{
  errno = 0;
  int ret = chdir(destdir.c_str());
  if (ret != 0) {
    const std::string msg = "Unable to change directory to " + destdir + ": " +
                            util::GetLastCErrorStr();
    throw std::runtime_error(msg);
  }
}

std::string GetTempDirectory()
{
#if defined(US_PLATFORM_WINDOWS)
  std::wstring temp_dir;
  wchar_t wcharPath[MAX_PATH];
  if (GetTempPathW(MAX_PATH, wcharPath)) {
    temp_dir = wcharPath;
  }

  return std::string(temp_dir.cbegin(), temp_dir.cend());
#else
  char* tempdir = getenv("TMPDIR");
  return std::string(((tempdir == nullptr) ? "/tmp" : tempdir));
#endif
}

static const char validLetters[] =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

// A cross-platform version of the mkstemps function
static int mkstemps_compat(char* tmpl, int suffixlen)
{
  static unsigned long long value = 0;
  int savedErrno = errno;

// Lower bound on the number of temporary files to attempt to generate.
#define ATTEMPTS_MIN (62 * 62 * 62)

/* The number of times to attempt to generate a temporary file.  To
   conform to POSIX, this must be no smaller than TMP_MAX.  */
#if ATTEMPTS_MIN < TMP_MAX
  const unsigned int attempts = TMP_MAX;
#else
  const unsigned int attempts = ATTEMPTS_MIN;
#endif

  const std::size_t len = strlen(tmpl);
  if ((len - suffixlen) < 6 ||
      strncmp(&tmpl[len - 6 - suffixlen], "XXXXXX", 6)) {
    errno = EINVAL;
    return -1;
  }

  /* This is where the Xs start.  */
  char* XXXXXX = &tmpl[len - 6 - suffixlen];

/* Get some more or less random data.  */
#ifdef US_PLATFORM_WINDOWS
  {
    SYSTEMTIME stNow;
    FILETIME ftNow;

    // get system time
    GetSystemTime(&stNow);
    stNow.wMilliseconds = 500;
    if (!SystemTimeToFileTime(&stNow, &ftNow)) {
      errno = -1;
      return -1;
    }
    unsigned long long randomTimeBits =
      ((static_cast<unsigned long long>(ftNow.dwHighDateTime) << 32) |
       static_cast<unsigned long long>(ftNow.dwLowDateTime));
    value =
      randomTimeBits ^ static_cast<unsigned long long>(GetCurrentThreadId());
  }
#else
  {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    unsigned long long randomTimeBits =
      ((static_cast<unsigned long long>(tv.tv_usec) << 32) |
       static_cast<unsigned long long>(tv.tv_sec));
    value = randomTimeBits ^ static_cast<unsigned long long>(getpid());
  }
#endif

  for (unsigned int count = 0; count < attempts; value += 7777, ++count) {
    unsigned long long v = value;

    /* Fill in the random bits.  */
    XXXXXX[0] = validLetters[v % 62];
    v /= 62;
    XXXXXX[1] = validLetters[v % 62];
    v /= 62;
    XXXXXX[2] = validLetters[v % 62];
    v /= 62;
    XXXXXX[3] = validLetters[v % 62];
    v /= 62;
    XXXXXX[4] = validLetters[v % 62];
    v /= 62;
    XXXXXX[5] = validLetters[v % 62];

#ifdef US_PLATFORM_WINDOWS
    int fd = _open(tmpl, O_RDWR | O_CREAT | O_EXCL, _S_IREAD | _S_IWRITE);
#else
    int fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
#endif
    if (fd >= 0) {
      errno = savedErrno;
      return fd;
    } else if (errno != EEXIST) {
      return -1;
    }
  }

  /* We got out of the loop because we ran out of combinations to try.  */
  errno = EEXIST;
  return -1;
}

// A cross-platform version of the POSIX mkdtemp function
char* mkdtemps_compat(char* tmpl, int suffixlen)
{
  static unsigned long long value = 0;
  int savedErrno = errno;

// Lower bound on the number of temporary dirs to attempt to generate.
#define ATTEMPTS_MIN (62 * 62 * 62)

/* The number of times to attempt to generate a temporary dir.  To
   conform to POSIX, this must be no smaller than TMP_MAX.  */
#if ATTEMPTS_MIN < TMP_MAX
  const unsigned int attempts = TMP_MAX;
#else
  const unsigned int attempts = ATTEMPTS_MIN;
#endif

  const std::size_t len = strlen(tmpl);
  if ((len - suffixlen) < 6 ||
      strncmp(&tmpl[len - 6 - suffixlen], "XXXXXX", 6)) {
    errno = EINVAL;
    return nullptr;
  }

  /* This is where the Xs start.  */
  char* XXXXXX = &tmpl[len - 6 - suffixlen];

/* Get some more or less random data.  */
#ifdef US_PLATFORM_WINDOWS
  {
    SYSTEMTIME stNow;
    FILETIME ftNow;

    // get system time
    GetSystemTime(&stNow);
    stNow.wMilliseconds = 500;
    if (!SystemTimeToFileTime(&stNow, &ftNow)) {
      errno = -1;
      return nullptr;
    }
    unsigned long long randomTimeBits =
      ((static_cast<unsigned long long>(ftNow.dwHighDateTime) << 32) |
       static_cast<unsigned long long>(ftNow.dwLowDateTime));
    value =
      randomTimeBits ^ static_cast<unsigned long long>(GetCurrentThreadId());
  }
#else
  {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    unsigned long long randomTimeBits =
      ((static_cast<unsigned long long>(tv.tv_usec) << 32) |
       static_cast<unsigned long long>(tv.tv_sec));
    value = randomTimeBits ^ static_cast<unsigned long long>(getpid());
  }
#endif

  unsigned int count = 0;
  for (; count < attempts; value += 7777, ++count) {
    unsigned long long v = value;

    /* Fill in the random bits.  */
    XXXXXX[0] = validLetters[v % 62];
    v /= 62;
    XXXXXX[1] = validLetters[v % 62];
    v /= 62;
    XXXXXX[2] = validLetters[v % 62];
    v /= 62;
    XXXXXX[3] = validLetters[v % 62];
    v /= 62;
    XXXXXX[4] = validLetters[v % 62];
    v /= 62;
    XXXXXX[5] = validLetters[v % 62];

#ifdef US_PLATFORM_WINDOWS
    int r = _mkdir(tmpl); //, _S_IREAD | _S_IWRITE | _S_IEXEC);
#else
    int r = mkdir(tmpl, S_IRUSR | S_IWUSR | S_IXUSR);
#endif
    if (r >= 0) {
      errno = savedErrno;
      return tmpl;
    } else if (errno != EEXIST) {
      return nullptr;
    }
  }

  /* We got out of the loop because we ran out of combinations to try.  */
  errno = EEXIST;
  return nullptr;
}

File::File()
  : FileDescr(-1)
  , Path()
{}

File::File(int fd, const std::string& path)
  : FileDescr(fd)
  , Path(path)
{}

File::File(File&& o)
  : FileDescr(o.FileDescr)
  , Path(std::move(o.Path))
{
  o.FileDescr = -1;
}

File& File::operator=(File&& o)
{
  std::swap(FileDescr, o.FileDescr);
  std::swap(Path, o.Path);
  return *this;
}

File::~File()
{
  if (FileDescr >= 0)
    close(FileDescr);
}

TempDir::TempDir() {}

TempDir::TempDir(const std::string& path)
  : Path(path)
{}

TempDir::TempDir(TempDir&& o)
  : Path(std::move(o.Path))
{}

TempDir& TempDir::operator=(TempDir&& o)
{
  std::swap(Path, o.Path);
  return *this;
}

TempDir::~TempDir()
{
  if (!Path.empty()) {
    try {
      util::RemoveDirectoryRecursive(Path);
    } catch (const std::exception&) {
      // ignored
    }
  }
}

TempDir::operator std::string() const
{
  return Path;
}

std::string MakeUniqueTempDirectory()
{
  std::string tmpStr = GetTempDirectory();
  if (!tmpStr.empty() && *--tmpStr.end() != util::DIR_SEP) {
    tmpStr += util::DIR_SEP;
  }
  tmpStr += "usdir-XXXXXX";
  std::vector<char> tmpChars(tmpStr.c_str(),
                             tmpStr.c_str() + tmpStr.length() + 1);

  errno = 0;
  if (!mkdtemps_compat(tmpChars.data(), 0))
    throw std::runtime_error(util::GetLastCErrorStr());

  return tmpChars.data();
}

File MakeUniqueTempFile(const std::string& base)
{
  const auto tmpStr = base + util::DIR_SEP + "usfile-XXXXXX";
  std::vector<char> tmpChars(tmpStr.c_str(),
                             tmpStr.c_str() + tmpStr.length() + 1);

  errno = 0;
  int fd = mkstemps_compat(tmpChars.data(), 0);
  if (fd < 0)
    throw std::runtime_error(util::GetLastCErrorStr());

  return File(fd, tmpChars.data());
}

Bundle GetBundle(const std::string& bsn, BundleContext context)
{
  if (!context)
    context = cppmicroservices::GetBundleContext();
  for (auto b : context.GetBundles()) {
    if (b.GetSymbolicName() == bsn)
      return b;
  }
  return {};
}
}
}

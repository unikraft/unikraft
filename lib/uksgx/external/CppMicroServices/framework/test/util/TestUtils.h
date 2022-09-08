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

#ifndef CPPMICROSERVICES_TESTUTILS_H
#define CPPMICROSERVICES_TESTUTILS_H

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleContext.h"

#include <string>

#ifdef US_PLATFORM_APPLE
#  include <mach/mach_time.h>
#elif defined(US_PLATFORM_POSIX)
#  include <limits.h>
#  include <time.h>
#  include <unistd.h>
#  ifndef _POSIX_MONOTONIC_CLOCK
#    error Monotonic clock support missing on this POSIX platform
#  endif
#elif defined(US_PLATFORM_WINDOWS)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef VC_EXTRA_LEAN
#    define VC_EXTRA_LEAN
#  endif
#  include <windows.h>
#else
#  error High precision timer support not available on this platform
#endif

namespace cppmicroservices {

namespace testing {

class HighPrecisionTimer
{

public:
  HighPrecisionTimer();

  void Start();

  long long ElapsedMilli();

  long long ElapsedMicro();

private:
#ifdef US_PLATFORM_APPLE
  static double timeConvert;
  uint64_t startTime;
#elif defined(US_PLATFORM_POSIX)
  timespec startTime;
#elif defined(US_PLATFORM_WINDOWS)
  LARGE_INTEGER timerFrequency;
  LARGE_INTEGER startTime;
#endif
};

// Helper function to install bundles, given a framework's bundle context and the name of the library.
// Assumes that test bundles are within the same directory during unit testing.
Bundle InstallLib(BundleContext frameworkCtx, const std::string& libName);

/*
* Change to destination directory specified by destdir
* @throws std::runtime_error if the directory cannot be changed to
*/
void ChangeDirectory(const std::string& destdir);

/*
* Returns a platform appropriate location for use as temporary storage.
*/
std::string GetTempDirectory();

/*
 * Closes the file descriptor on destruction.
 */
struct File
{
  File(const File&) = delete;
  File& operator=(const File&) = delete;

  File();

  // The file descriptor fd is owned by this
  File(int fd, const std::string& path);

  File(File&& o);
  File& operator=(File&& o);

  ~File();

  int FileDescr;
  std::string Path;
};

/*
 * Removes the directory on destruction.
 */
struct TempDir
{
  TempDir(const TempDir&) = delete;
  TempDir& operator=(const TempDir&) = delete;

  TempDir();

  // The file descriptor fd is owned by this
  TempDir(const std::string& path);

  TempDir(TempDir&& o);
  TempDir& operator=(TempDir&& o);

  ~TempDir();

  operator std::string() const;

  std::string Path;
};

/*
 * Creates a new unique sub-directory in the temporary storage
 * location returned by GetTempDirectory() and returns a string
 * containing the directory path.
 *
 * This is similar to mkdtemp on POSIX systems, but without a
 * custom template string.
 */
std::string MakeUniqueTempDirectory();

File MakeUniqueTempFile(const std::string& base);

Bundle GetBundle(const std::string& bsn,
                 BundleContext context = BundleContext());

} // namespace testing
} // namespace cppmicroservices

#endif // CPPMICROSERVICES_TESTUTILS_H

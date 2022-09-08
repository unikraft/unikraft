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

#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/GetBundleContext.h"
#include "cppmicroservices/util/FileSystem.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

#include <mutex>
#include <thread>
#include <vector>

using namespace cppmicroservices;

#ifdef US_BUILD_SHARED_LIBS
namespace {
std::mutex mutex_io = {};
std::unique_lock<std::mutex> io_lock()
{
  return std::unique_lock<std::mutex>(mutex_io);
}

// Attempt to get as close an approximation as to how long it takes to install a bundle
// without having the extra machinery of error handling in the way.
inline void InstallTestBundleNoErrorHandling(BundleContext frameworkCtx,
                                             const std::string& bundleName)
{
  frameworkCtx.InstallBundles(testing::LIB_PATH + util::DIR_SEP +
                              US_LIB_PREFIX + bundleName + US_LIB_EXT);
}

void TestSerial(const Framework& f)
{
  // Installing such a small set of bundles doesn't yield significant
  // data about performance. Consider increasing the number of bundles
  // used.
  auto bc = f.GetBundleContext();

  testing::HighPrecisionTimer timer;
  timer.Start();
  InstallTestBundleNoErrorHandling(bc, "TestBundleA");
  InstallTestBundleNoErrorHandling(bc, "TestBundleA2");
  InstallTestBundleNoErrorHandling(bc, "TestBundleB");
#  ifdef US_ENABLE_THREADING_SUPPORT
  InstallTestBundleNoErrorHandling(bc, "TestBundleC1");
#  endif
  InstallTestBundleNoErrorHandling(bc, "TestBundleH");
  InstallTestBundleNoErrorHandling(bc, "TestBundleLQ");
  InstallTestBundleNoErrorHandling(bc, "TestBundleM");
  InstallTestBundleNoErrorHandling(bc, "TestBundleR");
  InstallTestBundleNoErrorHandling(bc, "TestBundleRA");
  InstallTestBundleNoErrorHandling(bc, "TestBundleRL");
  InstallTestBundleNoErrorHandling(bc, "TestBundleS");
  InstallTestBundleNoErrorHandling(bc, "TestBundleSL1");
  InstallTestBundleNoErrorHandling(bc, "TestBundleSL3");
  InstallTestBundleNoErrorHandling(bc, "TestBundleSL4");

  long long elapsedTimeInMilliSeconds = timer.ElapsedMilli();
  io_lock(), US_TEST_OUTPUT(<< "[thread " << std::this_thread::get_id()
                            << "] Time elapsed to install 12 new bundles: "
                            << elapsedTimeInMilliSeconds << " milliseconds");

  elapsedTimeInMilliSeconds = 0;

  auto bundles = bc.GetBundles();
  for (auto& bundle : bundles) {
    timer.Start();
    try {
      bundle.Start();
    } catch (const std::exception& e) {
      io_lock(), US_TEST_OUTPUT(<< "[thread " << std::this_thread::get_id()
                                << "] exception: " << e.what());
    }

    elapsedTimeInMilliSeconds += timer.ElapsedMilli();
  }

  io_lock(), US_TEST_OUTPUT(<< "[thread " << std::this_thread::get_id()
                            << "] Time elapsed to start 12 bundles: "
                            << elapsedTimeInMilliSeconds << " milliseconds");
}

#  ifdef US_ENABLE_THREADING_SUPPORT
void TestConcurrent(const Framework& f)
{
  // This is by no means a "real world" example. At best it is a simulation to test
  // the performance of concurrent access to the bundle registry.
  // At any point in which real customer usage in a concurrent way becomes known,
  // it would be ideal to model it as a test.

  std::size_t numTestBundles =
    16; // 14 test bundles + 1 statically linked bundle + the system bundle
  numTestBundles += 1; // main
  const int numTestThreads = 50;
  std::vector<std::thread> threads;
  for (int i = 0; i < numTestThreads; ++i) {
    threads.emplace_back(TestSerial, f);
    threads.emplace_back([f] { f.GetBundleContext().GetBundles(); });
  }

  for (auto& th : threads)
    th.join();
  US_TEST_CONDITION(numTestBundles == f.GetBundleContext().GetBundles().size(),
                    "Test for correct number of installed bundles")
}
#  endif

} // end anonymous namespace
#endif

int BundleRegistryPerformanceTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("BundleRegistryPerformanceTest")

  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Start();
#ifdef US_BUILD_SHARED_LIBS
  US_TEST_OUTPUT(<< "Testing serial installation of bundles");
  TestSerial(framework);

  for (auto bundle : framework.GetBundleContext().GetBundles()) {
    if (bundle.GetBundleId() != 0 && bundle.GetSymbolicName() != "main") {
      bundle.Uninstall();
    }
  }
#  ifdef US_ENABLE_THREADING_SUPPORT
  US_TEST_OUTPUT(<< "Testing concurrent installation of bundles");
  TestConcurrent(framework);
#  endif
#endif
  framework.Stop();

  US_TEST_END()
}

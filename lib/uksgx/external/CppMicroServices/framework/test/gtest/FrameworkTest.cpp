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

#include <chrono>
#include <fstream>
#include <mutex>
#include <thread>
#include <type_traits>

#include "TestUtilBundleListener.h"
#include "TestUtils.h"
#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/BundleEvent.h"
#include "cppmicroservices/Constants.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/util/FileSystem.h"
#include "gtest/gtest.h"

using namespace cppmicroservices;
using cppmicroservices::testing::File;
using cppmicroservices::testing::GetTempDirectory;
using cppmicroservices::testing::MakeUniqueTempDirectory;
using cppmicroservices::testing::TempDir;

#if !defined(__clang__) && defined(__GNUC__)
#  define US_GCC_VER                                                           \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

// TODO: Remove all occurences of US_TYPE_OPERATIONS_AVAILABLE macro
// once the minimum GCC compiler required is 4.7 or above
#if defined(US_GCC_VER) && (US_GCC_VER < 40700)
#  define US_TYPE_OPERATIONS_AVAILABLE 0
#else
#  define US_TYPE_OPERATIONS_AVAILABLE 1
#endif

TEST(FrameworkTest, Ctor)
{
#if US_TYPE_OPERATIONS_AVAILABLE
  ASSERT_FALSE(std::is_default_constructible<Framework>::value);
  ASSERT_TRUE((std::is_constructible<Framework, Bundle>::value));
#endif
  //Bundle b;
  //ASSERT_THROW(Framework(Bundle(b)), std::logic_error); This causes a crash. TODO: Fix crash and uncomment this line.

  auto f = FrameworkFactory().NewFramework();
  ASSERT_TRUE(f);
  f.Start();
#if defined(US_BUILD_SHARED_LIBS)
  auto bundle =
    cppmicroservices::testing::InstallLib(f.GetBundleContext(), "TestBundleA");
#else
  auto bundle =
    cppmicroservices::testing::GetBundle("TestBundleA", f.GetBundleContext());
#endif
  ASSERT_THROW(auto f1 = Framework(Bundle(bundle)), std::logic_error);
}

TEST(FrameworkTest, MoveCtor)
{
#if US_TYPE_OPERATIONS_AVAILABLE
  ASSERT_TRUE(std::is_move_constructible<Framework>::value);
#endif
  auto f = FrameworkFactory().NewFramework();
  ASSERT_TRUE(f);
  f.Start();
  ASSERT_EQ(f.GetState(), Bundle::STATE_ACTIVE);
  Framework f1(std::move(f));
  ASSERT_EQ(f1.GetState(), Bundle::STATE_ACTIVE);
}

TEST(FrameworkTest, MoveAssign)
{
#if US_TYPE_OPERATIONS_AVAILABLE
  ASSERT_TRUE(std::is_move_assignable<Framework>::value);
#endif
  auto f = FrameworkFactory().NewFramework();
  ASSERT_TRUE(f);
  f.Start();
  ASSERT_EQ(f.GetState(), Bundle::STATE_ACTIVE);
  Framework f1 = FrameworkFactory().NewFramework();
  ASSERT_TRUE(f1);
  ASSERT_EQ(f1.GetState(), Bundle::STATE_INSTALLED);
  f1 = std::move(f);
  ASSERT_EQ(f1.GetState(), Bundle::STATE_ACTIVE);
}

TEST(FrameworkTest, CopyCtor)
{
#if US_TYPE_OPERATIONS_AVAILABLE
  ASSERT_TRUE(std::is_copy_constructible<Framework>::value);
#endif
  auto f = FrameworkFactory().NewFramework();
  ASSERT_TRUE(f);
  f.Start();
  ASSERT_EQ(f.GetState(), Bundle::STATE_ACTIVE);
  Framework f1(f);
  ASSERT_EQ(f1.GetState(), Bundle::STATE_ACTIVE);
  f1.Stop();
  f1.WaitForStop(std::chrono::milliseconds::zero());
  ASSERT_NE(f1.GetState(), Bundle::STATE_ACTIVE);
  ASSERT_EQ(f.GetState(), f1.GetState());
}

TEST(FrameworkTest, CopyAssign)
{
#if US_TYPE_OPERATIONS_AVAILABLE
  ASSERT_TRUE(std::is_copy_assignable<Framework>::value);
#endif
  auto f = FrameworkFactory().NewFramework();
  ASSERT_TRUE(f);
  f.Start();
  ASSERT_EQ(f.GetState(), Bundle::STATE_ACTIVE);
  Framework f1 = FrameworkFactory().NewFramework();
  ASSERT_TRUE(f1);
  ASSERT_EQ(f1.GetState(), Bundle::STATE_INSTALLED);
  f1 = f;
  ASSERT_EQ(f1.GetState(), Bundle::STATE_ACTIVE);
  f1.Stop();
  f1.WaitForStop(std::chrono::milliseconds::zero());
  ASSERT_NE(f1.GetState(), Bundle::STATE_ACTIVE);
  ASSERT_EQ(f.GetState(), f1.GetState());
}

TEST(FrameworkTest, DefaultConfig)
{
  auto f = FrameworkFactory().NewFramework();
  ASSERT_TRUE(f);

  f.Start();

  auto ctx = f.GetBundleContext();

  // Default framework properties:
  //  - threading model: multi
  //  - persistent storage location: The current working directory
  //  - diagnostic logging: off
  //  - diagnostic logger: std::clog
#ifdef US_ENABLE_THREADING_SUPPORT
  ASSERT_EQ(ctx.GetProperty(Constants::FRAMEWORK_THREADING_SUPPORT).ToString(),
            "multi");
#else
  ASSERT_EQ(ctx.GetProperty(Constants::FRAMEWORK_THREADING_SUPPORT).ToString(),
            "single");
#endif
  ASSERT_EQ(ctx.GetProperty(Constants::FRAMEWORK_STORAGE),
            std::string("fwdir"));
  ASSERT_FALSE(cppmicroservices::any_cast<bool>(
    ctx.GetProperty(Constants::FRAMEWORK_LOG)));

  ASSERT_EQ(ctx.GetProperty(Constants::FRAMEWORK_WORKING_DIR),
            util::GetCurrentWorkingDirectory());
}

TEST(FrameworkTest, CustomConfiguration)
{
  FrameworkConfiguration configuration;
  configuration["org.osgi.framework.security"] = std::string("osgi");
  configuration["org.osgi.framework.startlevel.beginning"] = 0;
  configuration["org.osgi.framework.bsnversion"] = std::string("single");
  configuration["org.osgi.framework.custom1"] = std::string("foo");
  configuration["org.osgi.framework.custom2"] = std::string("bar");
  configuration[Constants::FRAMEWORK_LOG] = true;
  configuration[Constants::FRAMEWORK_STORAGE] = GetTempDirectory();
  configuration[Constants::FRAMEWORK_WORKING_DIR] = GetTempDirectory();

  // the threading model framework property is set at compile time and read-only at runtime. Test that this
  // is always the case.
#ifdef US_ENABLE_THREADING_SUPPORT
  configuration[Constants::FRAMEWORK_THREADING_SUPPORT] = std::string("single");
#else
  configuration[Constants::FRAMEWORK_THREADING_SUPPORT] = std::string("multi");
#endif

  auto f = FrameworkFactory().NewFramework(configuration);
  ASSERT_TRUE(f);

  ASSERT_NO_THROW(f.Start(););

  auto ctx = f.GetBundleContext();

  ASSERT_EQ("osgi", ctx.GetProperty("org.osgi.framework.security").ToString());
  ASSERT_EQ(
    0,
    any_cast<int>(ctx.GetProperty("org.osgi.framework.startlevel.beginning")));
  ASSERT_EQ(
    "single",
    any_cast<std::string>(ctx.GetProperty("org.osgi.framework.bsnversion")));
  ASSERT_EQ(
    "foo",
    any_cast<std::string>(ctx.GetProperty("org.osgi.framework.custom1")));
  ASSERT_EQ(
    "bar",
    any_cast<std::string>(ctx.GetProperty("org.osgi.framework.custom2")));
  ASSERT_EQ(any_cast<bool>(ctx.GetProperty(Constants::FRAMEWORK_LOG)), true);
  ASSERT_EQ(ctx.GetProperty(Constants::FRAMEWORK_STORAGE).ToString(),
            GetTempDirectory());
  ASSERT_EQ(ctx.GetProperty(Constants::FRAMEWORK_WORKING_DIR),
            GetTempDirectory());

#ifdef US_ENABLE_THREADING_SUPPORT
  ASSERT_EQ(ctx.GetProperty(Constants::FRAMEWORK_THREADING_SUPPORT).ToString(),
            "multi");
#else
  ASSERT_EQ(ctx.GetProperty(Constants::FRAMEWORK_THREADING_SUPPORT).ToString(),
            "single");
#endif
}

TEST(FrameworkTest, FrameworkStartsWhenFileNamedDataExistsInTempDir)
{
  class ScopedFile
  {
  public:
    ScopedFile(std::string directory, std::string filename)
      : filePath(std::move(directory) + util::DIR_SEP + std::move(filename))
    {
      CreateFile();
    }
    ~ScopedFile()
    {
      // Note: The file is created in the temp area anyway, so not being able
      // to remove the file is not catastrophic. We simply ignore it.
      std::remove(filePath.c_str());
    }

  private:
    void CreateFile()
    {
      std::fstream file(filePath, std::fstream::out);
      ASSERT_FALSE(file.fail())
        << "Failbit of the file stream should not be set.";
      file << "test";
      file.close();
    }
    const std::string filePath;
  };

  auto frameworkStorage = MakeUniqueTempDirectory();
  TempDir scopedDir(
    frameworkStorage); // delete "frameworkStorage" dir on destruction
  ScopedFile scopedFile(
    frameworkStorage,
    "data"); // create a file named "data" inside "scopedDir" and delete file on destruction
  FrameworkConfiguration frameworkConfig;
  frameworkConfig[Constants::FRAMEWORK_STORAGE] = frameworkStorage;

  auto framework = FrameworkFactory().NewFramework(frameworkConfig);
  ASSERT_TRUE(framework);
  ASSERT_NO_THROW(framework.Start(););
  framework.Stop();
  framework.WaitForStop(std::chrono::milliseconds::zero());
}

TEST(FrameworkTest, TempDataDirIsNotCreatedWhenFrameworkStarts)
{
  TempDir frameworkStorage = MakeUniqueTempDirectory();
  FrameworkConfiguration frameworkConfig;
  frameworkConfig[Constants::FRAMEWORK_STORAGE] =
    static_cast<std::string>(frameworkStorage);
  std::string persistentStoragePath =
    static_cast<std::string>(frameworkStorage) + util::DIR_SEP + "data";

  auto framework = FrameworkFactory().NewFramework(frameworkConfig);
  ASSERT_NO_THROW(framework.Start(););
  ASSERT_FALSE(util::Exists(persistentStoragePath))
    << "The framework should not create a directory named data in the "
       "temporary directory.";
  framework.Stop();
  framework.WaitForStop(std::chrono::milliseconds::zero());
}

TEST(FrameworkTest, DefaultLogSink)
{
  FrameworkConfiguration configuration;
  // turn on diagnostic logging
  configuration[Constants::FRAMEWORK_LOG] = true;

  // Needs improvement to guarantee that log messages are generated,
  // however for the moment do some operations using the framework
  // which would create diagnostic log messages.
  std::stringstream temp_buf;
  auto clog_buf = std::clog.rdbuf();
  std::clog.rdbuf(temp_buf.rdbuf());

  auto f = FrameworkFactory().NewFramework(configuration);
  ASSERT_TRUE(f);

  f.Start();
  cppmicroservices::testing::InstallLib(f.GetBundleContext(), "TestBundleA");

  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());

  ASSERT_FALSE(temp_buf.str().empty());

  std::clog.rdbuf(clog_buf);
}

TEST(FrameworkTest, CustomLogSink)
{
  FrameworkConfiguration configuration;
  // turn on diagnostic logging
  configuration[Constants::FRAMEWORK_LOG] = true;

  std::ostream custom_log_sink(std::cerr.rdbuf());

  auto f = FrameworkFactory().NewFramework(configuration, &custom_log_sink);
  ASSERT_TRUE(
    f); //Test Framework instantiation succeeded with custom diagnostic logger

  f.Start();
  f.Stop();
}

TEST(FrameworkTest, Properties)
{
  auto f = FrameworkFactory().NewFramework();
  f.Init();
  ASSERT_EQ(f.GetLocation(),
            "System Bundle"); // "Test Framework Bundle Location"
  ASSERT_EQ(
    f.GetSymbolicName(),
    Constants::SYSTEM_BUNDLE_SYMBOLICNAME); // "Test Framework Bundle Name"
  ASSERT_EQ(f.GetBundleId(), 0);            // "Test Framework Bundle Id"
}

TEST(Framework, LifeCycle)
{
  TestBundleListener listener;
  std::vector<BundleEvent> pEvts;

  auto f = FrameworkFactory().NewFramework();

  ASSERT_EQ(f.GetState(),
            Bundle::STATE_INSTALLED); // "Check framework is installed"

  // make sure WaitForStop returns immediately when the Framework's state is "Installed"
  auto fNoWaitEvent = f.WaitForStop(std::chrono::milliseconds::zero());
  ASSERT_TRUE(fNoWaitEvent); // "Check for valid framework event");
  ASSERT_EQ(fNoWaitEvent.GetType(),
            FrameworkEvent::Type::
              FRAMEWORK_ERROR); // "Check for correct framework event type"
  ASSERT_EQ(fNoWaitEvent.GetThrowable(),
            nullptr); // "Check that no exception was thrown"

  f.Init();

  ASSERT_EQ(f.GetState(),
            Bundle::STATE_STARTING); // "Check framework is starting"
  ASSERT_TRUE(f.GetBundleContext()); // "Check for a valid bundle context"

  f.Start();

  ASSERT_TRUE(listener.CheckListenerEvents(
    pEvts)); // "Check framework bundle event listener"

  ASSERT_EQ(f.GetState(), Bundle::STATE_ACTIVE); // "Check framework is active"

  f.GetBundleContext().AddBundleListener(std::bind(
    &TestBundleListener::BundleChanged, &listener, std::placeholders::_1));

#ifdef US_ENABLE_THREADING_SUPPORT
  // To test that the correct FrameworkEvent is returned, we must guarantee that the Framework
  // is in a STARTING, ACTIVE, or STOPPING state.
  std::thread waitForStopThread([&f]() {
    auto fStopEvent = f.WaitForStop(std::chrono::milliseconds::zero());
    ASSERT_FALSE(
      f.GetState() &
      Bundle::STATE_ACTIVE); // "Check framework is in the Stop state"
    ASSERT_TRUE(fStopEvent); // "Check for valid framework event"
    ASSERT_EQ(
      fStopEvent.GetType(),
      FrameworkEvent::Type::
        FRAMEWORK_STOPPED); // "Check for correct framework event type");
    ASSERT_EQ(fStopEvent.GetThrowable(),
              nullptr); // "Check that no exception was thrown");
  });

  f.Stop();
  waitForStopThread.join();
#else
  f.Stop();
#endif

  pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STOPPING, f));

  ASSERT_TRUE(listener.CheckListenerEvents(
    pEvts)); // "Check framework bundle event listener")

  ASSERT_THROW(
    f.Uninstall(),
    std::
      runtime_error); // Test that uninstalling a framework throws an exception

  // Test that all bundles in the Start state are stopped when the framework is stopped.
  f.Start();
#if defined(US_BUILD_SHARED_LIBS)
  auto bundle =
    cppmicroservices::testing::InstallLib(f.GetBundleContext(), "TestBundleA");
#else
  auto bundle =
    cppmicroservices::testing::GetBundle("TestBundleA", f.GetBundleContext());
#endif
  ASSERT_TRUE(bundle); // "Non-null bundle"
  bundle.Start();
  ASSERT_EQ(
    bundle.GetState(),
    Bundle::STATE_ACTIVE); // "Check that TestBundleA is in an Active state"

#ifdef US_ENABLE_THREADING_SUPPORT
  // To test that the correct FrameworkEvent is returned, we must guarantee that the Framework
  // is in a STARTING, ACTIVE, or STOPPING state.
  waitForStopThread = std::thread([&f]() {
    auto ev = f.WaitForStop(std::chrono::seconds(10));
    ASSERT_TRUE(
      ev); // "Test for valid framework event returned by Framework::WaitForStop()"
    if (!ev && ev.GetType() == FrameworkEvent::Type::FRAMEWORK_ERROR)
      std::rethrow_exception(ev.GetThrowable());
    ASSERT_EQ(ev.GetType(),
              FrameworkEvent::Type::
                FRAMEWORK_STOPPED); // "Check that framework event is stopped"
  });

  // Stopping the framework stops all active bundles.
  f.Stop();
  waitForStopThread.join();
#else
  f.Stop();
#endif
  ASSERT_NE(bundle.GetState(),
            Bundle::STATE_ACTIVE); // "Check that TestBundleA is not active"
  ASSERT_NE(f.GetState(),
            Bundle::STATE_ACTIVE); // "Check framework is not active"
}

#ifdef US_ENABLE_THREADING_SUPPORT

TEST(FrameworkTest, ConcurrentFrameworkStart)
{
  // test concurrent Framework starts.
  auto f = FrameworkFactory().NewFramework();
  f.Init();
  int start_count{ 0 };
  f.GetBundleContext().AddFrameworkListener(
    [&start_count](const FrameworkEvent& ev) {
      if (FrameworkEvent::Type::FRAMEWORK_STARTED == ev.GetType())
        ++start_count;
    });
  size_t num_threads{ 100 };
  std::vector<std::thread> threads;
  for (size_t i = 0; i < num_threads; ++i) {
    threads.push_back(std::thread{ [&f]() { f.Start(); } });
  }

  for (auto& t : threads)
    t.join();

  // To test that the correct FrameworkEvent is returned, we must guarantee that the Framework
  // is in a STARTING, ACTIVE, or STOPPING state.
  std::thread waitForStopThread([&f]() {
    auto fEvent = f.WaitForStop(std::chrono::milliseconds::zero());
    ASSERT_TRUE(
      fEvent); // "Check for a valid framework event returned from WaitForStop"
    ASSERT_EQ(fEvent.GetType(),
              FrameworkEvent::Type::
                FRAMEWORK_STOPPED); // "Check that framework event is stopped"
    ASSERT_EQ(fEvent.GetThrowable(),
              nullptr); // "Check that no exception was thrown"
  });

  f.Stop();
  waitForStopThread.join();

  // Its somewhat ambiguous in the OSGi spec whether or not multiple Framework STARTED events should be sent
  // when repeated calls to Framework::Start() are made on the same Framework instance once its in the
  // ACTIVE bundle state.
  // Felix and Knopflerfish OSGi implementations take two different stances.
  // Lock down the behavior that only one Framework STARTED event is sent.
  ASSERT_EQ(
    1,
    start_count); // "Multiple Framework::Start() calls only produce one Framework STARTED event."
}

TEST(FrameworkTest, ConcurrentFrameworkStop)
{
  // test concurrent Framework stops.
  auto f = FrameworkFactory().NewFramework();
  f.Start();

  // To test that the correct FrameworkEvent is returned, we must guarantee that the Framework
  // is in a STARTING, ACTIVE, or STOPPING state.
  std::thread waitForStopThread([&f]() {
    auto fEvent = f.WaitForStop(std::chrono::milliseconds::zero());
    ASSERT_TRUE(
      fEvent); // "Check for a valid framework event returned from WaitForStop"
    ASSERT_EQ(fEvent.GetType(),
              FrameworkEvent::Type::
                FRAMEWORK_STOPPED); // "Check that framework event is stopped"
    ASSERT_EQ(fEvent.GetThrowable(),
              nullptr); // "Check that no exception was thrown"
  });

  size_t num_threads{ 100 };
  std::vector<std::thread> threads;
  for (size_t i = 0; i < num_threads; ++i) {
    threads.push_back(std::thread{ [&f]() { f.Stop(); } });
  }

  for (auto& t : threads)
    t.join();
  waitForStopThread.join();
}

TEST(FrameworkTest, ConcurrentFrameworkWaitForStop)
{
  // test concurrent Framework stops.
  auto f = FrameworkFactory().NewFramework();
  f.Start();

  std::mutex m;
  size_t num_threads{ 100 };
  std::vector<std::thread> threads;
  for (size_t i = 0; i < num_threads; ++i) {
    // To test that the correct FrameworkEvent is returned, we must guarantee that the Framework
    // is in a STARTING, ACTIVE, or STOPPING state.
    threads.push_back(std::thread([&f, &m]() {
      auto fEvent = f.WaitForStop(std::chrono::milliseconds::zero());
      std::unique_lock<std::mutex> lock(m);
      ASSERT_TRUE(
        fEvent); // "Check for a valid framework event returned from WaitForStop"
      ASSERT_EQ(fEvent.GetType(),
                FrameworkEvent::Type::
                  FRAMEWORK_STOPPED); // "Check that framework event is stopped"
      ASSERT_EQ(fEvent.GetThrowable(),
                nullptr); // "Check that no exception was thrown"
    }));
  }

  f.Stop();
  for (auto& t : threads) {
    t.join();
  }
}

#endif

TEST(FrameworkTest, Events)
{
  TestBundleListener listener;
  std::vector<BundleEvent> pEvts;
  std::vector<BundleEvent> pStopEvts;

  auto f = FrameworkFactory().NewFramework();

  f.Start();

  auto fmc = f.GetBundleContext();
  fmc.AddBundleListener(std::bind(
    &TestBundleListener::BundleChanged, &listener, std::placeholders::_1));
#ifdef US_BUILD_SHARED_LIBS
  auto install = [&pEvts, &fmc](const std::string& libName) {
    auto bundle = cppmicroservices::testing::InstallLib(fmc, libName);
    pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_INSTALLED, bundle));
    if (bundle.GetSymbolicName() == "TestBundleB") {
      // This is an additional install event from the bundle
      // that is statically imported by TestBundleB.
      pEvts.push_back(BundleEvent(
        BundleEvent::BUNDLE_INSTALLED,
        cppmicroservices::testing::GetBundle("TestBundleImportedByB", fmc)));
    }
  };

  // The bundles used to test bundle events when stopping the framework.
  // For static builds, the order of the "install" calls is imported.

  install("TestBundleA");
  install("TestBundleA2");
  install("TestBundleB");
#  ifdef US_ENABLE_THREADING_SUPPORT
  install("TestBundleC1");
#  endif
  install("TestBundleH");
  install("TestBundleLQ");
  install("TestBundleM");
  install("TestBundleR");
  install("TestBundleRA");
  install("TestBundleRL");
  install("TestBundleS");
  install("TestBundleSL1");
  install("TestBundleSL3");
  install("TestBundleSL4");
  auto bundles(fmc.GetBundles());
#else
  // since all bundles are embedded in the main executable, all bundles are
  // installed at framework start. simply check for start and stop events
  std::vector<cppmicroservices::Bundle> bundles;
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleA", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleA2", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleB", fmc));
#  ifdef US_ENABLE_THREADING_SUPPORT
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleC1", fmc));
#  endif
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleH", fmc));
  bundles.push_back(
    cppmicroservices::testing::GetBundle("TestBundleImportedByB", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleLQ", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleM", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleR", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleRA", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleRL", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleS", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleSL1", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleSL3", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("TestBundleSL4", fmc));
  bundles.push_back(cppmicroservices::testing::GetBundle("main", fmc));
#endif

  for (auto& bundle : bundles) {
    bundle.Start();
    // no events will be fired for the framework, its already active at this point
    if (bundle != f) {
      pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_RESOLVED, bundle));
      pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STARTING, bundle));
      pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STARTED, bundle));

      // bundles will be stopped in the reverse order in which they were started.
      // It is easier to maintain this test if the stop events are setup in the
      // right order here, while the bundles are starting, instead of hard coding
      // the order of events somewhere else.
      // Doing it this way also tests the order in which starting and stopping
      // bundles occurs and when their events are fired.
      pStopEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STOPPED, bundle));
      pStopEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STOPPING, bundle));
    }
  }
  // Remember, the framework is stopped first, before all bundles are stopped.
  pStopEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STOPPING, f));
  std::reverse(pStopEvts.begin(), pStopEvts.end());

  ASSERT_TRUE(
    listener.CheckListenerEvents(pEvts)); // "Check for bundle start events"

  // Stopping the framework stops all active bundles.
  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());

  ASSERT_TRUE(
    listener.CheckListenerEvents(pStopEvts)); // "Check for bundle stop events"
}

TEST(FrameworkTest, IndirectFrameworkStop)
{
  auto f = FrameworkFactory().NewFramework();
  f.Start();
  auto bundle = f.GetBundleContext().GetBundle(0);
  bundle.Stop();
  Framework f2(bundle);
  f2.WaitForStop(std::chrono::milliseconds::zero());
  ASSERT_EQ(f.GetState(), Bundle::STATE_RESOLVED); // "Framework stopped"
}

TEST(FrameworkTest, ShutdownAndStart)
{
  int startCount = 0;

  FrameworkEvent fwEvt;
  {
    auto f = FrameworkFactory().NewFramework();
    f.Init();

    auto ctx = f.GetBundleContext();

    ctx.AddFrameworkListener([&startCount](const FrameworkEvent& fwEvt) {
      if (fwEvt.GetType() == FrameworkEvent::FRAMEWORK_STARTED) {
        ++startCount;
        auto bundle = fwEvt.GetBundle();
        ASSERT_EQ(bundle.GetBundleId(), 0); // "Got framework bundle"
        ASSERT_EQ(bundle.GetState(),
                  Bundle::STATE_ACTIVE); // "Started framework"

        // This stops the framework
        bundle.Stop();
      }
    });

    ASSERT_EQ(f.GetState(), Framework::STATE_STARTING); // "Starting framework"
    f.Start();

    // Wait for stop
    fwEvt = f.WaitForStop(std::chrono::milliseconds::zero());
  }

  ASSERT_EQ(fwEvt.GetType(),
            FrameworkEvent::FRAMEWORK_STOPPED); // "Stopped framework event"

  Bundle fwBundle = fwEvt.GetBundle();
  ASSERT_EQ(fwBundle.GetState(),
            Bundle::STATE_RESOLVED); // "Resolved framework"

  // Start the framework again
  fwBundle.Start();

  ASSERT_EQ(fwBundle.GetState(), Bundle::STATE_ACTIVE); // "Active framework"

  ASSERT_EQ(startCount, 1); // "One framework start notification"
}

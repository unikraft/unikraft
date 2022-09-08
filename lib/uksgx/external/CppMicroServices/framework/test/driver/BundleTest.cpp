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

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/BundleEvent.h"
#include "cppmicroservices/Constants.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/GetBundleContext.h"
#include "cppmicroservices/ListenerToken.h"
#include "cppmicroservices/ServiceEvent.h"

#include "cppmicroservices/util/FileSystem.h"
#include "cppmicroservices/util/String.h"

#include "TestDriverActivator.h"
#include "TestUtilBundleListener.h"
#include "TestUtilFrameworkListener.h"
#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

#include <thread>
#include <chrono>

using namespace cppmicroservices;

class FrameworkTestSuite
{

  TestBundleListener listener;

  BundleContext bc;
  Bundle bu;
  Bundle buExec;
  Bundle buA;

public:
  FrameworkTestSuite(const BundleContext& bc)
    : bc(bc)
    , bu(bc.GetBundle())
  {}

  void setup()
  {
    try {
      bc.AddBundleListener(&listener, &TestBundleListener::BundleChanged);
    } catch (const std::logic_error& ise) {
      US_TEST_OUTPUT(<< "bundle listener registration failed " << ise.what());
      throw;
    }

    try {
      bc.AddServiceListener(&listener, &TestBundleListener::ServiceChanged);
    } catch (const std::logic_error& ise) {
      US_TEST_OUTPUT(<< "service listener registration failed " << ise.what());
      throw;
    }
  }

  void cleanup()
  {
    std::vector<Bundle> bundles = { buA, buExec };

    for (auto& b : bundles) {
      try {
        if (b)
          b.Uninstall();
      } catch (...) { /* ignored */
      }
    }

    buExec = nullptr;
    buA = nullptr;

    try {
      bc.RemoveBundleListener(&listener, &TestBundleListener::BundleChanged);
    } catch (...) { /* ignored */
    }

    try {
      bc.RemoveServiceListener(&listener, &TestBundleListener::ServiceChanged);
    } catch (...) { /* ignored */
    }
  }

  //----------------------------------------------------------------------------
  //Test result of GetService(ServiceReference()). Should throw std::invalid_argument
  void frame018a()
  {
    try {
      bc.GetService(ServiceReferenceU());
      US_TEST_FAILED_MSG(
        << "Got service object, expected std::invalid_argument exception")
    } catch (const std::invalid_argument&) {
    } catch (...) {
      US_TEST_FAILED_MSG(
        << "Got wrong exception, expected std::invalid_argument")
    }
  }

  // Load libA and check that it exists and that its expected service does not exist,
  // Also check that the expected events in the framework occurs
  void frame020a()
  {
    buA = testing::InstallLib(bc, "TestBundleA");
    US_TEST_CONDITION_REQUIRED(buA, "Test for existing bundle TestBundleA")
    US_TEST_CONDITION(buA.GetSymbolicName() == "TestBundleA",
                      "Test bundle name")

    US_TEST_CONDITION_REQUIRED(buA.GetState() == Bundle::STATE_INSTALLED,
                               "Test bundle A in state installed")

    US_TEST_CONDITION(buA.GetLastModified() > Bundle::TimeStamp(),
                      "Test bundle A last modified")
    US_TEST_CONDITION(buA.GetLastModified() <= std::chrono::steady_clock::now(),
                      "Test bundle A last modified")

    // Check that no service reference exist yet.
    ServiceReferenceU sr1 =
      bc.GetServiceReference("cppmicroservices::TestBundleAService");
    US_TEST_CONDITION_REQUIRED(!sr1, "service from bundle A must not exist yet")

    // Check manifest headers
    auto headers = buA.GetHeaders();
    US_TEST_CONDITION_REQUIRED(headers.size() > 0,
                               "One or more manifest header")
    US_TEST_CONDITION(headers.at("bundle.symbolic_name") ==
                        std::string("TestBundleA"),
                      "BSN manifest header")

    // check the listeners for events
    std::vector<BundleEvent> pEvts;
#ifdef US_BUILD_SHARED_LIBS // The bundle is installed at framework startup for static builds
    pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_INSTALLED, buA));
    bool relaxed = false;
#else
    bool relaxed = true;
#endif
    US_TEST_CONDITION(listener.CheckListenerEvents(pEvts, relaxed),
                      "Test for unexpected events");
  }

  // Start libA and check that it gets state ACTIVE
  // and that the service it registers exist
  void frame025a()
  {
    buA.Start();
    US_TEST_CONDITION_REQUIRED(buA.GetState() == Bundle::STATE_ACTIVE,
                               "Test bundle A in state active")

    try {
      // Check if testbundleA registered the expected service
      ServiceReferenceU sr1 =
        bc.GetServiceReference("cppmicroservices::TestBundleAService");
      US_TEST_CONDITION_REQUIRED(sr1, "expecting service")

      auto o1 = bc.GetService(sr1);
      US_TEST_CONDITION_REQUIRED(o1 && !o1->empty(), "no service object found")

      // check the listeners for events
      std::vector<BundleEvent> pEvts;
      pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_RESOLVED, buA));
      pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STARTING, buA));
      pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STARTED, buA));

      std::vector<ServiceEvent> seEvts;
      seEvts.push_back(ServiceEvent(ServiceEvent::SERVICE_REGISTERED, sr1));

      US_TEST_CONDITION(listener.CheckListenerEvents(pEvts, seEvts),
                        "Test for unexpected events");
    } catch (const ServiceException& /*se*/) {
      US_TEST_FAILED_MSG(<< "test bundle, expected service not found");
    }
  }

  // Start libA and check that it exists and that the storage paths are correct
  void frame020b(const std::string& tempPath)
  {
    buA = testing::InstallLib(bc, "TestBundleA");
    US_TEST_CONDITION_REQUIRED(buA, "Test for existing bundle TestBundleA");

    buA.Start();

    US_TEST_CONDITION(bc.GetProperty(Constants::FRAMEWORK_STORAGE).ToString() ==
                        tempPath,
                      "Test for valid base storage path");

    // launching properties should be accessible through any bundle
    US_TEST_CONDITION(buA.GetBundleContext()
                          .GetProperty(Constants::FRAMEWORK_STORAGE)
                          .ToString() == tempPath,
                      "Test for valid base storage path");

    std::cout << "Framework Storage Base Directory: " << bc.GetDataFile("")
              << std::endl;
    const std::string baseStoragePath =
      tempPath + util::DIR_SEP + "data" + util::DIR_SEP +
      util::ToString(buA.GetBundleId()) + util::DIR_SEP;
    US_TEST_CONDITION(buA.GetBundleContext().GetDataFile("") == baseStoragePath,
                      "Test for valid data path");
    US_TEST_CONDITION(buA.GetBundleContext().GetDataFile("bla") ==
                        (baseStoragePath + "bla"),
                      "Test for valid data file path");

    US_TEST_CONDITION(buA.GetState() & Bundle::STATE_ACTIVE,
                      "Test if started correctly");
  }

  // Stop libA and check for correct events
  void frame030b()
  {
    ServiceReferenceU sr1 = buA.GetBundleContext().GetServiceReference(
      "cppmicroservices::TestBundleAService");
    US_TEST_CONDITION(sr1, "Test for valid service reference")

    try {
      auto lm = buA.GetLastModified();
      buA.Stop();
      US_TEST_CONDITION(!(buA.GetState() & Bundle::STATE_ACTIVE),
                        "Test for stopped state")
      US_TEST_CONDITION(lm == buA.GetLastModified(),
                        "Unchanged last modified time after stop")
    } catch (const std::exception& e) {
      US_TEST_FAILED_MSG(<< "Stop bundle exception: " << e.what())
    }

    // Check manifest headers in stopped state
    auto headers = buA.GetHeaders();
    US_TEST_CONDITION_REQUIRED(headers.size() > 0,
                               "One ore more manifest header")
    US_TEST_CONDITION(headers.at("bundle.symbolic_name") ==
                        std::string("TestBundleA"),
                      "BSN manifest header")

    std::vector<BundleEvent> pEvts;
    pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STOPPING, buA));
    pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STOPPED, buA));

    std::vector<ServiceEvent> seEvts;
    seEvts.push_back(ServiceEvent(ServiceEvent::SERVICE_UNREGISTERING, sr1));

    US_TEST_CONDITION(listener.CheckListenerEvents(pEvts, seEvts),
                      "Test for unexpected events");
  }

  // Check that the executable's activator was started and called
  void frame035a()
  {
    try {
      buExec = testing::GetBundle("main", bc);
      US_TEST_CONDITION_REQUIRED(buExec, "Bundle 'main' not found");
      buExec.Start();
    } catch (const std::exception& e) {
      US_TEST_FAILED_MSG(<< "Install bundle exception: " << e.what()
                         << " + in frame01:FAIL")
    }

    US_TEST_CONDITION_REQUIRED(TestDriverActivator::StartCalled(),
                               "BundleActivator::Start() called for executable")

    long systemId = 0;
    // check expected meta-data
    US_TEST_CONDITION("main" == buExec.GetSymbolicName(), "Test bundle name")
    US_TEST_CONDITION(BundleVersion(0, 1, 0) == buExec.GetVersion(),
                      "Test test driver bundle version")
    US_TEST_CONDITION(
      BundleVersion(CppMicroServices_VERSION_MAJOR,
                    CppMicroServices_VERSION_MINOR,
                    CppMicroServices_VERSION_PATCH) ==
        buExec.GetBundleContext().GetBundle(systemId).GetVersion(),
      "Test CppMicroServices version")
  }

  // Get location, persistent storage and status of the bundle.
  // Test bundle context properties
  void frame037a()
  {
    std::string location = buExec.GetLocation();
    std::cout << "LOCATION:" << location;
    US_TEST_CONDITION(!location.empty(), "Test for non-empty bundle location")

    US_TEST_CONDITION(buExec.GetState() & Bundle::STATE_ACTIVE,
                      "Test for started flag")

    // launching properties should be accessible through any bundle
    auto p1 =
      bc.GetBundle().GetBundleContext().GetProperty(Constants::FRAMEWORK_UUID);
    auto p2 = buExec.GetBundleContext().GetProperty(Constants::FRAMEWORK_UUID);
    US_TEST_CONDITION(!p1.Empty() && p1.ToString() == p2.ToString(),
                      "Test for uuid accesible from framework and bundle")

    std::cout << buExec.GetBundleContext().GetDataFile("") << std::endl;
    const std::string baseStoragePath = util::GetCurrentWorkingDirectory();

    US_TEST_CONDITION(buExec.GetBundleContext().GetDataFile("").substr(
                        0, baseStoragePath.size()) == baseStoragePath,
                      "Test for valid data path")
    US_TEST_CONDITION(buExec.GetBundleContext().GetDataFile("bla").substr(
                        0, baseStoragePath.size()) == baseStoragePath,
                      "Test for valid data file path")

    US_TEST_CONDITION(buExec.GetBundleContext()
                          .GetProperty(Constants::FRAMEWORK_UUID)
                          .Empty() == false,
                      "Test for non-empty framework uuid property")
    auto props = buExec.GetBundleContext().GetProperties();
    US_TEST_CONDITION_REQUIRED(props.empty() == false,
                               "Test for non-empty bundle props")
    US_TEST_CONDITION(props.count(Constants::FRAMEWORK_VERSION) == 1,
                      "Test for existing framework version prop")
  }

  struct LocalListener
  {
    void ServiceChanged(const ServiceEvent&) {}
  };

  // Add a service listener with a broken LDAP filter to Get an exception
  void frame045a()
  {
    LocalListener sListen1;
    std::string brokenFilter = "A broken LDAP filter";

    try {
      bc.AddServiceListener(
        &sListen1, &LocalListener::ServiceChanged, brokenFilter);
      US_TEST_FAILED_MSG(<< "test bundle, no exception on broken LDAP filter:");
    } catch (const std::invalid_argument& /*ia*/) {
      //assertEquals("InvalidSyntaxException.GetFilter should be same as input string", brokenFilter, ise.GetFilter());
    } catch (...) {
      US_TEST_FAILED_MSG(
        << "test bundle, wrong exception on broken LDAP filter:");
    }
  }
};

namespace {

// Verify that the same member function pointers registered as listeners
// with different receivers works.
void TestListenerFunctors()
{
  TestBundleListener listener1;
  TestBundleListener listener2;

  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Start();

  auto bc = framework.GetBundleContext();

  try {
    bc.RemoveBundleListener(&listener1, &TestBundleListener::BundleChanged);
    bc.AddBundleListener(&listener1, &TestBundleListener::BundleChanged);
    bc.RemoveBundleListener(&listener2, &TestBundleListener::BundleChanged);
    bc.AddBundleListener(&listener2, &TestBundleListener::BundleChanged);
  } catch (const std::logic_error& ise) {
    US_TEST_FAILED_MSG(<< "bundle listener registration failed " << ise.what()
                       << " : frameSL02a:FAIL");
  }

  auto bundleA = testing::InstallLib(bc, "TestBundleA");
  US_TEST_CONDITION_REQUIRED(bundleA, "Test for existing bundle TestBundleA")

  bundleA.Start();

  std::vector<BundleEvent> pEvts;
#ifdef US_BUILD_SHARED_LIBS
  pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_INSTALLED, bundleA));
#endif
  pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_RESOLVED, bundleA));
  pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STARTING, bundleA));
  pEvts.push_back(BundleEvent(BundleEvent::BUNDLE_STARTED, bundleA));

  std::vector<ServiceEvent> seEvts;

  bool relaxed = false;
#ifndef US_BUILD_SHARED_LIBS
  relaxed = true;
#endif
  US_TEST_CONDITION(listener1.CheckListenerEvents(pEvts, seEvts, relaxed),
                    "Check first bundle listener")
  US_TEST_CONDITION(listener2.CheckListenerEvents(pEvts, seEvts, relaxed),
                    "Check second bundle listener")

  bc.RemoveBundleListener(&listener1, &TestBundleListener::BundleChanged);
  bc.RemoveBundleListener(&listener2, &TestBundleListener::BundleChanged);

  bundleA.Stop();
}

void TestBundleStates()
{
  TestBundleListener listener;
  std::vector<BundleEvent> bundleEvents;
  FrameworkFactory factory;

  FrameworkConfiguration frameworkConfig;
  auto framework = factory.NewFramework(frameworkConfig);
  framework.Start();

  auto frameworkCtx = framework.GetBundleContext();
  frameworkCtx.AddBundleListener(&listener, &TestBundleListener::BundleChanged);

  // Test Start -> Stop for auto-installed bundles
  auto bundles = frameworkCtx.GetBundles();
  for (auto& bundle : bundles) {
    if (bundle != framework) {
      US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_INSTALLED,
                        "Test installed bundle state")
      try {
        bundle.Start();
        US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_ACTIVE,
                          "Test started bundle state")
        bundleEvents.push_back(
          BundleEvent(BundleEvent::BUNDLE_RESOLVED, bundle));
        bundleEvents.push_back(
          BundleEvent(BundleEvent::BUNDLE_STARTING, bundle));
        bundleEvents.push_back(
          BundleEvent(BundleEvent::BUNDLE_STARTED, bundle));
      } catch (const std::runtime_error& /*ex*/) {
        US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_RESOLVED,
                          "Test bundle state if bundle start failed")
        bundleEvents.push_back(
          BundleEvent(BundleEvent::BUNDLE_RESOLVED, bundle));
        bundleEvents.push_back(
          BundleEvent(BundleEvent::BUNDLE_STARTING, bundle));
        bundleEvents.push_back(
          BundleEvent(BundleEvent::BUNDLE_STOPPING, bundle));
        bundleEvents.push_back(
          BundleEvent(BundleEvent::BUNDLE_STOPPED, bundle));
      }
      // stop the bundle if it is in active state
      if (bundle.GetState() & Bundle::STATE_ACTIVE) {
        bundle.Stop();
        US_TEST_CONDITION((bundle.GetState() & Bundle::STATE_ACTIVE) == false,
                          "Test stopped bundle state")
        bundleEvents.push_back(
          BundleEvent(BundleEvent::BUNDLE_STOPPING, bundle));
        bundleEvents.push_back(
          BundleEvent(BundleEvent::BUNDLE_STOPPED, bundle));
      }
    }
  }
  US_TEST_CONDITION(listener.CheckListenerEvents(bundleEvents, false),
                    "Test for unexpected events");
  bundleEvents.clear();

#ifdef US_BUILD_SHARED_LIBS // following combination can be tested only for shared library builds
  Bundle bundle;
  // Test install -> uninstall
  // expect 2 event (BUNDLE_INSTALLED, BUNDLE_UNINSTALLED)
  bundle = testing::InstallLib(frameworkCtx, "TestBundleA");
  US_TEST_CONDITION(bundle, "Test non-empty bundle")
  US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_INSTALLED,
                    "Test installed bundle state")
  bundle.Uninstall();
  US_TEST_CONDITION(!frameworkCtx.GetBundle(bundle.GetBundleId()),
                    "Test bundle install -> uninstall")
  US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_UNINSTALLED,
                    "Test uninstalled bundle state")
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_INSTALLED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_UNRESOLVED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_UNINSTALLED, bundle));

  US_TEST_CONDITION(listener.CheckListenerEvents(bundleEvents, false),
                    "Test for unexpected events");
  bundleEvents.clear();

  // Test install -> start -> uninstall
  // expect 6 events (BUNDLE_INSTALLED, BUNDLE_STARTING, BUNDLE_STARTED, BUNDLE_STOPPING, BUNDLE_STOPPED, BUNDLE_UNINSTALLED)
  bundle = testing::InstallLib(frameworkCtx, "TestBundleA");
  US_TEST_CONDITION(bundle, "Test non-empty bundle")
  US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_INSTALLED,
                    "Test installed bundle state")
  bundle.Start();
  US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_ACTIVE,
                    "Test started bundle state")
  bundle.Uninstall();
  US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_UNINSTALLED,
                    "Test uninstalled bundle state")
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_INSTALLED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_RESOLVED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_STARTING, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_STARTED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_STOPPING, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_STOPPED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_UNRESOLVED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_UNINSTALLED, bundle));
  US_TEST_CONDITION(listener.CheckListenerEvents(bundleEvents),
                    "Test for unexpected events");
  bundleEvents.clear();

  // Test install -> stop -> uninstall
  // expect 2 event (BUNDLE_INSTALLED, BUNDLE_UNINSTALLED)
  bundle = testing::InstallLib(frameworkCtx, "TestBundleA");
  US_TEST_CONDITION(bundle, "Test non-empty bundle")
  US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_INSTALLED,
                    "Test installed bundle state")
  bundle.Stop();
  US_TEST_CONDITION((bundle.GetState() & Bundle::STATE_ACTIVE) == false,
                    "Test stopped bundle state")
  bundle.Uninstall();
  US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_UNINSTALLED,
                    "Test uninstalled bundle state")
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_INSTALLED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_UNRESOLVED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_UNINSTALLED, bundle));
  US_TEST_CONDITION(listener.CheckListenerEvents(bundleEvents),
                    "Test for unexpected events");
  bundleEvents.clear();

  // Test install -> start -> stop -> uninstall
  // expect 6 events (BUNDLE_INSTALLED, BUNDLE_STARTING, BUNDLE_STARTED, BUNDLE_STOPPING, BUNDLE_STOPPED, BUNDLE_UNINSTALLED)
  bundle = testing::InstallLib(frameworkCtx, "TestBundleA");
  auto lm = bundle.GetLastModified();
  US_TEST_CONDITION(bundle, "Test non-empty bundle")
  US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_INSTALLED,
                    "Test installed bundle state")
  bundle.Start();
  US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_ACTIVE,
                    "Test started bundle state")
  bundle.Stop();
  US_TEST_CONDITION((bundle.GetState() & Bundle::STATE_ACTIVE) == false,
                    "Test stopped bundle state")
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  bundle.Uninstall();
  US_TEST_CONDITION(bundle.GetState() & Bundle::STATE_UNINSTALLED,
                    "Test uninstalled bundle state")
  US_TEST_CONDITION(lm < bundle.GetLastModified(),
                    "Last modified time changed after uninstall")
  US_TEST_CONDITION(bundle.GetLastModified() <= std::chrono::steady_clock::now(),
                    "Last modified time <= now")
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_INSTALLED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_RESOLVED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_STARTING, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_STARTED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_STOPPING, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_STOPPED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_UNRESOLVED, bundle));
  bundleEvents.push_back(BundleEvent(BundleEvent::BUNDLE_UNINSTALLED, bundle));
  US_TEST_CONDITION(listener.CheckListenerEvents(bundleEvents),
                    "Test for unexpected events");
  bundleEvents.clear();
#endif
}

void TestForInstallFailure()
{
  FrameworkFactory factory;

  auto framework = factory.NewFramework();
  framework.Start();

  auto frameworkCtx = framework.GetBundleContext();

  // Test that bogus bundle installs throw the appropriate exception
  try {
    frameworkCtx.InstallBundles(std::string());
    US_TEST_FAILED_MSG(<< "Failed to throw a std::runtime_error")
  } catch (const std::runtime_error& ex) {
    US_TEST_OUTPUT(<< "Caught std::runtime_exception: " << ex.what())
  } catch (...) {
    US_TEST_FAILED_MSG(<< "Failed to throw a std::runtime_error")
  }

  try {
    frameworkCtx.InstallBundles(
      std::string("\\path\\which\\won't\\exist\\phantom_bundle"));
    US_TEST_FAILED_MSG(<< "Failed to throw a std::runtime_error")
  } catch (const std::runtime_error& ex) {
    US_TEST_OUTPUT(<< "Caught std::runtime_exception: " << ex.what())
  } catch (...) {
    US_TEST_FAILED_MSG(<< "Failed to throw a std::runtime_error")
  }
#ifdef US_BUILD_SHARED_LIBS
  // 2 bundles - the framework(system_bundle) and the executable(main).
  US_TEST_CONDITION(2 == frameworkCtx.GetBundles().size(),
                    "Test # of installed bundles")
#else
  // There are atleast 2 bundles, maybe more depending on how the executable is created
  US_TEST_CONDITION(2 <= frameworkCtx.GetBundles().size(),
                    "Test # of installed bundles")
#endif
}

void TestDuplicateInstall()
{
  FrameworkFactory factory;

  auto framework = factory.NewFramework();
  framework.Start();

  auto frameworkCtx = framework.GetBundleContext();

  // Test installing the same bundle (i.e. a bundle with the same location) twice.
  // The exact same bundle should be returned on the second install.
  auto bundle = testing::InstallLib(frameworkCtx, "TestBundleA");
  auto bundleDuplicate = testing::InstallLib(frameworkCtx, "TestBundleA");

  US_TEST_CONDITION(bundle == bundleDuplicate,
                    "Test for the same bundle instance");
  US_TEST_CONDITION(bundle.GetBundleId() == bundleDuplicate.GetBundleId(),
                    "Test for the same bundle id");
}

void TestAutoInstallEmbeddedBundles()
{
  FrameworkFactory factory;
  auto f = factory.NewFramework();
  f.Start();
  auto frameworkCtx = f.GetBundleContext();

  US_TEST_NO_EXCEPTION(frameworkCtx.InstallBundles(
    testing::BIN_PATH + util::DIR_SEP + "usFrameworkTestDriver" + US_EXE_EXT));

#ifdef US_BUILD_SHARED_LIBS
  // 2 bundles - the framework(system_bundle) and the executable(main).
  US_TEST_CONDITION(2 == frameworkCtx.GetBundles().size(),
                    "Test # of installed bundles")
#else
  // There are atleast 2 bundles, maybe more depending on how the executable is created
  US_TEST_CONDITION(2 <= frameworkCtx.GetBundles().size(),
                    "Test # of installed bundles")
#endif

  auto bundles = frameworkCtx.GetBundles();
  auto bundleIter =
    std::find_if(bundles.begin(), bundles.end(), [](const Bundle& b) {
      return (std::string("main") == b.GetSymbolicName());
    });

  US_TEST_CONDITION_REQUIRED(bundleIter != bundles.end(),
                             "Found a valid usFrameworkTestDriver bundle")
  US_TEST_NO_EXCEPTION((*bundleIter).Start());
  US_TEST_NO_EXCEPTION((*bundleIter).Uninstall());

  auto b = frameworkCtx.GetBundle(0);
  US_TEST_FOR_EXCEPTION(std::runtime_error, b.Uninstall());

  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());
}

void TestNonStandardBundleExtension()
{
  FrameworkFactory factory;
  auto f = factory.NewFramework();
  f.Start();
  auto frameworkCtx = f.GetBundleContext();

#ifdef US_BUILD_SHARED_LIBS
  US_TEST_NO_EXCEPTION(frameworkCtx.InstallBundles(
    testing::LIB_PATH + util::DIR_SEP + US_LIB_PREFIX + "TestBundleExt.cppms"));
  // 3 bundles - the framework(system_bundle), the executable(main) and TextBundleExt
  US_TEST_CONDITION(3 == frameworkCtx.GetBundles().size(),
                    "Test # of installed bundles")
#else
  // There are atleast 3 bundles, maybe more depending on how the executable is created
  US_TEST_CONDITION(3 <= frameworkCtx.GetBundles().size(),
                    "Test # of installed bundles")
#endif

  // Test the non-standard file extension bundle's lifecycle
  auto bundles = frameworkCtx.GetBundles();
  auto bundleIter =
    std::find_if(bundles.begin(), bundles.end(), [](const Bundle& b) {
      return (std::string("TestBundleExt") == b.GetSymbolicName());
    });

  US_TEST_CONDITION_REQUIRED(bundleIter != bundles.end(),
                             "Found a valid non-standard file extension bundle")
  US_TEST_NO_EXCEPTION((*bundleIter).Start());
  US_TEST_NO_EXCEPTION((*bundleIter).Uninstall());

  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());
}

void TestUnicodePaths()
{
  // 1. building static libraries (test bundle is included in the executable)
  // 2. using MINGW evironment (MinGW linker fails to link DLL with unicode path)
  // 3. using a compiler with no support for C++11 unicode string literals
#if !defined(US_BUILD_SHARED_LIBS) || defined(__MINGW32__) ||                  \
  !defined(US_CXX_UNICODE_LITERALS)
  US_TEST_OUTPUT(<< "Skipping test point for unicode path");
#else
  FrameworkFactory factory;
  auto f = factory.NewFramework();
  f.Start();
  auto frameworkCtx = f.GetBundleContext();
  try {
    std::string path_utf8 =
      testing::LIB_PATH + cppmicroservices::util::DIR_SEP +
      u8"くいりのまちとこしくそ" + cppmicroservices::util::DIR_SEP +
      US_LIB_PREFIX + "TestBundleU" + US_LIB_EXT;
    auto bundles = frameworkCtx.InstallBundles(path_utf8);
    US_TEST_CONDITION(bundles.size() == 1, "Install bundle from unicode path");
    auto bundle = bundles.at(0);
    US_TEST_CONDITION(
      bundle.GetLocation() == path_utf8,
      "Bundle location is the same as the path used to install");
    US_TEST_NO_EXCEPTION(bundle.Start());
    US_TEST_CONDITION(bundle.GetState() == Bundle::State::STATE_ACTIVE,
                      "Bundle check start state");
    US_TEST_NO_EXCEPTION(bundle.Stop());
  } catch (const std::exception& ex) {
    US_TEST_FAILED_MSG(<< ex.what());
  } catch (...) {
    US_TEST_FAILED_MSG(<< "TestUnicodePaths failed with unknown exception");
  }
#endif
}

// At the moment only the code path is being exercised to satisfy code coverage metrics.
// @todo Add more tests once Bundle start options are supported.
void TestBundleStartOptions()
{
  FrameworkFactory factory;
  auto f = factory.NewFramework();
  f.Start();
  auto frameworkCtx = f.GetBundleContext();

  auto bundle = testing::InstallLib(frameworkCtx, "TestBundleA");
  US_TEST_NO_EXCEPTION(bundle.Start(
    cppmicroservices::Bundle::StartOptions::START_ACTIVATION_POLICY));
}

void TestBundleLessThanOperator()
{
  FrameworkFactory factory;
  auto f = factory.NewFramework();
  f.Start();
  auto frameworkCtx = f.GetBundleContext();

  auto bundleA = testing::InstallLib(frameworkCtx, "TestBundleA");
  auto bundleB = testing::InstallLib(frameworkCtx, "TestBundleB");

  US_TEST_CONDITION_REQUIRED(bundleA < bundleB,
                             "Test that bundles are ordered correctly.");
  US_TEST_CONDITION_REQUIRED(bundleA < Bundle(),
                             "Test that bundles are ordered correctly.");
  US_TEST_CONDITION_REQUIRED(!(Bundle() < bundleB),
                             "Test that bundles are ordered correctly.");
  US_TEST_CONDITION_REQUIRED(!(Bundle() < Bundle()),
                             "Test that bundles are ordered correctly.");
}

void TestBundleAssignmentOperator()
{
  Bundle b;
  Bundle b1;
  b = b1;
  US_TEST_CONDITION_REQUIRED(b1 == b,
                             "Test that the bundle objects are equivalent.");

  FrameworkFactory factory;
  auto f = factory.NewFramework();
  f.Start();
  auto frameworkCtx = f.GetBundleContext();

  auto bundleA = testing::InstallLib(frameworkCtx, "TestBundleA");
  auto bundleB = testing::InstallLib(frameworkCtx, "TestBundleB");

  US_TEST_CONDITION_REQUIRED(
    bundleB.GetBundleId() != bundleA.GetBundleId(),
    "Test that the bundles are different before assignment.");
  US_TEST_CONDITION_REQUIRED(
    bundleB != bundleA,
    "Test that the bundles are different before assignment.");

  bundleB = bundleA;

  US_TEST_CONDITION_REQUIRED(bundleB.GetBundleId() == bundleA.GetBundleId(),
                             "Test bundle object assignment.");
  US_TEST_CONDITION_REQUIRED(bundleB == bundleA,
                             "Test that the bundle objects are equivalent.");
}

void TestBundleStreamOperator()
{
  FrameworkFactory factory;
  auto f = factory.NewFramework();
  f.Start();
  auto frameworkCtx = f.GetBundleContext();

  const auto bundle = testing::InstallLib(frameworkCtx, "TestBundleA");
  US_TEST_OUTPUT(<< &bundle);
}

// Tests for a deprecated API
// @todo Remove once this API is removed.
void TestBundleGetProperties()
{
  FrameworkFactory factory;
  auto f = factory.NewFramework();
  f.Start();
  auto frameworkCtx = f.GetBundleContext();

  auto bundle = testing::InstallLib(frameworkCtx, "TestBundleA");
  auto bundleProperties = bundle.GetProperties();

  US_TEST_CONDITION_REQUIRED(!bundleProperties.empty(),
                             "Test that bundle properties exist.");

  Any symbolicNameProperty(bundleProperties.at(Constants::BUNDLE_SYMBOLICNAME));
  US_TEST_CONDITION_REQUIRED(
    !symbolicNameProperty.Empty(),
    "Test that bundle properties contain an expected property.");
  US_TEST_CONDITION_REQUIRED(
    "TestBundleA" == symbolicNameProperty.ToStringNoExcept(),
    "Test that bundle properties contain an expected property.");
}

// Tests for a deprecated API
// @todo Remove once this API is removed.
void TestBundleGetProperty()
{
  FrameworkFactory factory;
  auto f = factory.NewFramework();
  f.Start();
  auto frameworkCtx = f.GetBundleContext();

  auto bundle = testing::InstallLib(frameworkCtx, "TestBundleA");

  // get a bundle property
  Any bundleProperty(bundle.GetProperty(Constants::BUNDLE_SYMBOLICNAME));
  US_TEST_CONDITION_REQUIRED("TestBundleA" == bundleProperty.ToStringNoExcept(),
                             "Test querying a bundle property succeeds.");

  // get a framework property
  Any frameworkProperty(
    bundle.GetProperty(Constants::FRAMEWORK_THREADING_SUPPORT));
  US_TEST_CONDITION_REQUIRED(
    !frameworkProperty.ToStringNoExcept().empty(),
    "Test that querying a framework property from a bundle object succeeds.");

  // get a non existent property
  Any emptyProperty(bundle.GetProperty("does.not.exist"));
  US_TEST_CONDITION_REQUIRED(emptyProperty.Empty(),
                             "Test that querying a non-existent property "
                             "returns an empty property object.");
}

// Tests for a deprecated API
// @todo Remove test once this API is removed.
void TestBundleGetPropertyKeys()
{
  FrameworkFactory factory;
  auto f = factory.NewFramework();
  f.Start();
  auto frameworkCtx = f.GetBundleContext();

  auto bundle = testing::InstallLib(frameworkCtx, "TestBundleA");
  auto keys = bundle.GetPropertyKeys();

  US_TEST_CONDITION_REQUIRED(!keys.empty(),
                             "Test that bundle property keys exist.");
  US_TEST_CONDITION_REQUIRED(
    keys.end() !=
      std::find(keys.begin(), keys.end(), Constants::BUNDLE_SYMBOLICNAME),
    "Test that property keys contain the expected keys.");
  US_TEST_CONDITION_REQUIRED(
    keys.end() !=
      std::find(keys.begin(), keys.end(), Constants::BUNDLE_ACTIVATOR),
    "Test that property keys contain the expected keys.");
}

#if defined(US_BUILD_SHARED_LIBS)
// Test installing a bundle with invalid meta-data in its manifest.
void TestBundleManifestFailures()
{
  auto f = FrameworkFactory().NewFramework();
  f.Start();
  auto bc = f.GetBundleContext();

  std::vector<std::string> validBundleNames({ f.GetSymbolicName(), "main" });
  std::sort(validBundleNames.begin(), validBundleNames.end());

  auto validateBundleNames = [&bc](std::vector<std::string>& original) {
    auto installed = bc.GetBundles();
    std::vector<std::string> names;
    for (auto& b : installed) {
      names.push_back(b.GetSymbolicName());
    }
    std::sort(names.begin(), names.end());
    return std::includes(
      original.begin(), original.end(), names.begin(), names.end());
  };

  // throw if manifest.json bundle version key is not a string type
  US_TEST_FOR_EXCEPTION(
    std::runtime_error,
    testing::InstallLib(bc, "TestBundleWithInvalidVersionType"));
  US_TEST_CONDITION(2 == bc.GetBundles().size(),
                    "Test that an invalid bundle.version type results in not "
                    "installing the bundle.");
  US_TEST_CONDITION(true == validateBundleNames(validBundleNames),
                    "Test that an invalid bundle.version type results in not "
                    "installing the bundle.");

  // throw if BundleVersion ctor throws in BundlePrivate ctor
  US_TEST_FOR_EXCEPTION(
    std::runtime_error,
    testing::InstallLib(bc, "TestBundleWithInvalidVersion"));
  US_TEST_CONDITION(2 == bc.GetBundles().size(),
                    "Test that an invalid bundle.version results in not "
                    "installing the bundle.");
  US_TEST_CONDITION(true == validateBundleNames(validBundleNames),
                    "Test that an invalid bundle.version results in not "
                    "installing the bundle.");

  // throw if missing bundle.symbolic_name key in manifest.json
  US_TEST_FOR_EXCEPTION(std::runtime_error,
                        testing::InstallLib(bc, "TestBundleWithoutBundleName"));
  US_TEST_CONDITION(2 == bc.GetBundles().size(),
                    "Test that a missing bundle.symbolic_name results in not "
                    "installing the bundle.");
  US_TEST_CONDITION(true == validateBundleNames(validBundleNames),
                    "Test that a missing bundle.symbolic_name results in not "
                    "installing the bundle.");

  // throw if empty bundle.symbolic_name value in manifest.json
  US_TEST_FOR_EXCEPTION(
    std::runtime_error,
    testing::InstallLib(bc, "TestBundleWithEmptyBundleName"));
  US_TEST_CONDITION(2 == bc.GetBundles().size(),
                    "Test that an empty bundle.symbolic_name results in not "
                    "installing the bundle.");
  US_TEST_CONDITION(true == validateBundleNames(validBundleNames),
                    "Test that an empty bundle.symbolic_name results in not "
                    "installing the bundle.");

  f.Stop();
  f.WaitForStop(std::chrono::seconds::zero());
}
#endif

// Test the behavior of illegal bundle state changes.
void TestIllegalBundleStateChange()
{
  US_TEST_OUTPUT(<< "=== Testing Illegal Bundle State Changes ===");
  auto f = FrameworkFactory().NewFramework();
  US_TEST_NO_EXCEPTION(f.Start());
  auto bc = f.GetBundleContext();

  auto bundleA = testing::InstallLib(bc, "TestBundleA");
  US_TEST_NO_EXCEPTION(bundleA.Start());
  US_TEST_NO_EXCEPTION(bundleA.Uninstall());
  // Test stopping an uninstalled bundle
  US_TEST_FOR_EXCEPTION(std::logic_error, bundleA.Stop());

  f.Stop();
  f.WaitForStop(std::chrono::seconds::zero());

  f = FrameworkFactory().NewFramework();
  US_TEST_NO_EXCEPTION(f.Start());
  bc = f.GetBundleContext();

  bundleA = testing::InstallLib(bc, "TestBundleA");
  US_TEST_NO_EXCEPTION(bundleA.Start());
  US_TEST_NO_EXCEPTION(bundleA.Uninstall());
  // Test starting an uninstalled bundle
  US_TEST_FOR_EXCEPTION(std::logic_error, bundleA.Start());

  f.Stop();
  f.WaitForStop(std::chrono::seconds::zero());

  f = FrameworkFactory().NewFramework();
  US_TEST_NO_EXCEPTION(f.Start());
  bc = f.GetBundleContext();

  bundleA = testing::InstallLib(bc, "TestBundleA");
  US_TEST_NO_EXCEPTION(bundleA.Start());
  US_TEST_NO_EXCEPTION(bundleA.Uninstall());
  // Test uninstalling an already uninstalled bundle should throw
  US_TEST_FOR_EXCEPTION(std::logic_error, bundleA.Uninstall());

  // Test that BundlePrivate::CheckUninstalled throws on bundle operations performed after its been uninstalled.
  US_TEST_FOR_EXCEPTION(std::logic_error, bundleA.GetRegisteredServices());

  // That that installing a bundle with the same symbolic name and version throws an exception.
#if defined(US_BUILD_SHARED_LIBS)
  US_TEST_NO_EXCEPTION(testing::InstallLib(bc, "TestBundleA"));
  US_TEST_FOR_EXCEPTION(std::runtime_error,
                        testing::InstallLib(bc, "TestBundleADuplicate"));
#endif

  f.Stop();
  f.WaitForStop(std::chrono::seconds::zero());
}

// test failure on waiting for bundle operations.
void TestWaitOnBundleOperation()
{
  US_TEST_OUTPUT(<< "=== Testing Wait on Bundle Operation Failures ===");

  ///
  /// Fail to wait on each bundle operation from the bundle activator start method.
  ///
  auto f1 = FrameworkFactory().NewFramework(FrameworkConfiguration{
    { std::string(
        "org.cppmicroservices.framework.testing.waitonoperation.start"),
      Any{ std::string("start") } } });
  US_TEST_NO_EXCEPTION(f1.Start());
  auto bundleWaitOnOperation =
    testing::InstallLib(f1.GetBundleContext(), "TestBundleWaitOnOperation");
  US_TEST_FOR_EXCEPTION(std::runtime_error, bundleWaitOnOperation.Start());
  US_TEST_CONDITION(Bundle::State::STATE_RESOLVED ==
                      bundleWaitOnOperation.GetState(),
                    "Test the bundle state after a failed bundle start.");

  f1.Stop();
  f1.WaitForStop(std::chrono::seconds::zero());

  auto f2 = FrameworkFactory().NewFramework(FrameworkConfiguration{
    { std::string(
        "org.cppmicroservices.framework.testing.waitonoperation.start"),
      Any{ std::string("stop") } } });
  US_TEST_NO_EXCEPTION(f2.Start());
  bundleWaitOnOperation =
    testing::InstallLib(f2.GetBundleContext(), "TestBundleWaitOnOperation");
  US_TEST_FOR_EXCEPTION(std::runtime_error, bundleWaitOnOperation.Start());
  US_TEST_CONDITION(Bundle::State::STATE_RESOLVED ==
                      bundleWaitOnOperation.GetState(),
                    "Test the bundle state after a failed bundle start.");

  f2.Stop();
  f2.WaitForStop(std::chrono::seconds::zero());

  auto f3 = FrameworkFactory().NewFramework(FrameworkConfiguration{
    { std::string(
        "org.cppmicroservices.framework.testing.waitonoperation.start"),
      Any{ std::string("uninstall") } } });
  US_TEST_NO_EXCEPTION(f3.Start());
  bundleWaitOnOperation =
    testing::InstallLib(f3.GetBundleContext(), "TestBundleWaitOnOperation");
  // This call sometimes throws and sometimes doesn't. There may be a race lurking somewhere...
  try {
    bundleWaitOnOperation.Start();
  } catch (...) {
  }
  US_TEST_CONDITION(Bundle::State::STATE_UNINSTALLED ==
                      bundleWaitOnOperation.GetState(),
                    "Test the bundle state after a failed bundle start.");

  f3.Stop();
  f3.WaitForStop(std::chrono::seconds::zero());

  ///
  /// Fail to wait on each bundle operation from the bundle activator stop method.
  ///

  auto f4 = FrameworkFactory().NewFramework(FrameworkConfiguration{
    { std::string(
        "org.cppmicroservices.framework.testing.waitonoperation.stop"),
      Any{ std::string("start") } } });
  US_TEST_NO_EXCEPTION(f4.Start());
  bundleWaitOnOperation =
    testing::InstallLib(f4.GetBundleContext(), "TestBundleWaitOnOperation");
  US_TEST_NO_EXCEPTION(bundleWaitOnOperation.Start());
  US_TEST_FOR_EXCEPTION(std::runtime_error, bundleWaitOnOperation.Stop());
  US_TEST_CONDITION(Bundle::State::STATE_RESOLVED ==
                      bundleWaitOnOperation.GetState(),
                    "Test the bundle state after a failed bundle stop.");

  f4.Stop();
  f4.WaitForStop(std::chrono::seconds::zero());

  auto f5 = FrameworkFactory().NewFramework(FrameworkConfiguration{
    { std::string(
        "org.cppmicroservices.framework.testing.waitonoperation.stop"),
      Any{ std::string("stop") } } });
  US_TEST_NO_EXCEPTION(f5.Start());
  bundleWaitOnOperation =
    testing::InstallLib(f5.GetBundleContext(), "TestBundleWaitOnOperation");
  US_TEST_NO_EXCEPTION(bundleWaitOnOperation.Start());
  US_TEST_FOR_EXCEPTION(std::runtime_error, bundleWaitOnOperation.Stop());
  US_TEST_CONDITION(Bundle::State::STATE_RESOLVED ==
                      bundleWaitOnOperation.GetState(),
                    "Test the bundle state after a failed bundle stop.");

  f5.Stop();
  f5.WaitForStop(std::chrono::seconds::zero());

  auto f6 = FrameworkFactory().NewFramework(FrameworkConfiguration{
    { std::string(
        "org.cppmicroservices.framework.testing.waitonoperation.stop"),
      Any{ std::string("uninstall") } } });
  US_TEST_NO_EXCEPTION(f6.Start());
  bundleWaitOnOperation =
    testing::InstallLib(f6.GetBundleContext(), "TestBundleWaitOnOperation");
  US_TEST_NO_EXCEPTION(bundleWaitOnOperation.Start());
  US_TEST_FOR_EXCEPTION(std::runtime_error, bundleWaitOnOperation.Stop());
  US_TEST_CONDITION(Bundle::State::STATE_UNINSTALLED ==
                      bundleWaitOnOperation.GetState(),
                    "Test the bundle state after a failed bundle stop.");

  f6.Stop();
  f6.WaitForStop(std::chrono::seconds::zero());
}

#if defined(US_BUILD_SHARED_LIBS)
// Test the behavior of a bundle activator which throws an exception.
void TestBundleActivatorFailures()
{
  US_TEST_OUTPUT(<< "=== Testing Bundle State Change Failures ===");

  auto f = FrameworkFactory().NewFramework();
  US_TEST_NO_EXCEPTION(f.Start());
  auto bc = f.GetBundleContext();

  TestFrameworkListener listener;
  auto token = bc.AddFrameworkListener(std::bind(
    &TestFrameworkListener::frameworkEvent, &listener, std::placeholders::_1));

  auto bundleStopFail = testing::InstallLib(bc, "TestBundleStopFail");
  US_TEST_NO_EXCEPTION(bundleStopFail.Start());
  // Test the state of a bundle after stop failed
  US_TEST_FOR_EXCEPTION(std::runtime_error, bundleStopFail.Stop());
  US_TEST_CONDITION(Bundle::State::STATE_RESOLVED == bundleStopFail.GetState(),
                    "Test that the bundle is not active after a failed stop.");

  US_TEST_NO_EXCEPTION(bundleStopFail.Start());
  US_TEST_CONDITION(Bundle::State::STATE_ACTIVE == bundleStopFail.GetState(),
                    "Test that the bundle is active prior to uninstall.");
  // Test that bundle stop throws an exception and is sent as a FrameworkEvent during uninstall
  US_TEST_NO_EXCEPTION(bundleStopFail.Uninstall());
  US_TEST_CONDITION(1 == listener.events_received(),
                    "Test that one FrameworkEvent was received.");
  US_TEST_CONDITION(
    listener.CheckEvents(std::vector<FrameworkEvent>{ FrameworkEvent{
      FrameworkEvent::Type::FRAMEWORK_WARNING,
      bundleStopFail,
      std::string(),
      std::make_exception_ptr(std::runtime_error("whoopsie!")) } }),
    "Test that the correct FrameworkEvent was received.");
  bc.RemoveListener(std::move(token));
  US_TEST_CONDITION(
    Bundle::State::STATE_UNINSTALLED == bundleStopFail.GetState(),
    "Test that even if Stop throws, the bundle is uninstalled.");

  auto bundleStartFail = testing::InstallLib(bc, "TestBundleStartFail");
  // Test the state of a bundle after start failed
  US_TEST_FOR_EXCEPTION(std::runtime_error, bundleStartFail.Start());
  US_TEST_CONDITION(Bundle::State::STATE_RESOLVED == bundleStartFail.GetState(),
                    "Test that the bundle is not active after a failed start.");
}
#endif

} // end anonymous namespace

int BundleTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("BundleTest");

  {
    auto framework = FrameworkFactory().NewFramework();
    framework.Start();

    auto bundles = framework.GetBundleContext().GetBundles();
    for (auto const& bundle : bundles) {
      std::cout << "----- " << bundle.GetSymbolicName() << std::endl;
    }

    FrameworkTestSuite ts(framework.GetBundleContext());

    ts.setup();

    ts.frame018a();

    ts.frame020a();
    ts.frame025a();
    ts.frame030b();

    ts.frame035a();
    ts.frame037a();
    ts.frame045a();

    ts.cleanup();
  }

  // test a non-default framework instance using a different persistent storage location.
  {
    testing::TempDir frameworkStorage = testing::MakeUniqueTempDirectory();
    FrameworkConfiguration frameworkConfig;
    frameworkConfig[Constants::FRAMEWORK_STORAGE] =
      static_cast<std::string>(frameworkStorage);
    auto framework = FrameworkFactory().NewFramework(frameworkConfig);
    framework.Start();

    FrameworkTestSuite ts(framework.GetBundleContext());
    ts.setup();
    ts.frame020b(frameworkStorage);
    ts.cleanup();
  }

  TestListenerFunctors();
  TestBundleStates();
  TestForInstallFailure();
  TestDuplicateInstall();
  TestAutoInstallEmbeddedBundles();
  TestNonStandardBundleExtension();
  TestUnicodePaths();
  TestBundleStartOptions();
  TestBundleLessThanOperator();
  TestBundleAssignmentOperator();
  TestBundleStreamOperator();
  TestBundleGetProperties();
  TestBundleGetPropertyKeys();
  TestBundleGetProperty();

  // will not test:
  // Fragments - unable to test without defining what a Fragment is and implementing it
  //    Fragment code path in BundlePrivate::GetUpdatedState
  // lazy activation code path - requires defining lazy activation for C++ and implementing it.
  // BundlePrivate::GetAutoStartSetting() - no callers, internal or external
  // invalid json use case - this is automatically checked by usResourceCompiler.
#if defined(US_BUILD_SHARED_LIBS)
  TestBundleManifestFailures();
  TestBundleActivatorFailures();
#endif

  TestIllegalBundleStateChange();
  TestWaitOnBundleOperation();

  US_TEST_END()
}

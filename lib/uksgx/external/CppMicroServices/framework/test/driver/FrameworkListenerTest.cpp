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
#include "cppmicroservices/Constants.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/FrameworkFactory.h"

#include "TestUtilFrameworkListener.h"
#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

#include <thread>
#include <vector>

using namespace cppmicroservices;

namespace {

void testStartStopFrameworkEvents()
{
  auto f = FrameworkFactory().NewFramework();

  TestFrameworkListener l;
  f.Init();
  f.GetBundleContext().AddFrameworkListener(
    &l, &TestFrameworkListener::frameworkEvent);
  f.Start();
  f.Stop();

  std::vector<FrameworkEvent> events;
  events.push_back(FrameworkEvent(
    FrameworkEvent::Type::FRAMEWORK_STARTED, f, "Framework Started"));
  US_TEST_CONDITION_REQUIRED(
    l.CheckEvents(events),
    "Test for the correct number and order of Framework start/stop events.");
}

void testAddRemoveFrameworkListener()
{
  auto f = FrameworkFactory().NewFramework();
  f.Init();
  BundleContext fCtx{ f.GetBundleContext() };

  // Test that the lambda is removed correctly if the lambda is referenced by a variable
  int count{ 0 };
  auto listener = [&count](const FrameworkEvent&) { ++count; };
  fCtx.AddFrameworkListener(listener);
  fCtx.RemoveFrameworkListener(listener);

  // test listener removal...
  TestFrameworkListener l;
  fCtx.AddFrameworkListener(&l, &TestFrameworkListener::frameworkEvent);
  fCtx.RemoveFrameworkListener(&l, &TestFrameworkListener::frameworkEvent);

  f.Start(); // generate framework event
  US_TEST_CONDITION(l.CheckEvents(std::vector<FrameworkEvent>()),
                    "Test listener removal");
  US_TEST_CONDITION(count == 0, "Test listener removal");
  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());

  count = 0;
  f.Init();
  fCtx = f.GetBundleContext();
  auto fl = [&count](const FrameworkEvent&) { ++count; };
  fCtx.AddFrameworkListener(fl);

  f.Start();
  US_TEST_CONDITION(count == 1, "Test listener addition");

  fCtx.RemoveFrameworkListener(fl);
  // note: The Framework STARTED event is only sent once. Stop and Start the framework to generate another one.
  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());

  f.Init();
  fCtx = f.GetBundleContext();
  fCtx.AddFrameworkListener(&l, &TestFrameworkListener::frameworkEvent);
  f.Start();
  US_TEST_CONDITION(
    l.CheckEvents(std::vector<FrameworkEvent>{ FrameworkEvent(
      FrameworkEvent::Type::FRAMEWORK_STARTED, f, "Framework Started") }),
    "Test listener addition");
  US_TEST_CONDITION(count == 1, "Test listener was successfully removed");
  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());

  int count1(0);
  int count2(0);
  auto listener_callback_counter1 = [&count1](const FrameworkEvent&) {
    ++count1;
    std::cout << "listener_callback_counter1: call count " << count1
              << std::endl;
  };
  auto listener_callback_counter2 = [&count2](const FrameworkEvent&) {
    ++count2;
    std::cout << "listener_callback_counter2: call count " << count2
              << std::endl;
  };
  auto listener_callback_throw = [](const FrameworkEvent&) {
    throw std::runtime_error("boo");
  };

  f.Init();
  fCtx = f.GetBundleContext();
  auto t1 = fCtx.AddFrameworkListener(listener_callback_counter1);
  auto t2 = fCtx.AddFrameworkListener(listener_callback_counter2);
  auto t3 = fCtx.AddFrameworkListener(listener_callback_throw);

  f.Start(); // generate framework event (started)
  US_TEST_CONDITION(count1 == 1,
                    "Test that multiple framework listeners were called");
  US_TEST_CONDITION(count2 == 1,
                    "Test that multiple framework listeners were called");

  fCtx.RemoveListener(std::move(t1));
  fCtx.RemoveListener(std::move(t2));
  fCtx.RemoveListener(std::move(t3));

  f.Start(); // generate framework event (started)
  US_TEST_CONDITION(
    count1 == 1,
    "Test that multiple framework listeners were NOT called after removal");
  US_TEST_CONDITION(
    count2 == 1,
    "Test that multiple framework listeners were NOT called after removal");
}

void testFrameworkListenersAfterFrameworkStop()
{
  auto f = FrameworkFactory().NewFramework();
  f.Init();
  // OSGi section 10.2.2.13 (Framework::stop API):
  //    4. Event handling is disabled.
  //    6. All resources held by this Framework are released.
  // The assumption is that framework listeners are one such resource described in step #6.
  int events(0);
  auto listener = [&events](const FrameworkEvent& evt) {
    ++events;
    std::cout << evt << std::endl;
  };
  f.GetBundleContext().AddFrameworkListener(listener);
  f.Start(); // generate framework event (started)
  f.Stop();  // resources (such as framework listeners) are released
  // due to the asynchronous nature of Stop(), we must wait for the stop to complete
  // before starting the framework again. If this doesn't happen, the start may send
  // a framework event before the listener is disabled and cleaned up.
  f.WaitForStop(std::chrono::milliseconds::zero());
  f.Start(); // generate framework event (started) with no listener to see it
  US_TEST_CONDITION(events == 1,
                    "Test that listeners were released on Framework Stop");
}

void testFrameworkListenerThrowingInvariant()
{
  /*
    The Framework must publish a FrameworkEvent.ERROR if a callback to an event listener generates an exception - except
    when the callback happens while delivering a FrameworkEvent.ERROR (to prevent an infinite loop).

    Tests:
    1. Given a bundle listener which throws -> verfiy a FrameworkEvent ERROR is received with the correct event info.
    2. Given a service listener which throws -> verfiy a FrameworkEvent ERROR is received with the correct event info.
    3. Given a framework listener which throws -> No FrameworkEvent is received, instead an internal log message is sent.
  */
  std::stringstream logstream;
  std::ostream sink(logstream.rdbuf());
  // Use a redirected logger to verify that the framework listener logged an
  // error message when it encountered a FrameworkEvent::ERROR coming from
  // a framework listener.
  auto f = FrameworkFactory().NewFramework(
    std::map<std::string, cppmicroservices::Any>{
      { Constants::FRAMEWORK_LOG, true } },
    &sink);
  f.Init();

  bool fwk_error_received(false);
  std::string exception_string("bad callback");
  auto listener = [&fwk_error_received,
                   &exception_string](const FrameworkEvent& evt) {
    try {
      if (evt.GetThrowable())
        std::rethrow_exception(evt.GetThrowable());
    } catch (const std::exception& e) {
      if (FrameworkEvent::Type::FRAMEWORK_ERROR == evt.GetType() &&
          e.what() == exception_string &&
          std::string(typeid(e).name()) == typeid(std::runtime_error).name()) {
        fwk_error_received = true;
      }
    }
  };

  f.GetBundleContext().AddFrameworkListener(listener);
  // @todo A STARTING BundleEvent should be sent before the Framework runs its Activator (in Start()). Apache Felix does it this way.
  f.Start();

  // Test #1 - test bundle event listener
  auto bl = [](const BundleEvent&) {
    throw std::runtime_error("bad callback");
  };
  f.GetBundleContext().AddBundleListener(bl);
  auto bundleA2 = testing::InstallLib(
    f.GetBundleContext(),
    "TestBundleA2"); // generate a bundle event for shared libs
#ifndef US_BUILD_SHARED_LIBS
  US_TEST_CONDITION(bundleA2, "TestBundleA2 bundle not found");
  bundleA2
    .Start(); // since bundles are auto-installed, start the bundle to generate a bundle event
#endif
  US_TEST_CONDITION(fwk_error_received,
                    "Test that a Framework ERROR event was received from a "
                    "throwing bundle listener");
  f.GetBundleContext().RemoveBundleListener(bl);

  // Test #2 - test service event listener
  fwk_error_received = false;
  exception_string = "you sunk my battleship";
  auto sl = [](const ServiceEvent&) {
    throw std::runtime_error("you sunk my battleship");
  };
  f.GetBundleContext().AddServiceListener(sl);
  auto bundleA = testing::InstallLib(f.GetBundleContext(), "TestBundleA");
  bundleA.Start(); // generate a service event
  US_TEST_CONDITION(fwk_error_received,
                    "Test that a Framework ERROR event was received from a "
                    "throwing service listener");
  f.GetBundleContext().RemoveServiceListener(sl);

  // note: The Framework STARTED event is only sent once. Stop and Start the framework to generate another one.
  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());

  // Test #3 - test framework event listener
  f.Init();
  fwk_error_received = false;
  exception_string = "whoopsie!";
  TestFrameworkListener l;
  f.GetBundleContext().RemoveFrameworkListener(
    listener); // remove listener until issue #95 is fixed.
  f.GetBundleContext().AddFrameworkListener(
    &l, &TestFrameworkListener::throwOnFrameworkEvent);
  // This will cause a deadlock if this test fails.
  f.Start(); // generates a framework event
  US_TEST_CONDITION(false == fwk_error_received,
                    "Test that a Framework ERROR event was NOT received from a "
                    "throwing framework listener");
  US_TEST_CONDITION(
    std::string::npos !=
      logstream.str().find("A Framework Listener threw an exception:"),
    "Test for internal log message from Framework event handler");
}

#ifdef US_ENABLE_THREADING_SUPPORT
void testDeadLock()
{
  // test for deadlocks during Framework API re-entry from a Framework Listener callback
  auto f = FrameworkFactory().NewFramework();
  f.Start();

  auto listener = [&f](const FrameworkEvent& evt) {
    if (FrameworkEvent::Type::FRAMEWORK_ERROR == evt.GetType()) {
      // generate a framework event on another thread,
      // which will cause a deadlock if any mutexes are locked.
      // Doing this on the same thread would produce
      // undefined behavior (typically a deadlock or an exception)
      std::thread t([&f]() {
        try {
          f.Start();
        } catch (...) {
        }
      });
      t.join();
    }
  };

  f.GetBundleContext().AddBundleListener(
    [](const BundleEvent&) { throw std::runtime_error("bad bundle"); });
  f.GetBundleContext().AddFrameworkListener(listener);
  auto bundleA = testing::InstallLib(
    f.GetBundleContext(),
    "TestBundleA"); // trigger the bundle listener to be called

  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());
}
#endif

} // end anonymous namespace

int FrameworkListenerTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("FrameworkListenerTest");

  /*
    @note Framework events will be sent like service events; synchronously.
          Framework events SHOULD be delivered asynchronously (see OSGi spec), however that's not yet supported.
    @todo test asynchronous event delivery once its supported.
  */

  testStartStopFrameworkEvents();
  testAddRemoveFrameworkListener();
  testFrameworkListenersAfterFrameworkStop();
  testFrameworkListenerThrowingInvariant();
#ifdef US_ENABLE_THREADING_SUPPORT
  testDeadLock();
#endif

  US_TEST_END()
}

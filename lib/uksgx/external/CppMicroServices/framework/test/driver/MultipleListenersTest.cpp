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
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/BundleEvent.h"
#include "cppmicroservices/Constants.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/ServiceEvent.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

#include <array>
#include <bitset>
#include <future>

using namespace cppmicroservices;

namespace {
enum ListenerType : short
{
  CALLBACK1 = 0,
  CALLBACK2,
  CALLBACK3,
  FUNCTOR,
  MEMFN1,
  MEMFN2,
  LAMBDA1,
  LAMBDA2,
};

int count = 0;
const int BITFIELD_LEN = 8;
std::bitset<BITFIELD_LEN> bitfield(0x0);

void callback_function_1(const FrameworkEvent&)
{
  bitfield.set(CALLBACK1);
  ++count;
}

void callback_function_2(const FrameworkEvent&)
{
  bitfield.set(CALLBACK2);
  ++count;
}

void callback_function_3(int val, const FrameworkEvent&)
{
  US_UNUSED(val);
  bitfield.set(CALLBACK3);
  ++count;
}

class CallbackFunctor
{
public:
  void operator()(const FrameworkEvent&)
  {
    bitfield.set(FUNCTOR);
    ++count;
  }
};

class Listener
{
public:
  void memfn1(const FrameworkEvent&)
  {
    bitfield.set(MEMFN1);
    ++count;
  }

  void memfn2(const FrameworkEvent&)
  {
    bitfield.set(MEMFN2);
    ++count;
  }
};

void testMultipleListeners()
{
  auto lambda1 = [](const FrameworkEvent&) {
    bitfield.set(LAMBDA1);
    ++count;
  };
  auto lambda2 = [](const FrameworkEvent&) {
    bitfield.set(LAMBDA2);
    ++count;
  };
  CallbackFunctor cb;
  Listener l1;
  Listener l2;

  auto f = FrameworkFactory().NewFramework();

  // 1. Add all listeners
  f.Init();
  BundleContext fCtx{ f.GetBundleContext() };
  fCtx.AddFrameworkListener(callback_function_1);
  fCtx.AddFrameworkListener(&callback_function_2);
  fCtx.AddFrameworkListener(&l1, &Listener::memfn1);
  fCtx.AddFrameworkListener(&l2, &Listener::memfn2);
  fCtx.AddFrameworkListener(cb);
  fCtx.AddFrameworkListener(lambda1);
  fCtx.AddFrameworkListener(lambda2);
  fCtx.AddFrameworkListener(CallbackFunctor());
  fCtx.AddFrameworkListener(
    std::bind(callback_function_3, 42, std::placeholders::_1));
  f.Start(); // generate framework event (started)
  US_TEST_CONDITION(bitfield.all(), "Test if all the listeners are triggered.");
  US_TEST_CONDITION(count == 9, "Test the listeners count.")
  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());
  US_TEST_OUTPUT(<< "-- End of testing addition of multiple listeners"
                 << "\n\n");

  // 2. Add all listeners and try removing listeners using their name
  // This removal using the names is deprecated and will be removed in the next major release.
  f.Init();
  fCtx = f.GetBundleContext();
  bitfield.reset();
  count = 0;
  // Add listeners of each variety
  fCtx.AddFrameworkListener(callback_function_1);
  fCtx.AddFrameworkListener(&callback_function_2);
  fCtx.AddFrameworkListener(&l1, &Listener::memfn1);
  fCtx.AddFrameworkListener(&l2, &Listener::memfn2);
  fCtx.AddFrameworkListener(cb);
  fCtx.AddFrameworkListener(lambda1);
  fCtx.AddFrameworkListener(lambda2);
  fCtx.AddFrameworkListener(CallbackFunctor());
  auto bind1 = std::bind(callback_function_3, 42, std::placeholders::_1);
  fCtx.AddFrameworkListener(bind1);
  // Remove listeners
  fCtx.RemoveFrameworkListener(callback_function_1);
  fCtx.RemoveFrameworkListener(&callback_function_2);
  fCtx.RemoveFrameworkListener(&l1, &Listener::memfn1);
  fCtx.RemoveFrameworkListener(&l2, &Listener::memfn2);
  fCtx.RemoveFrameworkListener(cb);
  fCtx.RemoveFrameworkListener(bind1);
  fCtx.RemoveFrameworkListener(lambda1);
  fCtx.RemoveFrameworkListener(lambda2);
  // Remove few listeners one more time.
  fCtx.RemoveFrameworkListener(callback_function_1);
  fCtx.RemoveFrameworkListener(&callback_function_2);
  fCtx.RemoveFrameworkListener(&l1, &Listener::memfn1);
  fCtx.RemoveFrameworkListener(&l2, &Listener::memfn2);
  fCtx.RemoveFrameworkListener(cb);
  fCtx.RemoveFrameworkListener(bind1);
  f.Start(); // generate framework event (started)
  US_TEST_CONDITION(bitfield.none(),
                    "Test if none of the listeners are registered.");
  US_TEST_CONDITION(count == 0, "Test the listeners count.")
  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());
  US_TEST_OUTPUT(
    << "-- End of testing removing listeners using the name of the callable"
    << "\n\n");

  // 3. Add all listeners and remove them using tokens
  f.Init();
  fCtx = f.GetBundleContext();
  auto token1 = fCtx.AddFrameworkListener(callback_function_1);
  auto token2 = fCtx.AddFrameworkListener(&callback_function_2);
  auto token3 = fCtx.AddFrameworkListener(&l1, &Listener::memfn1);
  auto token4 = fCtx.AddFrameworkListener(&l1, &Listener::memfn2);
  auto token5 = fCtx.AddFrameworkListener(&l2, &Listener::memfn1);
  auto token6 = fCtx.AddFrameworkListener(&l2, &Listener::memfn2);
  auto token7 = fCtx.AddFrameworkListener(cb);
  auto token8 = fCtx.AddFrameworkListener(lambda1);
  auto token9 = fCtx.AddFrameworkListener(lambda2);
  auto token10 = fCtx.AddFrameworkListener(CallbackFunctor());
  auto token11 = fCtx.AddFrameworkListener(bind1);
  // Remove all added listeners using tokens.
  fCtx.RemoveListener(std::move(token1));
  fCtx.RemoveListener(std::move(token2));
  fCtx.RemoveListener(std::move(token3));
  fCtx.RemoveListener(std::move(token4));
  fCtx.RemoveListener(std::move(token5));
  fCtx.RemoveListener(std::move(token6));
  fCtx.RemoveListener(std::move(token7));
  fCtx.RemoveListener(std::move(token8));
  fCtx.RemoveListener(std::move(token9));
  fCtx.RemoveListener(std::move(token10));
  fCtx.RemoveListener(std::move(token11));
  // Remove all added listeners again using the tokens. These should all be no-op.
  fCtx.RemoveListener(std::move(token1));
  fCtx.RemoveListener(std::move(token2));
  fCtx.RemoveListener(std::move(token3));
  fCtx.RemoveListener(std::move(token4));
  fCtx.RemoveListener(std::move(token5));
  fCtx.RemoveListener(std::move(token6));
  fCtx.RemoveListener(std::move(token7));
  fCtx.RemoveListener(std::move(token8));
  fCtx.RemoveListener(std::move(token9));
  fCtx.RemoveListener(std::move(token10));
  fCtx.RemoveListener(std::move(token11));
  // This should result in no output because all the listeners were successfully removed
  f.Start(); // generate framework event (started)
  US_TEST_CONDITION(bitfield.none(),
                    "Test if none of the listeners are registered.");
  US_TEST_CONDITION(count == 0, "Test the listeners count.")
  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());
  US_TEST_OUTPUT(
    << "-- End of testing addition and removing listeners using tokens"
    << "\n\n");

  // 4. Test the move ability
  f.Init();
  fCtx = f.GetBundleContext();
  token1 = fCtx.AddFrameworkListener(callback_function_1);
  US_TEST_CONDITION(token1, "Check validity of a token1");
  token2 = fCtx.AddFrameworkListener(&callback_function_2);
  token3 = fCtx.AddFrameworkListener(&l1, &Listener::memfn1);
  token3 = fCtx.AddFrameworkListener(&l2, &Listener::memfn2);
  token4 = std::move(token1); // move assignment
  US_TEST_CONDITION(!token1, "Check invalidity of a moved-from token1");
  auto token2_(std::move(token2)); // move construction
  US_TEST_CONDITION(!token2, "Check invalidity of a moved-from token2");
  ListenerToken emptytoken; // default construction
  US_TEST_CONDITION(!emptytoken,
                    "Check invalidity of a newly constructed token");
  fCtx.RemoveListener(std::move(token4));
  fCtx.RemoveListener(std::move(token2_));
  fCtx.RemoveListener(std::move(emptytoken)); // This should do nothing.
  f.Start(); // generate framework event (started)
  US_TEST_CONDITION(bitfield.to_string() == "00110000",
                    "Test if only member functions 1 & 2 are registered.");
  US_TEST_CONDITION(count == 2, "Test the listeners count.")
  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());
  US_TEST_OUTPUT(<< "-- End of testing move ability"
                 << "\n\n");
}

void testListenerTypes()
{
  class TestListener
  {
  public:
    TestListener()
      : service_count(0)
      , bundle_count(0)
      , framework_count(0)
    {}

    void serviceChanged(const ServiceEvent& evt)
    {
      US_TEST_OUTPUT(<< "ServiceEvent: " << evt);
      ++service_count;
    }
    void bundleChanged(const BundleEvent& evt)
    {
      US_TEST_OUTPUT(<< "BundleEvent: " << evt);
      ++bundle_count;
    }
    void frameworkChanged(const FrameworkEvent& evt)
    {
      US_TEST_OUTPUT(<< "FrameworkEvent: " << evt);
      ++framework_count;
    }

    std::vector<ListenerToken> tokens;
    int service_count;
    int bundle_count;
    int framework_count;
  };

  auto f = FrameworkFactory().NewFramework();

  f.Init();
  auto fCtx = f.GetBundleContext();
  TestListener tListen;

  tListen.tokens.push_back(
    fCtx.AddServiceListener(&tListen, &TestListener::serviceChanged));
  tListen.tokens.push_back(
    fCtx.AddServiceListener(&tListen, &TestListener::serviceChanged));
  tListen.tokens.push_back(
    fCtx.AddBundleListener(&tListen, &TestListener::bundleChanged));
  tListen.tokens.push_back(
    fCtx.AddBundleListener(&tListen, &TestListener::bundleChanged));
  tListen.tokens.push_back(
    fCtx.AddFrameworkListener(&tListen, &TestListener::frameworkChanged));
  tListen.tokens.push_back(
    fCtx.AddFrameworkListener(&tListen, &TestListener::frameworkChanged));

  fCtx.RemoveListener(std::move(tListen.tokens[0]));
  fCtx.RemoveListener(std::move(tListen.tokens[2]));
  fCtx.RemoveListener(std::move(tListen.tokens[4]));
  f.Start();

  auto bundleA = testing::InstallLib(fCtx, "TestBundleA");
  US_TEST_CONDITION_REQUIRED(bundleA, "Test for existing bundle TestBundleA")
  bundleA.Start();

  US_TEST_CONDITION(tListen.service_count == 1,
                    "Test for number of times service listeners got triggered")
  US_TEST_CONDITION(tListen.bundle_count > 0,
                    "Test for number of times bundle listeners got triggered")
  US_TEST_CONDITION(
    tListen.framework_count == 1,
    "Test for number of times framework listeners got triggered")

  bundleA.Stop();
  f.Stop();
  f.WaitForStop(std::chrono::milliseconds::zero());
}

#ifdef US_ENABLE_THREADING_SUPPORT
// Test the addition of thousand listeners asynchronously.
void testConcurrentAdd()
{
  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Init();
  BundleContext fCtx = framework.GetBundleContext();

  const int numAdditions = 1001;
  std::vector<ListenerToken> tokens;
  std::vector<std::future<ListenerToken>> futures;
  int count = 0;

  auto addListener = [&fCtx, &count]() -> ListenerToken {
    auto listener = [&count](const FrameworkEvent&) { ++count; };
    auto token = fCtx.AddFrameworkListener(listener);
    return token;
  };

  for (int i = 0; i < numAdditions; i++) {
    futures.push_back(std::async(std::launch::async, addListener));
  }
  for (auto& future : futures) {
    tokens.push_back(future.get());
  }
  for (auto& token : tokens) {
    fCtx.RemoveListener(std::move(token));
  }

  framework.Start();
  US_TEST_CONDITION(
    count == 0, "Testing concurrent listener addition and sequential removal.")
  framework.Stop();
  framework.WaitForStop(std::chrono::seconds(0));
}

// NOTE: These should be replaced with template lambdas and moved inside
// testConcurrectAddRemove() when we support only C++14 and beyond.
template<typename ListenerType>
ListenerToken AddListener(BundleContext&, ListenerType)
{
  return ListenerToken();
}

template<>
ListenerToken AddListener(BundleContext& fCtx, FrameworkListener listener)
{
  return fCtx.AddFrameworkListener(listener);
}

template<>
ListenerToken AddListener(BundleContext& fCtx, ServiceListener listener)
{
  return fCtx.AddServiceListener(listener);
}

template<>
ListenerToken AddListener(BundleContext& fCtx, BundleListener listener)
{
  return fCtx.AddBundleListener(listener);
}

// Test true concurrent addition and removal of listeners.
// This is a better simulation of what we want to test - that only the API
// calls that add listeners or remove listeners are executed at the same time,
// and not other boilerplate code.
template<typename ListenerType, typename EventType>
void testConcurrentAddRemove(std::string listenerStr)
{
  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Init();
  BundleContext fCtx = framework.GetBundleContext();

  const int numAdditions = 50;
  const int numRemovals = 30;
  std::vector<uint8_t> listenerFlags(numAdditions, 0);
  std::vector<ListenerToken> tokens;

  // Test concurrent addition
  // All threads wait at the 'ready.wait()' point inside the lambdas,
  // until the time when the promise 'go' is set by the main thread.
  {
    std::vector<std::future<ListenerToken>> futures;
    std::promise<void> go;
    std::shared_future<void> ready(go.get_future());
    std::vector<std::promise<void>> readies(numAdditions);

    auto addListener = [&fCtx, &readies, ready](std::vector<uint8_t>& flags,
                                                int i) -> ListenerToken {
      auto listener = [&flags, i](const EventType&) { flags[i] = 1; };
      readies[i].set_value();
      ready.wait();
      auto token = AddListener<ListenerType>(fCtx, listener);
      return token;
    };

    try {
      for (int i = 0; i < numAdditions; i++) {
        futures.push_back(std::async(
          std::launch::async, addListener, std::ref(listenerFlags), i));
      }
      for (auto& r : readies) {
        r.get_future().wait();
      }

      go.set_value();

      for (auto& future : futures) {
        tokens.push_back(future.get());
      }
    } catch (...) {
      go.set_value();
      throw;
    }
  }

  // Test concurrent removal
  // All threads wait at the 'ready.wait()' point inside the lambdas,
  // until the time when the promise 'go' is set by the main thread.
  {
    std::vector<std::future<void>> futures;
    std::promise<void> go;
    std::shared_future<void> ready(go.get_future());
    std::vector<std::promise<void>> readies(numRemovals);

    // Using ListenerToken& because of VS2013 compiler bug
    // https://connect.microsoft.com/VisualStudio/feedback/details/884836
    auto removeListener = [&fCtx, &readies, ready](int i,
                                                   ListenerToken& token) {
      readies[i].set_value();
      ready.wait();
      fCtx.RemoveListener(std::move(token));
    };

    try {
      for (int i = 0; i < numRemovals; i++) {
        futures.push_back(std::async(
          std::launch::async, removeListener, i, std::ref(tokens[i])));
      }
      for (auto& r : readies) {
        r.get_future().wait();
      }

      go.set_value();

      for (auto& future : futures) {
        future.get();
      }
    } catch (...) {
      go.set_value();
      throw;
    }
  }

  framework.Start();
  auto bundleA = testing::InstallLib(fCtx, "TestBundleA");
  bundleA.Start();

  auto countOnes = std::count(listenerFlags.begin(), listenerFlags.end(), 1);
  US_TEST_OUTPUT(<< "");
  std::string message("Testing concurrent " + listenerStr +
                      " addition and removal");
  US_TEST_CONDITION(countOnes == numAdditions - numRemovals, message)
  bundleA.Stop();
  framework.Stop();
  framework.WaitForStop(std::chrono::seconds(0));
}
#endif // US_ENABLE_THREADING_SUPPORT
}

int MultipleListenersTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("MultipleListenersTest");
  testMultipleListeners();
  testListenerTypes();

#ifdef US_ENABLE_THREADING_SUPPORT
  testConcurrentAdd();
  // Test all the three types of listeners.
  testConcurrentAddRemove<FrameworkListener, FrameworkEvent>(
    "framework listener");
  testConcurrentAddRemove<BundleListener, BundleEvent>("bundle listener");
  testConcurrentAddRemove<ServiceListener, ServiceEvent>("service listener");
#endif

  US_TEST_END()
}

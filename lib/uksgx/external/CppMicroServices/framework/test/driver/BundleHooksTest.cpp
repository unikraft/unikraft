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
#include "cppmicroservices/BundleEventHook.h"
#include "cppmicroservices/BundleFindHook.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/GetBundleContext.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

using namespace cppmicroservices;

namespace {

class TestBundleListener
{
public:
  void BundleChanged(const BundleEvent& bundleEvent)
  {
    this->events.push_back(bundleEvent);
  }

  std::vector<BundleEvent> events;
};

class TestBundleFindHook : public BundleFindHook
{
public:
  void Find(const BundleContext& /*context*/, ShrinkableVector<Bundle>& bundles)
  {
    for (auto i = bundles.begin(); i != bundles.end();) {
      if (i->GetSymbolicName() == "TestBundleA") {
        i = bundles.erase(i);
      } else {
        ++i;
      }
    }
  }
};

class TestBundleEventHook : public BundleEventHook
{
public:
  void Event(const BundleEvent& event,
             ShrinkableVector<BundleContext>& contexts)
  {
    if (event.GetType() == BundleEvent::BUNDLE_STARTING ||
        event.GetType() == BundleEvent::BUNDLE_STOPPING) {
      contexts
        .clear(); //erase(std::remove(contexts.begin(), contexts.end(), GetBundleContext()), contexts.end());
    }
  }
};

void TestFindHook(const Framework& framework)
{
  auto bundleA =
    testing::InstallLib(framework.GetBundleContext(), "TestBundleA");
  US_TEST_CONDITION_REQUIRED(bundleA, "Test for existing bundle TestBundleA")

  US_TEST_CONDITION(bundleA.GetSymbolicName() == "TestBundleA",
                    "Test bundle name")

  bundleA.Start();

  US_TEST_CONDITION(bundleA.GetState() & Bundle::STATE_ACTIVE,
                    "Test if started correctly");

  long bundleAId = bundleA.GetBundleId();
  US_TEST_CONDITION_REQUIRED(bundleAId > 0, "Test for valid bundle id")

  US_TEST_CONDITION_REQUIRED(framework.GetBundleContext().GetBundle(bundleAId),
                             "Test for non-filtered GetBundle(long) result")

  auto findHookReg =
    framework.GetBundleContext().RegisterService<BundleFindHook>(
      std::make_shared<TestBundleFindHook>());

  US_TEST_CONDITION_REQUIRED(!framework.GetBundleContext().GetBundle(bundleAId),
                             "Test for filtered GetBundle(long) result")

  auto bundles = framework.GetBundleContext().GetBundles();
  for (auto const& i : bundles) {
    if (i.GetSymbolicName() == "TestBundleA") {
      US_TEST_FAILED_MSG(<< "TestBundleA not filtered from GetBundles()")
    }
  }

  findHookReg.Unregister();

  bundleA.Stop();
}

void TestEventHook(const Framework& framework)
{
  TestBundleListener bundleListener;
  framework.GetBundleContext().AddBundleListener(
    &bundleListener, &TestBundleListener::BundleChanged);

  auto bundleA =
    testing::InstallLib(framework.GetBundleContext(), "TestBundleA");
  US_TEST_CONDITION_REQUIRED(bundleA, "Non-null bundle")

  bundleA.Start();
  US_TEST_CONDITION_REQUIRED(bundleListener.events.size() == 2,
                             "Test for received load bundle events")

  bundleA.Stop();
  US_TEST_CONDITION_REQUIRED(bundleListener.events.size() == 4,
                             "Test for received unload bundle events")

  auto eventHookReg =
    framework.GetBundleContext().RegisterService<BundleEventHook>(
      std::make_shared<TestBundleEventHook>());

  bundleListener.events.clear();

  bundleA.Start();
  US_TEST_CONDITION_REQUIRED(bundleListener.events.size() == 1,
                             "Test for filtered load bundle events")
  US_TEST_CONDITION_REQUIRED(bundleListener.events[0].GetType() ==
                               BundleEvent::BUNDLE_STARTED,
                             "Test for BUNDLE_STARTED event")

  bundleA.Stop();
  US_TEST_CONDITION_REQUIRED(bundleListener.events.size() == 2,
                             "Test for filtered unload bundle events")
  US_TEST_CONDITION_REQUIRED(bundleListener.events[1].GetType() ==
                               BundleEvent::BUNDLE_STOPPED,
                             "Test for BUNDLE_STOPPED event")

  eventHookReg.Unregister();
  framework.GetBundleContext().RemoveBundleListener(
    &bundleListener, &TestBundleListener::BundleChanged);
}

} // end unnamed namespace

int BundleHooksTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("BundleHooksTest");

  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Start();

  TestFindHook(framework);
  TestEventHook(framework);

  US_TEST_END()
}

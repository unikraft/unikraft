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
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/GetBundleContext.h"
#include "cppmicroservices/LDAPProp.h"
#include "cppmicroservices/ServiceEvent.h"
#include "cppmicroservices/ServiceEventListenerHook.h"
#include "cppmicroservices/ServiceFindHook.h"
#include "cppmicroservices/ServiceListenerHook.h"

#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

#include <unordered_set>

using namespace cppmicroservices;

namespace {

class TestServiceListener
{
public:
  void ServiceChanged(const ServiceEvent& serviceEvent)
  {
    this->events.push_back(serviceEvent);
  }

  std::vector<ServiceEvent> events;
};

class TestServiceEventListenerHook : public ServiceEventListenerHook
{
private:
  int id;
  BundleContext bundleCtx;

public:
  TestServiceEventListenerHook(int id, const BundleContext& context)
    : id(id)
    , bundleCtx(context)
  {}

  typedef ShrinkableMap<BundleContext,
                        ShrinkableVector<ServiceListenerHook::ListenerInfo>>
    MapType;

  void Event(const ServiceEvent& /*event*/, MapType& listeners)
  {
    US_TEST_CONDITION_REQUIRED(listeners.size() > 0 &&
                                 listeners.find(bundleCtx) != listeners.end(),
                               "Check listener content");
    ShrinkableVector<ServiceListenerHook::ListenerInfo>& listenerInfos =
      listeners[bundleCtx];

    // listener count should be 2 because the event listener hooks are called with
    // the list of listeners before filtering them according to ther LDAP filter
    if (id == 1) {
#ifdef US_BUILD_SHARED_LIBS
      US_TEST_CONDITION(listenerInfos.size() == 2,
                        "2 service listeners expected");
#else
      US_TEST_CONDITION(listenerInfos.size() >= 2,
                        "2 service listeners expected");
#endif
      US_TEST_CONDITION(listenerInfos[0].IsRemoved() == false,
                        "Listener is not removed");
      US_TEST_CONDITION(listenerInfos[1].IsRemoved() == false,
                        "Listener is not removed");
      US_TEST_CONDITION(!(listenerInfos[0] == listenerInfos[1]),
                        "listener info inequality");
    } else {
      // there is already one listener filtered out
#ifdef US_BUILD_SHARED_LIBS
      US_TEST_CONDITION(listenerInfos.size() == 1,
                        "1 service listener expected");
#else
      US_TEST_CONDITION(listenerInfos.size() >= 1,
                        "1 service listener expected");
#endif
      US_TEST_CONDITION(listenerInfos[0].IsRemoved() == false,
                        "Listener is not removed");
    }
    if (listenerInfo.IsNull()) {
      listenerInfo = listenerInfos[0];
    } else {
      US_TEST_CONDITION(listenerInfo == listenerInfos[0],
                        "Equal listener info objects");
    }

    // Remove the listener without a filter from the list
    for (ShrinkableVector<ServiceListenerHook::ListenerInfo>::iterator
           infoIter = listenerInfos.begin();
         infoIter != listenerInfos.end();) {
      if (infoIter->GetFilter().empty()) {
        infoIter = listenerInfos.erase(infoIter);
      } else {
        ++infoIter;
      }
    }
#ifdef US_BUILD_SHARED_LIBS
    US_TEST_CONDITION(listenerInfos.size() == 1,
                      "One listener with LDAP filter should remain");
#else
    US_TEST_CONDITION(listenerInfos.size() >= 1,
                      "One listener with LDAP filter should remain");
#endif

    ordering.push_back(id);
  }

  ServiceListenerHook::ListenerInfo listenerInfo;

  static std::vector<int> ordering;
};

std::vector<int> TestServiceEventListenerHook::ordering;

class TestServiceFindHook : public ServiceFindHook
{
private:
  int id;
  BundleContext bundleCtx;

public:
  TestServiceFindHook(int id, const BundleContext& context)
    : id(id)
    , bundleCtx(context)
  {}

  void Find(const BundleContext& context,
            const std::string& /*name*/,
            const std::string& /*filter*/,
            ShrinkableVector<ServiceReferenceBase>& references)
  {
    US_TEST_CONDITION(context == bundleCtx, "Bundle context");

    references.clear();
    ordering.push_back(id);
  }

  static std::vector<int> ordering;
};

std::vector<int> TestServiceFindHook::ordering;

class TestServiceListenerHook : public ServiceListenerHook
{
private:
  int id;
  BundleContext bundleCtx;

public:
  TestServiceListenerHook(int id, const BundleContext& context)
    : id(id)
    , bundleCtx(context)
  {}

  void Added(const std::vector<ListenerInfo>& listeners)
  {
    for (std::vector<ListenerInfo>::const_iterator iter = listeners.begin();
         iter != listeners.end();
         ++iter) {
      if (iter->IsRemoved() ||
          iter->GetBundleContext().GetBundle() != bundleCtx.GetBundle())
        continue;
      listenerInfos.insert(*iter);
      lastAdded = listeners.back();
      ordering.push_back(id);
    }
  }

  void Removed(const std::vector<ListenerInfo>& listeners)
  {
    for (std::vector<ListenerInfo>::const_iterator iter = listeners.begin();
         iter != listeners.end();
         ++iter) {
      listenerInfos.erase(*iter);
      ordering.push_back(id * 10);
    }
    lastRemoved = listeners.back();
  }

  static std::vector<int> ordering;

  std::unordered_set<ListenerInfo> listenerInfos;
  ListenerInfo lastAdded;
  ListenerInfo lastRemoved;
};

std::vector<int> TestServiceListenerHook::ordering;

void TestEventListenerHook(const Framework& framework)
{
  auto context =
    testing::GetBundle("main", framework.GetBundleContext()).GetBundleContext();

  TestServiceListener serviceListener1;
  TestServiceListener serviceListener2;
  context.AddServiceListener(&serviceListener1,
                             &TestServiceListener::ServiceChanged);
  context.AddServiceListener(&serviceListener2,
                             &TestServiceListener::ServiceChanged,
                             LDAPProp(Constants::OBJECTCLASS) == "bla");

  auto serviceEventListenerHook1 =
    std::make_shared<TestServiceEventListenerHook>(1, context);
  ServiceProperties hookProps1;
  hookProps1[Constants::SERVICE_RANKING] = 10;
  ServiceRegistration<ServiceEventListenerHook> eventListenerHookReg1 =
    context.RegisterService<ServiceEventListenerHook>(serviceEventListenerHook1,
                                                      hookProps1);

  auto serviceEventListenerHook2 =
    std::make_shared<TestServiceEventListenerHook>(2, context);
  ServiceProperties hookProps2;
  hookProps2[Constants::SERVICE_RANKING] = 0;
  ServiceRegistration<ServiceEventListenerHook> eventListenerHookReg2 =
    context.RegisterService<ServiceEventListenerHook>(serviceEventListenerHook2,
                                                      hookProps2);

  std::vector<int> expectedOrdering;
  expectedOrdering.push_back(1);
  expectedOrdering.push_back(1);
  expectedOrdering.push_back(2);
  US_TEST_CONDITION(serviceEventListenerHook1->ordering == expectedOrdering,
                    "Event listener hook call order");

  US_TEST_CONDITION(serviceListener1.events.empty(),
                    "service event of service event listener hook");
  US_TEST_CONDITION(serviceListener2.events.empty(),
                    "no service event for filtered listener");

  auto bundle = testing::InstallLib(context, "TestBundleA");
  US_TEST_CONDITION_REQUIRED(bundle, "non-null installed bundle");
  bundle.Start();

  expectedOrdering.push_back(1);
  expectedOrdering.push_back(2);
  US_TEST_CONDITION(serviceEventListenerHook1->ordering == expectedOrdering,
                    "Event listener hook call order");

  bundle.Stop();

  US_TEST_CONDITION(serviceListener1.events.empty(),
                    "no service event due to service event listener hook");
  US_TEST_CONDITION(serviceListener2.events.empty(),
                    "no service event for filtered listener due to service "
                    "event listener hook");

  eventListenerHookReg2.Unregister();
  eventListenerHookReg1.Unregister();

  context.RemoveServiceListener(&serviceListener1,
                                &TestServiceListener::ServiceChanged);
  context.RemoveServiceListener(&serviceListener2,
                                &TestServiceListener::ServiceChanged);
}

void TestListenerHook(const Framework& framework)
{
  auto context =
    testing::GetBundle("main", framework.GetBundleContext()).GetBundleContext();

  TestServiceListener serviceListener1;
  TestServiceListener serviceListener2;
  context.AddServiceListener(&serviceListener1,
                             &TestServiceListener::ServiceChanged);
  context.AddServiceListener(&serviceListener2,
                             &TestServiceListener::ServiceChanged,
                             LDAPProp(Constants::OBJECTCLASS) == "bla");

  auto serviceListenerHook1 =
    std::make_shared<TestServiceListenerHook>(1, context);
  ServiceProperties hookProps1;
  hookProps1[Constants::SERVICE_RANKING] = 0;
  ServiceRegistration<ServiceListenerHook> listenerHookReg1 =
    context.RegisterService<ServiceListenerHook>(serviceListenerHook1,
                                                 hookProps1);

  auto serviceListenerHook2 =
    std::make_shared<TestServiceListenerHook>(2, context);
  ServiceProperties hookProps2;
  hookProps2[Constants::SERVICE_RANKING] = 10;
  ServiceRegistration<ServiceListenerHook> listenerHookReg2 =
    context.RegisterService<ServiceListenerHook>(serviceListenerHook2,
                                                 hookProps2);

#ifdef US_BUILD_SHARED_LIBS
  // check if hooks got notified about the existing listeners
  US_TEST_CONDITION_REQUIRED(serviceListenerHook1->listenerInfos.size() == 2,
                             "Notification about existing listeners")
#endif
  const std::size_t listenerInfoSizeOld =
    serviceListenerHook1->listenerInfos.size() - 2;

  context.AddServiceListener(&serviceListener1,
                             &TestServiceListener::ServiceChanged);
  auto lastAdded = serviceListenerHook1->lastAdded;

#ifdef US_BUILD_SHARED_LIBS
  std::vector<int> expectedOrdering;
  expectedOrdering.push_back(1);
  expectedOrdering.push_back(1);
  expectedOrdering.push_back(2);
  expectedOrdering.push_back(2);
  expectedOrdering.push_back(20);
  expectedOrdering.push_back(10);
  expectedOrdering.push_back(2);
  expectedOrdering.push_back(1);
  US_TEST_CONDITION(serviceListenerHook1->ordering == expectedOrdering,
                    "Listener hook call order");
#endif

  context.AddServiceListener(&serviceListener1,
                             &TestServiceListener::ServiceChanged,
                             LDAPProp(Constants::OBJECTCLASS) == "blub");
  US_TEST_CONDITION(lastAdded == serviceListenerHook1->lastRemoved,
                    "Same ListenerInfo object)");
  US_TEST_CONDITION(!(lastAdded == serviceListenerHook1->lastAdded),
                    "New ListenerInfo object)");

#ifdef US_BUILD_SHARED_LIBS
  expectedOrdering.push_back(20);
  expectedOrdering.push_back(10);
  expectedOrdering.push_back(2);
  expectedOrdering.push_back(1);
  US_TEST_CONDITION(serviceListenerHook1->ordering == expectedOrdering,
                    "Listener hook call order");
#endif

  context.RemoveServiceListener(&serviceListener1,
                                &TestServiceListener::ServiceChanged);
  context.RemoveServiceListener(&serviceListener2,
                                &TestServiceListener::ServiceChanged);

#ifdef US_BUILD_SHARED_LIBS
  expectedOrdering.push_back(20);
  expectedOrdering.push_back(10);
  expectedOrdering.push_back(20);
  expectedOrdering.push_back(10);
  US_TEST_CONDITION(serviceListenerHook1->ordering == expectedOrdering,
                    "Listener hook call order");
#endif

  US_TEST_CONDITION_REQUIRED(serviceListenerHook1->listenerInfos.size() ==
                               listenerInfoSizeOld,
                             "Removed listener infos")

  listenerHookReg2.Unregister();
  listenerHookReg1.Unregister();
}

void TestFindHook(const Framework& framework)
{
  auto context =
    testing::GetBundle("main", framework.GetBundleContext()).GetBundleContext();

  auto serviceFindHook1 = std::make_shared<TestServiceFindHook>(1, context);
  ServiceProperties hookProps1;
  hookProps1[Constants::SERVICE_RANKING] = 0;
  ServiceRegistration<ServiceFindHook> findHookReg1 =
    context.RegisterService<ServiceFindHook>(serviceFindHook1, hookProps1);

  auto serviceFindHook2 = std::make_shared<TestServiceFindHook>(2, context);
  ServiceProperties hookProps2;
  hookProps2[Constants::SERVICE_RANKING] = 10;
  ServiceRegistration<ServiceFindHook> findHookReg2 =
    context.RegisterService<ServiceFindHook>(serviceFindHook2, hookProps2);

  std::vector<int> expectedOrdering;
  US_TEST_CONDITION(serviceFindHook1->ordering == expectedOrdering,
                    "Find hook call order");

  TestServiceListener serviceListener;
  context.AddServiceListener(&serviceListener,
                             &TestServiceListener::ServiceChanged);

  auto bundle = testing::InstallLib(context, "TestBundleA");
  US_TEST_CONDITION_REQUIRED(bundle, "non-null installed bundle");

  bundle.Start();

  US_TEST_CONDITION(serviceListener.events.size() == 1, "Service registered");

  std::vector<ServiceReferenceU> refs =
    context.GetServiceReferences("cppmicroservices::TestBundleAService");
  US_TEST_CONDITION(refs.empty(), "Empty references");
  ServiceReferenceU ref =
    context.GetServiceReference("cppmicroservices::TestBundleAService");
  US_TEST_CONDITION(!ref, "Invalid reference (filtered out)");

  expectedOrdering.push_back(2);
  expectedOrdering.push_back(1);
  expectedOrdering.push_back(2);
  expectedOrdering.push_back(1);

  US_TEST_CONDITION(serviceFindHook1->ordering == expectedOrdering,
                    "Find hook call order");

  findHookReg2.Unregister();
  findHookReg1.Unregister();

  refs = context.GetServiceReferences("cppmicroservices::TestBundleAService");
  US_TEST_CONDITION(!refs.empty(), "Non-empty references");
  ref = context.GetServiceReference("cppmicroservices::TestBundleAService");
  US_TEST_CONDITION(ref, "Valid reference");

  bundle.Stop();

  context.RemoveServiceListener(&serviceListener,
                                &TestServiceListener::ServiceChanged);
}

} // end unnamed namespace

int ServiceHooksTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("ServiceHooksTest");

  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Start();

  try {
    auto bundles = framework.GetBundleContext().GetBundles();
    US_TEST_CONDITION_REQUIRED(!bundles.empty(),
                               "Test installation of bundle main")
    for (auto& b : bundles) {
      if (b.GetSymbolicName() == "main") {
        b.Start();
        break;
      }
    }
  } catch (const std::exception& e) {
    US_TEST_FAILED_MSG(<< "Install bundle exception: " << e.what())
  }

  TestListenerHook(framework);
  TestFindHook(framework);
  TestEventListenerHook(framework);

  US_TEST_END()
}

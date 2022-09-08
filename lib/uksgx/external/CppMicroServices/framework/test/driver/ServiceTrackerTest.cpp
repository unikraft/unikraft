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

#include "cppmicroservices/ServiceTracker.h"
#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/Framework.h"
#include "cppmicroservices/FrameworkFactory.h"
#include "cppmicroservices/GetBundleContext.h"
#include "cppmicroservices/ServiceInterface.h"

#include "ServiceControlInterface.h"
#include "TestUtils.h"
#include "TestingConfig.h"
#include "TestingMacros.h"

#include <future>
#include <memory>
#include <chrono>

using namespace cppmicroservices;

bool CheckConvertibility(const std::vector<ServiceReferenceU>& refs,
                         std::vector<std::string>::const_iterator idBegin,
                         std::vector<std::string>::const_iterator idEnd)
{
  std::vector<std::string> ids;
  ids.assign(idBegin, idEnd);

  for (std::vector<ServiceReferenceU>::const_iterator sri = refs.begin();
       sri != refs.end();
       ++sri) {
    for (std::vector<std::string>::iterator idIter = ids.begin();
         idIter != ids.end();
         ++idIter) {
      if (sri->IsConvertibleTo(*idIter)) {
        ids.erase(idIter);
        break;
      }
    }
  }

  return ids.empty();
}

struct MyInterfaceOne
{
  virtual ~MyInterfaceOne() {}
};

struct MyInterfaceTwo
{
  virtual ~MyInterfaceTwo() {}
};

class MyCustomizer
  : public cppmicroservices::ServiceTrackerCustomizer<MyInterfaceOne>
{

public:
  MyCustomizer(const BundleContext& context)
    : m_context(context)
  {}

  virtual std::shared_ptr<MyInterfaceOne> AddingService(
    const ServiceReference<MyInterfaceOne>& reference)
  {
    US_TEST_CONDITION_REQUIRED(reference, "AddingService() valid reference")
    return m_context.GetService(reference);
  }

  virtual void ModifiedService(
    const ServiceReference<MyInterfaceOne>& reference,
    const std::shared_ptr<MyInterfaceOne>& service)
  {
    US_TEST_CONDITION(reference, "ModifiedService() valid reference")
    US_TEST_CONDITION(service, "ModifiedService() valid service")
  }

  virtual void RemovedService(const ServiceReference<MyInterfaceOne>& reference,
                              const std::shared_ptr<MyInterfaceOne>& service)
  {
    US_TEST_CONDITION(reference, "RemovedService() valid reference")
    US_TEST_CONDITION(service, "RemovedService() valid service")
  }

private:
  BundleContext m_context;
};

void TestFilterString(BundleContext context)
{
  MyCustomizer customizer(context);

  cppmicroservices::LDAPFilter filter(
    "(" + cppmicroservices::Constants::SERVICE_ID + ">=0)");
  cppmicroservices::ServiceTracker<MyInterfaceOne> tracker(
    context, filter, &customizer);
  tracker.Open();

  struct MyServiceOne : public MyInterfaceOne
  {};
  struct MyServiceTwo : public MyInterfaceTwo
  {};

  auto serviceOne = std::make_shared<MyServiceOne>();
  auto serviceTwo = std::make_shared<MyServiceTwo>();

  context.RegisterService<MyInterfaceOne>(serviceOne);
  context.RegisterService<MyInterfaceTwo>(serviceTwo);

  US_TEST_CONDITION(tracker.GetServiceReferences().size() == 1,
                    "tracking count")
}

void TestServiceTracker(BundleContext context)
{
  auto bundle = testing::InstallLib(context, "TestBundleS");
  bundle.Start();

  // 1. Create a ServiceTracker with ServiceTrackerCustomizer == null

  std::string s1("cppmicroservices::TestBundleSService");
  ServiceReferenceU servref = context.GetServiceReference(s1 + "0");

  US_TEST_CONDITION_REQUIRED(
    servref,
    "Test if registered service of id cppmicroservices::TestBundleSService0");

  ServiceReference<ServiceControlInterface> servCtrlRef =
    context.GetServiceReference<ServiceControlInterface>();
  US_TEST_CONDITION_REQUIRED(servCtrlRef,
                             "Test if constrol service was registered");

  auto serviceController = context.GetService(servCtrlRef);
  US_TEST_CONDITION_REQUIRED(serviceController,
                             "Test valid service controller");

  std::unique_ptr<ServiceTracker<void>> st1(
    new ServiceTracker<void>(context, servref));

  // 2. Check the size method with an unopened service tracker

  US_TEST_CONDITION_REQUIRED(st1->Size() == 0, "Test if size == 0");

  // 3. Open the service tracker and see what it finds,
  // expect to find one instance of the implementation,
  // "org.cppmicroservices.TestBundleSService0"

  st1->Open();
  std::vector<ServiceReferenceU> sa2 = st1->GetServiceReferences();

  US_TEST_CONDITION_REQUIRED(sa2.size() == 1, "Checking ServiceTracker size");
  US_TEST_CONDITION_REQUIRED(s1 + "0" == sa2[0].GetInterfaceId(),
                             "Checking service implementation name");

#ifdef US_ENABLE_THREADING_SUPPORT
  // 4. Test notifications via closing the tracker
  {
    ServiceTracker<void> st2(context, "dummy");
    st2.Open();

    // wait indefinitely
    auto fut1 =
      std::async(std::launch::async, [&st2] { return st2.WaitForService(); });
    // wait "long enough"
    auto fut2 = std::async(std::launch::async, [&st2] {
      return st2.WaitForService(std::chrono::minutes(1));
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    US_TEST_CONDITION_REQUIRED(fut1.wait_for(std::chrono::milliseconds(1)) ==
                                 US_FUTURE_TIMEOUT,
                               "Waiter not notified yet");
    US_TEST_CONDITION_REQUIRED(fut2.wait_for(std::chrono::milliseconds(1)) ==
                                 US_FUTURE_TIMEOUT,
                               "Waiter not notified yet");

    st2.Close();

    // Closing the tracker should notify the waiters
    auto wait_until = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    US_TEST_CONDITION_REQUIRED(fut1.wait_until(wait_until) == US_FUTURE_READY,
                               "Closed service tracker notifies waiters");
    US_TEST_CONDITION_REQUIRED(fut2.wait_until(wait_until) == US_FUTURE_READY,
                               "Closed service tracker notifies waiters");
  }
#endif

  // 5. Close this service tracker
  st1->Close();

  // 6. Check the size method, now when the servicetracker is closed
  US_TEST_CONDITION_REQUIRED(st1->Size() == 0, "Checking ServiceTracker size");

  // 7. Check if we still track anything , we should get null
  sa2 = st1->GetServiceReferences();
  US_TEST_CONDITION_REQUIRED(sa2.empty(), "Checking ServiceTracker size");

  // 8. A new Servicetracker, this time with a filter for the object
  std::string fs =
    std::string("(") + Constants::OBJECTCLASS + "=" + s1 + "*" + ")";
  LDAPFilter f1(fs);
  st1.reset(new ServiceTracker<void>(context, f1));
  // add a service
  serviceController->ServiceControl(1, "register", 7);

  // 9. Open the service tracker and see what it finds,
  // expect to find two instances of references to
  // "org.cppmicroservices.TestBundleSService*"
  // i.e. they refer to the same piece of code

  std::vector<std::string> ids;
  ids.push_back((s1 + "0"));
  ids.push_back((s1 + "1"));
  ids.push_back((s1 + "2"));
  ids.push_back((s1 + "3"));

  st1->Open();
  sa2 = st1->GetServiceReferences();
  US_TEST_CONDITION_REQUIRED(sa2.size() == 2,
                             "Checking service reference count");
  US_TEST_CONDITION_REQUIRED(
    CheckConvertibility(sa2, ids.begin(), ids.begin() + 2),
    "Check for expected interface id [0]");
  US_TEST_CONDITION_REQUIRED(sa2[1].IsConvertibleTo(s1 + "1"),
                             "Check for expected interface id [1]");

  // 10. Get libTestBundleS to register one more service and see if it appears
  serviceController->ServiceControl(2, "register", 1);
  sa2 = st1->GetServiceReferences();

  US_TEST_CONDITION_REQUIRED(sa2.size() == 3,
                             "Checking service reference count");

  US_TEST_CONDITION_REQUIRED(
    CheckConvertibility(sa2, ids.begin(), ids.begin() + 3),
    "Check for expected interface id [2]");

  // 11. Get libTestBundleS to register one more service and see if it appears
  serviceController->ServiceControl(3, "register", 2);
  sa2 = st1->GetServiceReferences();
  US_TEST_CONDITION_REQUIRED(sa2.size() == 4,
                             "Checking service reference count");
  US_TEST_CONDITION_REQUIRED(CheckConvertibility(sa2, ids.begin(), ids.end()),
                             "Check for expected interface id [3]");

  // 12. Get libTestBundleS to unregister one service and see if it disappears
  serviceController->ServiceControl(3, "unregister", 0);
  sa2 = st1->GetServiceReferences();
  US_TEST_CONDITION_REQUIRED(sa2.size() == 3,
                             "Checking service reference count");

  // 13. Get the highest ranking service reference, it should have ranking 7
  ServiceReferenceU h1 = st1->GetServiceReference();
  int rank = any_cast<int>(h1.GetProperty(Constants::SERVICE_RANKING));
  US_TEST_CONDITION_REQUIRED(rank == 7, "Check service rank");

  // 14. Get the service of the highest ranked service reference

  auto o1 = st1->GetService(h1);
  US_TEST_CONDITION_REQUIRED(o1.get() != nullptr && !o1->empty(),
                             "Check for non-null service");

  // 14a Get the highest ranked service, directly this time
  auto o3 = st1->GetService();
  US_TEST_CONDITION_REQUIRED(o3.get() != nullptr && !o3->empty(),
                             "Check for non-null service");
  US_TEST_CONDITION_REQUIRED(o1 == o3, "Check for equal service instances");

  // 15. Now release the tracking of that service and then try to get it
  //     from the servicetracker, which should yield a null object
  serviceController->ServiceControl(1, "unregister", 7);
  auto o2 = st1->GetService(h1);
  US_TEST_CONDITION_REQUIRED(!o2 || !o2.get(), "Check that service is null");

  // 16. Get all service objects this tracker tracks, it should be 2
  auto ts1 = st1->GetServices();
  US_TEST_CONDITION_REQUIRED(ts1.size() == 2, "Check service count");

  // 17. Test the remove method.
  //     First register another service, then remove it being tracked
  serviceController->ServiceControl(1, "register", 7);
  h1 = st1->GetServiceReference();
  auto sa3 = st1->GetServiceReferences();
  US_TEST_CONDITION_REQUIRED(sa3.size() == 3, "Check service reference count");
  US_TEST_CONDITION_REQUIRED(
    CheckConvertibility(sa3, ids.begin(), ids.begin() + 3),
    "Check for expected interface id [0]");

  st1->Remove(h1); // remove tracking on one servref
  sa2 = st1->GetServiceReferences();
  US_TEST_CONDITION_REQUIRED(sa2.size() == 2, "Check service reference count");

  // 18. Test the addingService method,add a service reference

  // 19. Test the removedService method, remove a service reference

  // 20. Test the waitForService method
  auto o9 = st1->WaitForService(std::chrono::milliseconds(50));
  US_TEST_CONDITION_REQUIRED(o9 && !o9->empty(),
                             "Checking WaitForService method");
}

int ServiceTrackerTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("ServiceTrackerTest")

  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Start();

  TestFilterString(framework.GetBundleContext());
  TestServiceTracker(framework.GetBundleContext());

  US_TEST_END()
}

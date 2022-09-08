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
#include "cppmicroservices/ServiceEvent.h"

#include "TestUtils.h"
#include "TestingMacros.h"

#include <vector>

using namespace cppmicroservices;

class MyServiceListener;

struct IPerfTestService
{
  virtual ~IPerfTestService() {}
};

class ServiceRegistryPerformanceTest
{

private:
  friend class MyServiceListener;

  BundleContext context;

  int nListeners;
  int nServices;

  std::size_t nRegistered;
  std::size_t nUnregistering;
  std::size_t nModified;

  std::vector<ServiceRegistration<IPerfTestService>> regs;
  std::vector<MyServiceListener*> listeners;
  std::vector<std::shared_ptr<IPerfTestService>> services;

public:
  ServiceRegistryPerformanceTest(const BundleContext& context);

  void InitTestCase();
  void CleanupTestCase();

  void TestAddListeners();
  void TestRegisterServices();

  void TestModifyServices();
  void TestUnregisterServices();

private:
  std::ostream& Log() const { return std::cout; }

  void AddListeners(int n);
  void RegisterServices(int n);
  void ModifyServices();
  void UnregisterServices();
};

class MyServiceListener
{

private:
  ServiceRegistryPerformanceTest* ts;

public:
  MyServiceListener(ServiceRegistryPerformanceTest* ts)
    : ts(ts)
  {}

  void ServiceChanged(const ServiceEvent& ev)
  {
    switch (ev.GetType()) {
      case ServiceEvent::SERVICE_REGISTERED:
        ts->nRegistered++;
        break;
      case ServiceEvent::SERVICE_UNREGISTERING:
        ts->nUnregistering++;
        break;
      case ServiceEvent::SERVICE_MODIFIED:
        ts->nModified++;
        break;
      default:
        break;
    }
  }
};

ServiceRegistryPerformanceTest::ServiceRegistryPerformanceTest(
  const BundleContext& context)
  : context(context)
  , nListeners(100)
  , nServices(1000)
  , nRegistered(0)
  , nUnregistering(0)
  , nModified(0)
{}

void ServiceRegistryPerformanceTest::InitTestCase()
{
  Log() << "Initialize event counters\n";

  nRegistered = 0;
  nUnregistering = 0;
  nModified = 0;
}

void ServiceRegistryPerformanceTest::CleanupTestCase()
{
  Log() << "Remove all service listeners\n";

  for (std::size_t i = 0; i < listeners.size(); i++) {
    try {
      MyServiceListener* l = listeners[i];
      context.RemoveServiceListener(l, &MyServiceListener::ServiceChanged);
      delete l;
    } catch (const std::exception& e) {
      Log() << e.what();
    }
  }
  listeners.clear();
}

void ServiceRegistryPerformanceTest::TestAddListeners()
{
  AddListeners(nListeners);
}

void ServiceRegistryPerformanceTest::AddListeners(int n)
{
  Log() << "adding " << n << " service listeners\n";
  for (int i = 0; i < n; i++) {
    MyServiceListener* l = new MyServiceListener(this);
    try {
      listeners.push_back(l);
      context.AddServiceListener(
        l, &MyServiceListener::ServiceChanged, "(perf.service.value>=0)");
    } catch (const std::exception& e) {
      Log() << e.what();
    }
  }
  Log() << "listener count=" << listeners.size() << "\n";
}

void ServiceRegistryPerformanceTest::TestRegisterServices()
{
  Log() << "Register services, and check that we get #of services ("
        << nServices << ") * #of listeners (" << nListeners
        << ")  SERVICE_REGISTERED events\n";

  Log() << "registering " << nServices
        << " services, listener count=" << listeners.size() << "\n";

  testing::HighPrecisionTimer t;
  t.Start();
  RegisterServices(nServices);
  long long ms = t.ElapsedMilli();
  Log() << "register took " << ms << "ms\n";
  US_TEST_CONDITION_REQUIRED(nServices * listeners.size() == nRegistered,
                             "# SERVICE_REGISTERED events must be same as # of "
                             "registered services  * # of listeners");
}

void ServiceRegistryPerformanceTest::RegisterServices(int n)
{
  class PerfTestService : public IPerfTestService
  {};

  std::string pid("my.service.");

  for (int i = 0; i < n; i++) {
    ServiceProperties props;
    std::stringstream ss;
    ss << pid << i;
    props["service.pid"] = ss.str();
    props["perf.service.value"] = i + 1;

    services.emplace_back(std::make_shared<PerfTestService>());
    ServiceRegistration<IPerfTestService> reg =
      context.RegisterService<IPerfTestService>(services.back(), props);
    regs.push_back(reg);
  }
}

void ServiceRegistryPerformanceTest::TestModifyServices()
{
  Log() << "Modify all services, and check that we get #of services ("
        << nServices << ") * #of listeners (" << nListeners
        << ")  SERVICE_MODIFIED events\n";

  testing::HighPrecisionTimer t;
  t.Start();
  ModifyServices();
  long long ms = t.ElapsedMilli();
  Log() << "modify took " << ms << "ms\n";
  US_TEST_CONDITION_REQUIRED(nServices * listeners.size() == nModified,
                             "# SERVICE_MODIFIED events must be same as # of "
                             "modified services  * # of listeners");
}

void ServiceRegistryPerformanceTest::ModifyServices()
{
  Log() << "modifying " << regs.size()
        << " services, listener count=" << listeners.size() << "\n";

  for (std::size_t i = 0; i < regs.size(); i++) {
    ServiceRegistration<IPerfTestService> reg = regs[i];
    ServiceProperties props;
    props["perf.service.value"] = i * 2;
    reg.SetProperties(props);
  }
}

void ServiceRegistryPerformanceTest::TestUnregisterServices()
{
  Log() << "Unregister all services, and check that we get #of services ("
        << nServices << ") * #of listeners (" << nListeners
        << ")  SERVICE_UNREGISTERING events\n";

  testing::HighPrecisionTimer t;
  t.Start();
  UnregisterServices();
  long long ms = t.ElapsedMilli();
  Log() << "unregister took " << ms << "ms\n";
  US_TEST_CONDITION_REQUIRED(nServices * listeners.size() == nUnregistering,
                             "# SERVICE_UNREGISTERING events must be same as # "
                             "of (un)registered services * # of listeners");
}

void ServiceRegistryPerformanceTest::UnregisterServices()
{
  Log() << "unregistering " << regs.size()
        << " services, listener count=" << listeners.size() << "\n";
  for (std::size_t i = 0; i < regs.size(); i++) {
    ServiceRegistration<IPerfTestService> reg = regs[i];
    reg.Unregister();
  }
  regs.clear();
}

int ServiceRegistryPerformanceTest(int /*argc*/, char* /*argv*/ [])
{
  US_TEST_BEGIN("ServiceRegistryPerformanceTest")

  FrameworkFactory factory;
  auto framework = factory.NewFramework();
  framework.Start();

  class ServiceRegistryPerformanceTest perfTest(framework.GetBundleContext());
  perfTest.InitTestCase();
  perfTest.TestAddListeners();
  perfTest.TestRegisterServices();
  perfTest.TestModifyServices();
  perfTest.TestUnregisterServices();
  perfTest.CleanupTestCase();

  US_TEST_END()
}

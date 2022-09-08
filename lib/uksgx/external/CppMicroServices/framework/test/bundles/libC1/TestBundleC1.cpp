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

#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/LDAPProp.h"
#include "cppmicroservices/ServiceRegistration.h"
#include "cppmicroservices/ServiceTracker.h"

#include "cppmicroservices/util/String.h"

#include "TestingMacros.h"

#include <condition_variable>
#include <future>
#include <queue>
#include <thread>

namespace cppmicroservices {

class TestBundleC1Activator
  : public BundleActivator
  , public ServiceTrackerCustomizer<void>
{
public:
  TestBundleC1Activator()
    : context()
    , stop(false)
    , count(0)
  {}

  ~TestBundleC1Activator() {}

  void Start(BundleContext context)
  {
    this->context = context;

    cppmicroservices::LDAPFilter filter(
      cppmicroservices::LDAPProp(Constants::OBJECTCLASS) ==
        "org.cppmicroservices.c1.*" &&
      cppmicroservices::LDAPProp("i") == 5);
    tracker.reset(new ServiceTracker<void>(context, filter, this));
    tracker->Open();

    std::promise<void> p;
    std::shared_future<void> f = p.get_future();

    // Start ten threads, each registering ten services
    for (int c = 0; c < 10; ++c) {
      threads.emplace_back(
        &TestBundleC1Activator::RegisterServices, this, f, c);
    }

    // Start a thread that unregisters the services
    unregThread = std::thread(&TestBundleC1Activator::UnRegister, this);

    p.set_value();
  }

  void Stop(BundleContext)
  {
    for (auto& th : threads) {
      th.join();
    }

    stop = true;
    unregThread.join();

    tracker->Close();

    if (count != 0)
      throw std::logic_error("Not unregistered all services");
  }

  void RegisterServices(std::shared_future<void> f, int c)
  {
    f.wait();

    for (int i = 0; i < 10; ++i) {
      // Register a service ten times with different properties
      InterfaceMap im;
      im[std::string("org.cppmicroservices.c1.") + util::ToString(c)] =
        std::make_shared<int>(1);
      ServiceProperties props;
      props["i"] = i;
      auto reg = context.RegisterService(
        std::make_shared<const InterfaceMap>(im), props);
      regs.Lock(), regs.v.emplace_back(std::move(reg));
      count++;
      if (i % 5 == 0)
        regs.NotifyAll();
    }
  }

  void UnRegister()
  {
    while (!(stop.load() && (regs.Lock(), regs.v.empty()))) {
      std::vector<ServiceRegistrationU> currRegs;
      regs.Lock(), std::swap(currRegs, regs.v);
      if (currRegs.empty()) {
        auto l = regs.Lock();
        regs.WaitFor(l, std::chrono::milliseconds(100), [this] {
          return !regs.v.empty();
        });
        std::swap(currRegs, regs.v);
      }

      for (auto& reg : currRegs) {
        if (reg.GetReference().GetProperty("i") == 5) {
          // change a property, unregister later
          ServiceProperties newProps;
          newProps["i"] = 15;
          reg.SetProperties(newProps);
          regs.Lock(), regs.v.push_back(reg);
        } else {
          reg.Unregister();
          count--;
        }
      }
    }
  }

  // -------------------- Service Tracker Customizer -------------------------

  InterfaceMapConstPtr AddingService(const ServiceReferenceU& reference)
  {
    if (reference.GetProperty("i") == 5) {
      auto l = additionalReg.Lock();
      if (!additionalReg.v) {
        InterfaceMap im;
        im[std::string("org.cppmicroservices.c1.additional")] =
          std::make_shared<int>(2);
        additionalReg.v =
          context.RegisterService(std::make_shared<const InterfaceMap>(im));
      }
      return InterfaceMapConstPtr();
    }
    return context.GetService(reference);
  }

  void ModifiedService(const ServiceReferenceU& reference,
                       const InterfaceMapConstPtr& /*service*/)
  {
    if (reference.GetProperty("i") != 5)
      throw std::logic_error("modified end match: wrong property");
    context.GetService(reference);
  }

  void RemovedService(const ServiceReferenceU& /*reference*/,
                      const InterfaceMapConstPtr& /*service*/)
  {
    auto l = additionalReg.Lock();
    if (additionalReg.v)
      additionalReg.v.Unregister();
  }

private:
  BundleContext context;

  std::unique_ptr<ServiceTracker<void>> tracker;

  std::vector<std::thread> threads;
  std::thread unregThread;

  struct
    : detail::MultiThreaded<detail::MutexLockingStrategy<>,
                            detail::WaitCondition>
  {
    std::vector<ServiceRegistrationU> v;
  } regs;

  struct : detail::MultiThreaded<>
  {
    ServiceRegistrationU v;
  } additionalReg;

  std::atomic<bool> stop;
  std::atomic<int> count;
};

} // namespace cppmicroservices

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(
  cppmicroservices::TestBundleC1Activator)

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

#ifndef CPPMICROSERVICES_SERVICEHOOKS_H
#define CPPMICROSERVICES_SERVICEHOOKS_H

#include "cppmicroservices/ServiceTracker.h"
#include "cppmicroservices/detail/WaitCondition.h"

#include "ServiceListeners.h"

namespace cppmicroservices {

struct ServiceListenerHook;

class ServiceHooks
  : private detail::MultiThreaded<>
  , private ServiceTrackerCustomizer<ServiceListenerHook>
{

private:
  CoreBundleContext* coreCtx;
  std::unique_ptr<ServiceTracker<ServiceListenerHook>> listenerHookTracker;

  std::atomic<bool> bOpen;

  virtual std::shared_ptr<ServiceListenerHook> AddingService(
    const ServiceReference<ServiceListenerHook>& reference);
  virtual void ModifiedService(
    const ServiceReference<ServiceListenerHook>& reference,
    const std::shared_ptr<ServiceListenerHook>& service);
  virtual void RemovedService(
    const ServiceReference<ServiceListenerHook>& reference,
    const std::shared_ptr<ServiceListenerHook>& service);

public:
  ServiceHooks(CoreBundleContext* coreCtx);
  ~ServiceHooks();

  void Open();

  void Close();

  bool IsOpen() const;

  void FilterServiceReferences(BundleContextPrivate* context,
                               const std::string& service,
                               const std::string& filter,
                               std::vector<ServiceReferenceBase>& refs);

  void FilterServiceEventReceivers(
    const ServiceEvent& evt,
    ServiceListeners::ServiceListenerEntries& receivers);

  void HandleServiceListenerReg(const ServiceListenerEntry& sle);

  void HandleServiceListenerUnreg(const ServiceListenerEntry& sle);

  void HandleServiceListenerUnreg(const std::vector<ServiceListenerEntry>& set);
};
}

#endif // CPPMICROSERVICES_SERVICEHOOKS_H

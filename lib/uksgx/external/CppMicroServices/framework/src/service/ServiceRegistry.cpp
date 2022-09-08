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

#include "ServiceRegistry.h"

#include "cppmicroservices/PrototypeServiceFactory.h"
#include "cppmicroservices/ServiceFactory.h"

#include "BundlePrivate.h"
#include "CoreBundleContext.h"
#include "ServiceRegistrationBasePrivate.h"

#include <cassert>
#include <iterator>
#include <stdexcept>

namespace cppmicroservices {

void ServiceRegistry::Clear()
{
  auto l = this->Lock();
  US_UNUSED(l);
  services.clear();
  classServices.clear();
  serviceRegistrations.clear();
}

Properties ServiceRegistry::CreateServiceProperties(
  const ServiceProperties& in,
  const std::vector<std::string>& classes,
  bool isFactory,
  bool isPrototypeFactory,
  long sid)
{
  static std::atomic<long> nextServiceID(1);
  ServiceProperties props(in);

  if (!classes.empty()) {
    props.insert(std::make_pair(Constants::OBJECTCLASS, classes));
  }

  props.insert(
    std::make_pair(Constants::SERVICE_ID, sid != -1 ? sid : nextServiceID++));

  if (isPrototypeFactory) {
    props.insert(
      std::make_pair(Constants::SERVICE_SCOPE, Constants::SCOPE_PROTOTYPE));
  } else if (isFactory) {
    props.insert(
      std::make_pair(Constants::SERVICE_SCOPE, Constants::SCOPE_BUNDLE));
  } else {
    props.insert(
      std::make_pair(Constants::SERVICE_SCOPE, Constants::SCOPE_SINGLETON));
  }

  return Properties(props);
}

ServiceRegistry::ServiceRegistry(CoreBundleContext* coreCtx)
  : core(coreCtx)
{}

ServiceRegistrationBase ServiceRegistry::RegisterService(
  BundlePrivate* bundle,
  const InterfaceMapConstPtr& service,
  const ServiceProperties& properties)
{
  if (!service || service->empty()) {
    throw std::invalid_argument(
      "Can't register empty InterfaceMap as a service");
  }

  // Check if we got a service factory
  bool isFactory = service->count("org.cppmicroservices.factory") > 0;
  bool isPrototypeFactory =
    (isFactory
       ? static_cast<bool>(std::dynamic_pointer_cast<PrototypeServiceFactory>(
           std::static_pointer_cast<ServiceFactory>(
             service->find("org.cppmicroservices.factory")->second)))
       : false);

  std::vector<std::string> classes;
  // Check if service implements claimed classes and that they exist.
  for (auto i : *service) {
    if (i.first.empty() || (!isFactory && i.second == nullptr)) {
      throw std::invalid_argument("Can't register as null class");
    }
    classes.push_back(i.first);
  }

  ServiceRegistrationBase res(
    bundle,
    service,
    CreateServiceProperties(
      properties, classes, isFactory, isPrototypeFactory));
  {
    auto l = this->Lock();
    US_UNUSED(l);
    services.insert(std::make_pair(res, classes));
    serviceRegistrations.push_back(res);
    for (auto& clazz : classes) {
      std::vector<ServiceRegistrationBase>& s = classServices[clazz];
      std::vector<ServiceRegistrationBase>::iterator ip =
        std::lower_bound(s.begin(), s.end(), res);
      s.insert(ip, res);
    }
  }

  ServiceReferenceBase r = res.GetReference(std::string());
  ServiceListeners::ServiceListenerEntries listeners;
  ServiceEvent registeredEvent(ServiceEvent::SERVICE_REGISTERED, r);
  bundle->coreCtx->listeners.GetMatchingServiceListeners(registeredEvent,
                                                         listeners);
  bundle->coreCtx->listeners.ServiceChanged(listeners, registeredEvent);
  return res;
}

void ServiceRegistry::UpdateServiceRegistrationOrder(
  const ServiceRegistrationBase& sr,
  const std::vector<std::string>& classes)
{
  auto l = this->Lock();
  US_UNUSED(l);
  for (auto& clazz : classes) {
    std::vector<ServiceRegistrationBase>& s = classServices[clazz];
    s.erase(std::remove(s.begin(), s.end(), sr), s.end());
    s.insert(std::lower_bound(s.begin(), s.end(), sr), sr);
  }
}

void ServiceRegistry::Get(
  const std::string& clazz,
  std::vector<ServiceRegistrationBase>& serviceRegs) const
{
  this->Lock(), Get_unlocked(clazz, serviceRegs);
}

void ServiceRegistry::Get_unlocked(
  const std::string& clazz,
  std::vector<ServiceRegistrationBase>& serviceRegs) const
{
  MapClassServices::const_iterator i = classServices.find(clazz);
  if (i != classServices.end()) {
    serviceRegs = i->second;
  }
}

ServiceReferenceBase ServiceRegistry::Get(BundlePrivate* bundle,
                                          const std::string& clazz) const
{
  auto l = this->Lock();
  US_UNUSED(l);
  try {
    std::vector<ServiceReferenceBase> srs;
    Get_unlocked(clazz, "", bundle, srs);
    DIAG_LOG(*core->sink) << "get service ref " << clazz << " for bundle "
                          << bundle->symbolicName << " = " << srs.size()
                          << " refs";

    if (!srs.empty()) {
      return srs.back();
    }
  } catch (const std::invalid_argument&) {
  }

  return ServiceReferenceBase();
}

void ServiceRegistry::Get(const std::string& clazz,
                          const std::string& filter,
                          BundlePrivate* bundle,
                          std::vector<ServiceReferenceBase>& res) const
{
  this->Lock(), Get_unlocked(clazz, filter, bundle, res);
}

void ServiceRegistry::Get_unlocked(const std::string& clazz,
                                   const std::string& filter,
                                   BundlePrivate* bundle,
                                   std::vector<ServiceReferenceBase>& res) const
{
  std::vector<ServiceRegistrationBase>::const_iterator s;
  std::vector<ServiceRegistrationBase>::const_iterator send;
  std::vector<ServiceRegistrationBase> v;
  LDAPExpr ldap;
  if (clazz.empty()) {
    if (!filter.empty()) {
      ldap = LDAPExpr(filter);
      LDAPExpr::ObjectClassSet matched;
      if (ldap.GetMatchedObjectClasses(matched)) {
        v.clear();
        for (auto& className : matched) {
          auto i = classServices.find(className);
          if (i != classServices.end()) {
            std::copy(
              i->second.begin(), i->second.end(), std::back_inserter(v));
          }
        }
        if (!v.empty()) {
          s = v.begin();
          send = v.end();
        } else {
          return;
        }
      } else {
        s = serviceRegistrations.begin();
        send = serviceRegistrations.end();
      }
    } else {
      s = serviceRegistrations.begin();
      send = serviceRegistrations.end();
    }
  } else {
    MapClassServices::const_iterator it = classServices.find(clazz);
    if (it != classServices.end()) {
      s = it->second.begin();
      send = it->second.end();
    } else {
      return;
    }
    if (!filter.empty()) {
      ldap = LDAPExpr(filter);
    }
  }

  for (; s != send; ++s) {
    ServiceReferenceBase sri = s->GetReference(clazz);

    if (filter.empty() ||
        ldap.Evaluate(PropertiesHandle(s->d->properties, true), false)) {
      res.push_back(sri);
    }
  }

  if (!res.empty()) {
    if (bundle != nullptr) {
      auto ctx = bundle->bundleContext.Load();
      core->serviceHooks.FilterServiceReferences(ctx.get(), clazz, filter, res);
    } else {
      core->serviceHooks.FilterServiceReferences(nullptr, clazz, filter, res);
    }
  }
}

void ServiceRegistry::RemoveServiceRegistration(
  const ServiceRegistrationBase& sr)
{
  auto l = this->Lock();
  US_UNUSED(l);
  RemoveServiceRegistration_unlocked(sr);
}

void ServiceRegistry::RemoveServiceRegistration_unlocked(
  const ServiceRegistrationBase& sr)
{
  std::vector<std::string> classes;
  {
    auto l2 = sr.d->properties.Lock();
    US_UNUSED(l2);
    assert(sr.d->properties.Value_unlocked(Constants::OBJECTCLASS).Type() ==
           typeid(std::vector<std::string>));
    classes = ref_any_cast<std::vector<std::string>>(
      sr.d->properties.Value_unlocked(Constants::OBJECTCLASS));
  }
  services.erase(sr);
  serviceRegistrations.erase(
    std::remove(serviceRegistrations.begin(), serviceRegistrations.end(), sr),
    serviceRegistrations.end());
  for (auto& clazz : classes) {
    std::vector<ServiceRegistrationBase>& s = classServices[clazz];
    if (s.size() > 1) {
      s.erase(std::remove(s.begin(), s.end(), sr), s.end());
    } else {
      classServices.erase(clazz);
    }
  }
}

void ServiceRegistry::GetRegisteredByBundle(
  BundlePrivate* p,
  std::vector<ServiceRegistrationBase>& res) const
{
  auto l = this->Lock();
  US_UNUSED(l);

  for (auto& sr : serviceRegistrations) {
    if (sr.d->bundle == p) {
      res.push_back(sr);
    }
  }
}

void ServiceRegistry::GetUsedByBundle(
  BundlePrivate* bundle,
  std::vector<ServiceRegistrationBase>& res) const
{
  auto l = this->Lock();
  US_UNUSED(l);

  for (std::vector<ServiceRegistrationBase>::const_iterator i =
         serviceRegistrations.begin();
       i != serviceRegistrations.end();
       ++i) {
    if (i->d->IsUsedByBundle(bundle)) {
      res.push_back(*i);
    }
  }
}
}

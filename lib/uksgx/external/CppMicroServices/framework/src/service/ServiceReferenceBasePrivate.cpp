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

#include "ServiceReferenceBasePrivate.h"

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/ServiceException.h"
#include "cppmicroservices/ServiceFactory.h"

#include "BundlePrivate.h"
#include "CoreBundleContext.h"
#include "ServiceRegistrationBasePrivate.h"
#include "ServiceRegistry.h"

#include <cassert>

US_MSVC_DISABLE_WARNING(
  4503) // decorated name length exceeded, name was truncated

namespace cppmicroservices {

typedef std::unordered_map<BundlePrivate*,
                           std::unordered_set<ServiceRegistrationBasePrivate*>>
  ThreadMarksMapType;

ServiceReferenceBasePrivate::ServiceReferenceBasePrivate(
  ServiceRegistrationBasePrivate* reg)
  : ref(1)
  , registration(reg)
{
  if (registration)
    ++registration->ref;
}

ServiceReferenceBasePrivate::~ServiceReferenceBasePrivate()
{
  if (registration && !--registration->ref) {
    delete registration;
  }
}

InterfaceMapConstPtr ServiceReferenceBasePrivate::GetServiceFromFactory(
  BundlePrivate* bundle,
  const std::shared_ptr<ServiceFactory>& factory)
{
  assert(factory && "Factory service pointer is nullptr");
  InterfaceMapConstPtr s;
  try {
    InterfaceMapConstPtr smap =
      factory->GetService(MakeBundle(bundle->shared_from_this()),
                          ServiceRegistrationBase(registration));
    if (!smap || smap->empty()) {
      registration->bundle->coreCtx->listeners.SendFrameworkEvent(
        FrameworkEvent(
          FrameworkEvent::Type::FRAMEWORK_WARNING,
          MakeBundle(bundle->shared_from_this()),
          std::string(
            "ServiceFactory returned an empty or nullptr interface map."),
          std::make_exception_ptr(std::logic_error(
            "ServiceFactory returned an invalid interface map"))));
      return smap;
    }
    std::vector<std::string> classes =
      (registration->properties.Lock(),
       any_cast<std::vector<std::string>>(
         registration->properties.Value_unlocked(Constants::OBJECTCLASS)));
    for (auto clazz : classes) {
      if (smap->find(clazz) == smap->end() &&
          clazz != "org.cppmicroservices.factory") {
        std::string message(
          "ServiceFactory produced an object that did not implement: " + clazz);
        registration->bundle->coreCtx->listeners.SendFrameworkEvent(
          FrameworkEvent(
            FrameworkEvent::Type::FRAMEWORK_WARNING,
            MakeBundle(bundle->shared_from_this()),
            message,
            std::make_exception_ptr(std::logic_error(message.c_str()))));
        return nullptr;
      }
    }
    s = smap;
  } catch (...) {
    s.reset();
    registration->bundle->coreCtx->listeners.SendFrameworkEvent(
      FrameworkEvent(FrameworkEvent::Type::FRAMEWORK_ERROR,
                     MakeBundle(bundle->shared_from_this()),
                     std::string("ServiceFactory threw an unknown exception."),
                     std::current_exception()));
  }
  return s;
}

InterfaceMapConstPtr ServiceReferenceBasePrivate::GetPrototypeService(
  const Bundle& bundle)
{
  InterfaceMapConstPtr s;
  {
    if (registration->available) {
      auto factory = std::static_pointer_cast<ServiceFactory>(
        registration->GetService("org.cppmicroservices.factory"));
      s = GetServiceFromFactory(GetPrivate(bundle).get(), factory);
      registration->Lock(),
        registration->prototypeServiceInstances[GetPrivate(bundle).get()]
          .push_back(s);
    }
  }
  return s;
}

std::shared_ptr<void> ServiceReferenceBasePrivate::GetService(
  BundlePrivate* bundle)
{
  return ExtractInterface(GetServiceInterfaceMap(bundle), interfaceId);
}

InterfaceMapConstPtr ServiceReferenceBasePrivate::GetServiceInterfaceMap(
  BundlePrivate* bundle)
{
  /*
   * Detect recursive service factory calls. For each thread, this
   * map contains an entry for each bundle that tries to get a service
   * from a service factory.
   */
#ifdef US_HAVE_THREAD_LOCAL
  static thread_local ThreadMarksMapType threadMarks;
#elif !defined(US_ENABLE_THREADING_SUPPORT)
  static ThreadMarksMapType threadMarks;
#else
  // A non-static map will never lead to the detection of
  // recursive service factory calls.
  ThreadMarksMapType threadMarks;
#endif

  InterfaceMapConstPtr s;
  if (!registration->available)
    return s;
  std::shared_ptr<ServiceFactory> serviceFactory;

  std::unordered_set<ServiceRegistrationBasePrivate*>* marks = nullptr;

  struct Unmark
  {
    ~Unmark()
    {
      if (s)
        s->erase(r);
    }
    std::unordered_set<ServiceRegistrationBasePrivate*>*& s;
    ServiceRegistrationBasePrivate* r;
  } unmark{ marks, registration };
  US_UNUSED(unmark);

  {
    auto l = registration->Lock();
    US_UNUSED(l);
    if (!registration->available)
      return s;
    serviceFactory = std::static_pointer_cast<ServiceFactory>(
      registration->GetService_unlocked("org.cppmicroservices.factory"));

    auto res = registration->dependents.insert(std::make_pair(bundle, 0));
    auto& depCounter = res.first->second;

    // No service factory, just return the registered service directly.
    if (!serviceFactory) {
      s = registration->service;
      if (s && !s->empty()) {
        ++depCounter;
      }
      return s;
    }

    auto serviceIter = registration->bundleServiceInstance.find(bundle);
    if (registration->bundleServiceInstance.end() != serviceIter) {
      ++depCounter;
      return serviceIter->second;
    }

    marks = &threadMarks[bundle];
    if (marks->find(registration) != marks->end()) {
      // Prevent recursive service factory calls from the same thread
      // for the same bundle.
      std::string msg = "Recursive call to ServiceFactory::GetService";
      auto fwEvent =
        FrameworkEvent(FrameworkEvent::Type::FRAMEWORK_ERROR,
                       MakeBundle(bundle->shared_from_this()),
                       msg,
                       std::make_exception_ptr(ServiceException(
                         msg, ServiceException::FACTORY_RECURSION)));

      registration->bundle->coreCtx->listeners.SendFrameworkEvent(fwEvent);
      return nullptr;
    }

    marks->insert(registration);
  }

  // Calling into a service factory could cause re-entrancy into the
  // framework and even, theoretically, into this function. Ensuring
  // we don't hold a lock while calling into the service factory eliminates
  // the possibility of a deadlock. It does not however eliminate the
  // possibility of infinite recursion.
  s = GetServiceFromFactory(bundle, serviceFactory);

  auto l = registration->Lock();
  US_UNUSED(l);

  registration->dependents.insert(std::make_pair(bundle, 0));

  if (s && !s->empty()) {
    // Insert a cached service object instance only if one isn't already cached. If another thread
    // already inserted a cached service object, discard the service object returned by
    // GetServiceFromFactory and return the cached one.
    auto insertResultPair =
      registration->bundleServiceInstance.insert(std::make_pair(bundle, s));
    s = insertResultPair.first->second;
    ++registration->dependents.at(bundle);
  } else {
    // If the service factory returned an invalid service object check the cache and return a valid one
    // if it exists.
    if (registration->bundleServiceInstance.end() !=
        registration->bundleServiceInstance.find(bundle)) {
      s = registration->bundleServiceInstance.at(bundle);
      ++registration->dependents.at(bundle);
    }
  }
  return s;
}

bool ServiceReferenceBasePrivate::UngetPrototypeService(
  const std::shared_ptr<BundlePrivate>& bundle,
  const InterfaceMapConstPtr& service)
{
  std::list<InterfaceMapConstPtr> prototypeServiceMaps;
  std::shared_ptr<ServiceFactory> sf;

  {
    auto l = registration->Lock();
    US_UNUSED(l);
    auto iter = registration->prototypeServiceInstances.find(bundle.get());
    if (iter == registration->prototypeServiceInstances.end()) {
      return false;
    }

    prototypeServiceMaps = iter->second;
    sf = std::static_pointer_cast<ServiceFactory>(
      registration->GetService_unlocked("org.cppmicroservices.factory"));
  }

  if (!sf)
    return false;

  for (auto imIter = prototypeServiceMaps.begin();
       imIter != prototypeServiceMaps.end();
       ++imIter) {
    // compare the contents of the map
    if (*service.get() == *imIter->get()) {
      try {
        sf->UngetService(
          MakeBundle(bundle), ServiceRegistrationBase(registration), service);
      } catch (...) {
        std::string message("ServiceFactory threw an exception");
        registration->bundle->coreCtx->listeners.SendFrameworkEvent(
          FrameworkEvent(FrameworkEvent::Type::FRAMEWORK_WARNING,
                         MakeBundle(bundle->shared_from_this()),
                         message,
                         std::current_exception()));
      }

      auto l = registration->Lock();
      US_UNUSED(l);
      auto iter = registration->prototypeServiceInstances.find(bundle.get());
      if (iter == registration->prototypeServiceInstances.end())
        return true;

      auto serviceIter =
        std::find(iter->second.begin(), iter->second.end(), service);
      if (serviceIter != iter->second.end())
        iter->second.erase(serviceIter);
      if (iter->second.empty()) {
        registration->prototypeServiceInstances.erase(iter);
      }
      return true;
    }
  }

  return false;
}

bool ServiceReferenceBasePrivate::UngetService(
  const std::shared_ptr<BundlePrivate>& bundle,
  bool checkRefCounter)
{
  bool hadReferences = false;
  bool removeService = false;
  InterfaceMapConstPtr sfi;
  std::shared_ptr<ServiceFactory> sf;

  {
    auto l = registration->Lock();
    US_UNUSED(l);
    auto depIter = registration->dependents.find(bundle.get());
    if (registration->dependents.end() == depIter) {
      return hadReferences && removeService;
    }

    int& count = depIter->second;
    if (count > 0) {
      hadReferences = true;
    }

    if (checkRefCounter) {
      if (count > 1) {
        --count;
      } else if (count == 1) {
        removeService = true;
      }
    } else {
      removeService = true;
    }

    if (removeService) {
      auto serviceIter = registration->bundleServiceInstance.find(bundle.get());
      if (registration->bundleServiceInstance.end() != serviceIter) {
        sfi = serviceIter->second;
      }

      if (sfi && !sfi->empty()) {
        sf = std::static_pointer_cast<ServiceFactory>(
          registration->GetService_unlocked("org.cppmicroservices.factory"));
      }
      registration->bundleServiceInstance.erase(bundle.get());
      registration->dependents.erase(bundle.get());
    }
  }

  if (sf && sfi && !sfi->empty()) {
    try {
      sf->UngetService(
        MakeBundle(bundle), ServiceRegistrationBase(registration), sfi);
    } catch (...) {
      std::string message("ServiceFactory threw an exception");
      registration->bundle->coreCtx->listeners.SendFrameworkEvent(
        FrameworkEvent(FrameworkEvent::Type::FRAMEWORK_WARNING,
                       MakeBundle(bundle->shared_from_this()),
                       message,
                       std::current_exception()));
    }
  }

  return hadReferences && removeService;
}

PropertiesHandle ServiceReferenceBasePrivate::GetProperties() const
{
  return PropertiesHandle(registration->properties, true);
}

bool ServiceReferenceBasePrivate::IsConvertibleTo(
  const std::string& interfaceId) const
{
  if (registration) {
    auto l = registration->Lock();
    US_UNUSED(l);
    return registration->service ? registration->service->find(interfaceId) !=
                                     registration->service->end()
                                 : false;
  }
  return false;
}
}

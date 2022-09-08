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

#include "cppmicroservices/ServiceObjects.h"

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/Constants.h"
#include "cppmicroservices/detail/Log.h"
#include "cppmicroservices/util/Error.h"

#include "BundleContextPrivate.h"
#include "BundlePrivate.h"
#include "CoreBundleContext.h"
#include "ServiceReferenceBasePrivate.h"

#include <set>

namespace cppmicroservices {

class ServiceObjectsBasePrivate
{
public:
  std::shared_ptr<BundleContextPrivate> m_context;
  ServiceReferenceBase m_reference;

  ServiceObjectsBasePrivate(
    const std::shared_ptr<BundleContextPrivate>& context,
    const ServiceReferenceBase& reference)
    : m_context(context)
    , m_reference(reference)
  {}

  InterfaceMapConstPtr GetServiceInterfaceMap()
  {
    InterfaceMapConstPtr result;

    bool isPrototypeScope =
      m_reference.GetProperty(Constants::SERVICE_SCOPE).ToString() ==
      Constants::SCOPE_PROTOTYPE;

    if (isPrototypeScope) {
      result = m_reference.d.load()->GetPrototypeService(
        MakeBundleContext(m_context).GetBundle());
    } else {
      result = m_reference.d.load()->GetServiceInterfaceMap(
        GetPrivate(MakeBundleContext(m_context).GetBundle()).get());
    }

    return result;
  }
};

ServiceObjectsBase::ServiceObjectsBase(
  const std::shared_ptr<BundleContextPrivate>& context,
  const ServiceReferenceBase& reference)
  : d(new ServiceObjectsBasePrivate(context, reference))
{
  if (!reference) {
    throw std::invalid_argument("The service reference is invalid");
  }
}

/* @brief Private helper struct used to facilitate the shared_ptr aliasing constructor
 *        in ServiceObjectsBase::GetService & ServiceObjectsBase::GetServiceInterfaceMap
 *        methods. The aliasing constructor helps automate the call to UngetService method.
 *
 *        Service consumers can simply call GetService to obtain a shared_ptr to the
 *        service object and not worry about calling UngetService when they are done.
 *        The UngetService is called when all instances of the returned shared_ptr object
 *        go out of scope.
 */
struct UngetHelper
{
  const InterfaceMapConstPtr interfaceMap;
  const ServiceReferenceBase sref;
  const std::weak_ptr<BundlePrivate> b;

  UngetHelper(const InterfaceMapConstPtr& im,
              const ServiceReferenceBase& sr,
              const std::shared_ptr<BundlePrivate>& b)
    : interfaceMap(im)
    , sref(sr)
    , b(b)
  {}
  ~UngetHelper()
  {
    try {
      auto bundle = b.lock();
      if (sref) {
        bool isPrototypeScope =
          sref.GetProperty(Constants::SERVICE_SCOPE).ToString() ==
          Constants::SCOPE_PROTOTYPE;

        if (isPrototypeScope) {
          sref.d.load()->UngetPrototypeService(bundle, interfaceMap);
        } else {
          sref.d.load()->UngetService(bundle, true);
        }
      }
    } catch (...) {
      // Make sure that we don't crash if the shared_ptr service object outlives
      // the BundlePrivate or CoreBundleContext objects.
      if (!b.expired()) {
        DIAG_LOG(*b.lock()->coreCtx->sink)
          << "UngetHelper threw an exception. " << util::GetLastExceptionStr();
      }
      // don't throw exceptions from the destructor. For an explanation, see:
      // https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md
      // Following this rule means that a FrameworkEvent isn't an option here
      // since it contains an exception object which clients could throw.
    }
  }
};

std::shared_ptr<void> ServiceObjectsBase::GetService() const
{
  if (!d->m_reference) {
    return nullptr;
  }

  std::shared_ptr<UngetHelper> h(
    new UngetHelper(d->GetServiceInterfaceMap(),
                    d->m_reference,
                    d->m_context->bundle->shared_from_this()));
  return std::shared_ptr<void>(
    h, (h->interfaceMap->find(d->m_reference.GetInterfaceId()))->second.get());
}

InterfaceMapConstPtr ServiceObjectsBase::GetServiceInterfaceMap() const
{
  InterfaceMapConstPtr result;
  if (!d->m_reference) {
    return result;
  }
  // copy construct a new map to be handed out to consumers
  result =
    std::make_shared<const InterfaceMap>(*(d->GetServiceInterfaceMap().get()));
  std::shared_ptr<UngetHelper> h(new UngetHelper{
    result, d->m_reference, d->m_context->bundle->shared_from_this() });
  return InterfaceMapConstPtr(h, h->interfaceMap.get());
}

ServiceReferenceBase ServiceObjectsBase::GetReference() const
{
  return d->m_reference;
}

ServiceObjectsBase::ServiceObjectsBase(ServiceObjectsBase&& other)
  : d(std::move(other.d))
{}

ServiceObjectsBase::~ServiceObjectsBase() {}

ServiceObjectsBase& ServiceObjectsBase::operator=(ServiceObjectsBase&& other)
{
  d = std::move(other.d);
  return *this;
}

ServiceObjects<void>::ServiceObjects(ServiceObjects&& other)
  : ServiceObjectsBase(std::move(other))
{}

ServiceObjects<void>& ServiceObjects<void>::operator=(ServiceObjects&& other)
{
  ServiceObjectsBase::operator=(std::move(other));
  return *this;
}

InterfaceMapConstPtr ServiceObjects<void>::GetService() const
{
  return this->ServiceObjectsBase::GetServiceInterfaceMap();
}

ServiceReferenceU ServiceObjects<void>::GetServiceReference() const
{
  return this->ServiceObjectsBase::GetReference();
}

ServiceObjects<void>::ServiceObjects(
  const std::shared_ptr<BundleContextPrivate>& context,
  const ServiceReferenceU& reference)
  : ServiceObjectsBase(context, reference)
{}
}

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

#include "cppmicroservices/BundleEvent.h"

#include "cppmicroservices/Bundle.h"

#include <stdexcept>

namespace cppmicroservices {

class BundleEventData
{
public:
  BundleEventData(BundleEvent::Type type,
                  const Bundle& bundle,
                  const Bundle& origin)
    : type(type)
    , bundle(bundle)
    , origin(origin)
  {
    if (!bundle)
      throw std::invalid_argument("invalid bundle");
    if (!origin)
      throw std::invalid_argument("invalid origin");
  }

  const BundleEvent::Type type;

  // Bundle that had a change occur in its lifecycle.
  Bundle bundle;

  // Bundle that was the origin of the event. For install event type, this is
  // the bundle whose context was used to install the bundle. Otherwise it is
  // the bundle itself.
  Bundle origin;
};

BundleEvent::BundleEvent()
  : d(nullptr)
{}

BundleEvent::operator bool() const
{
  return d.operator bool();
}

BundleEvent::BundleEvent(Type type, const Bundle& bundle)
  : d(new BundleEventData(type, bundle, bundle))
{}

BundleEvent::BundleEvent(Type type, const Bundle& bundle, const Bundle& origin)
  : d(new BundleEventData(type, bundle, origin))
{}

Bundle BundleEvent::GetBundle() const
{
  if (!d)
    return Bundle{};
  return d->bundle;
}

BundleEvent::Type BundleEvent::GetType() const
{
  if (!d)
    return BundleEvent::Type::BUNDLE_UNINSTALLED;
  return d->type;
}

Bundle BundleEvent::GetOrigin() const
{
  if (!d)
    return Bundle{};
  return d->origin;
}

bool BundleEvent::operator==(const BundleEvent& evt) const
{
  if (!(*this) && !evt)
    return true;
  if (!(*this) || !evt)
    return false;
  return GetType() == evt.GetType() && GetBundle() == evt.GetBundle() &&
         GetOrigin() == evt.GetOrigin();
}

std::ostream& operator<<(std::ostream& os, BundleEvent::Type eventType)
{
  switch (eventType) {
    case BundleEvent::BUNDLE_STARTED:
      return os << "STARTED";
    case BundleEvent::BUNDLE_STOPPED:
      return os << "STOPPED";
    case BundleEvent::BUNDLE_STARTING:
      return os << "STARTING";
    case BundleEvent::BUNDLE_STOPPING:
      return os << "STOPPING";
    case BundleEvent::BUNDLE_INSTALLED:
      return os << "INSTALLED";
    case BundleEvent::BUNDLE_UNINSTALLED:
      return os << "UNINSTALLED";
    case BundleEvent::BUNDLE_RESOLVED:
      return os << "RESOLVED";
    case BundleEvent::BUNDLE_UNRESOLVED:
      return os << "UNRESOLVED";
    case BundleEvent::BUNDLE_LAZY_ACTIVATION:
      return os << "LAZY_ACTIVATION";

    default:
      return os << "Unknown bundle event type (" << static_cast<int>(eventType)
                << ")";
  }
}

std::ostream& operator<<(std::ostream& os, const BundleEvent& event)
{
  if (!event)
    return os << "NONE";

  auto m = event.GetBundle();
  os << event.GetType() << " #" << m.GetBundleId() << " ("
     << m.GetSymbolicName() << " at " << m.GetLocation() << ")";
  return os;
}
}

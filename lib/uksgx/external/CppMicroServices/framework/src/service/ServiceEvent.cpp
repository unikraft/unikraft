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

#include "cppmicroservices/ServiceEvent.h"

#include "cppmicroservices/Constants.h"

namespace cppmicroservices {

class ServiceEventData
{
public:
  ServiceEventData(const ServiceEvent::Type& type,
                   const ServiceReferenceBase& reference)
    : type(type)
    , reference(reference)
  {}

  const ServiceEvent::Type type;
  const ServiceReferenceBase reference;
};

ServiceEvent::ServiceEvent()
  : d(nullptr)
{}

ServiceEvent::operator bool() const
{
  return d.operator bool();
}

ServiceEvent::ServiceEvent(Type type, const ServiceReferenceBase& reference)
  : d(new ServiceEventData(type, reference))
{}

ServiceEvent::ServiceEvent(const ServiceEvent& other)
  : d(other.d)
{}

ServiceEvent& ServiceEvent::operator=(const ServiceEvent& other)
{
  d = other.d;
  return *this;
}

ServiceReferenceU ServiceEvent::GetServiceReference() const
{
  return d->reference;
}

ServiceEvent::Type ServiceEvent::GetType() const
{
  return d->type;
}

std::ostream& operator<<(std::ostream& os, const ServiceEvent::Type& type)
{
  switch (type) {
    case ServiceEvent::SERVICE_MODIFIED:
      return os << "MODIFIED";
    case ServiceEvent::SERVICE_MODIFIED_ENDMATCH:
      return os << "MODIFIED_ENDMATCH";
    case ServiceEvent::SERVICE_REGISTERED:
      return os << "REGISTERED";
    case ServiceEvent::SERVICE_UNREGISTERING:
      return os << "UNREGISTERING";

    default:
      return os << "unknown service event type (" << static_cast<int>(type)
                << ")";
  }
}

std::ostream& operator<<(std::ostream& os, const ServiceEvent& event)
{
  if (!event)
    return os << "NONE";

  os << event.GetType();

  ServiceReferenceU sr = event.GetServiceReference();
  if (sr) {
    // Some events will not have a service reference
    long int sid = any_cast<long int>(sr.GetProperty(Constants::SERVICE_ID));
    os << " " << sid;

    Any classes = sr.GetProperty(Constants::OBJECTCLASS);
    os << " objectClass=" << classes.ToString() << ")";
  }

  return os;
}
}

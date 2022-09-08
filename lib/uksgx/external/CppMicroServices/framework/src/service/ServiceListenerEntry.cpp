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

#include "cppmicroservices/GlobalConfig.h"

US_MSVC_PUSH_DISABLE_WARNING(
  4180) // qualifier applied to function type has no meaning; ignored

#include "ServiceListenerEntry.h"

#include "ServiceListenerHookPrivate.h"

#include <cassert>

namespace cppmicroservices {

struct ServiceListenerCompare
  : std::binary_function<ServiceListener, ServiceListener, bool>
{
  bool operator()(const ServiceListener& f1, const ServiceListener& f2) const
  {
    return f1.target<void(const ServiceEvent&)>() ==
           f2.target<void(const ServiceEvent&)>();
  }
};

class ServiceListenerEntryData : public ServiceListenerHook::ListenerInfoData
{
public:
  ServiceListenerEntryData(const ServiceListenerEntryData&) = delete;
  ServiceListenerEntryData& operator=(const ServiceListenerEntryData&) = delete;

  ServiceListenerEntryData(const std::shared_ptr<BundleContextPrivate>& context,
                           const ServiceListener& l,
                           void* data,
                           ListenerTokenId tokenId,
                           const std::string& filter)
    : ServiceListenerHook::ListenerInfoData(context, l, data, tokenId, filter)
    , ldap()
    , hashValue(0)
  {
    if (!filter.empty()) {
      ldap = LDAPExpr(filter);
    }
  }

  ~ServiceListenerEntryData() {}

  LDAPExpr ldap;

  /**
   * The elements of "simple" filters are cached, for easy lookup.
   *
   * The grammar for simple filters is as follows:
   *
   * <pre>
   * Simple = '(' attr '=' value ')'
   *        | '(' '|' Simple+ ')'
   * </pre>
   * where <code>attr</code> is one of Constants#OBJECTCLASS,
   * Constants#SERVICE_ID or Constants#SERVICE_PID, and
   * <code>value</code> must not contain a wildcard character.
   * <p>
   * The index of the vector determines which key the cache is for
   * (see ServiceListenerState#hashedKeys). For each key, there is
   * a vector pointing out the values which are accepted by this
   * ServiceListenerEntry's filter. This cache is maintained to make
   * it easy to remove this service listener.
   */
  LDAPExpr::LocalCache local_cache;

  std::size_t hashValue;
};

ServiceListenerEntry::ServiceListenerEntry() {}

ServiceListenerEntry::ServiceListenerEntry(const ServiceListenerEntry& other)
  : ServiceListenerHook::ListenerInfo(other)
{}

ServiceListenerEntry::ServiceListenerEntry(
  const ServiceListenerHook::ListenerInfo& info)
  : ServiceListenerHook::ListenerInfo(info)
{
  assert(info.d);
}

ServiceListenerEntry::~ServiceListenerEntry() {}

ServiceListenerEntry& ServiceListenerEntry::operator=(
  const ServiceListenerEntry& other)
{
  d = other.d;
  return *this;
}

void ServiceListenerEntry::SetRemoved(bool removed) const
{
  d->bRemoved = removed;
}

ServiceListenerEntry::ServiceListenerEntry(
  const std::shared_ptr<BundleContextPrivate>& context,
  const ServiceListener& l,
  void* data,
  ListenerTokenId tokenId,
  const std::string& filter)
  : ServiceListenerHook::ListenerInfo(
      new ServiceListenerEntryData(context, l, data, tokenId, filter))
{}

const LDAPExpr& ServiceListenerEntry::GetLDAPExpr() const
{
  return static_cast<ServiceListenerEntryData*>(d.Data())->ldap;
}

LDAPExpr::LocalCache& ServiceListenerEntry::GetLocalCache() const
{
  return static_cast<ServiceListenerEntryData*>(d.Data())->local_cache;
}

void ServiceListenerEntry::CallDelegate(const ServiceEvent& event) const
{
  d->listener(event);
}

bool ServiceListenerEntry::operator==(const ServiceListenerEntry& other) const
{
  return ((d->context == nullptr || other.d->context == nullptr) ||
          d->context == other.d->context) &&
         (d->data == other.d->data) && (d->tokenId == other.d->tokenId) &&
         ServiceListenerCompare()(d->listener, other.d->listener);
}

bool ServiceListenerEntry::Contains(
  const std::shared_ptr<BundleContextPrivate>& context,
  ListenerTokenId tokenId) const
{
  return (d->context == context) && (d->tokenId == tokenId);
}

bool ServiceListenerEntry::Contains(
  const std::shared_ptr<BundleContextPrivate>& context,
  const ServiceListener& listener,
  void* data) const
{
  return (d->context == context) && (d->data == data) &&
         ServiceListenerCompare()(d->listener, listener);
}

ListenerTokenId ServiceListenerEntry::Id() const
{
  return d->tokenId;
}

std::size_t ServiceListenerEntry::Hash() const
{
  using namespace std;

  if (static_cast<ServiceListenerEntryData*>(d.Data())->hashValue == 0) {
    static_cast<ServiceListenerEntryData*>(d.Data())->hashValue =
      ((hash<BundleContextPrivate*>()(d->context.get()) ^
        (hash<void*>()(d->data) << 1)) >>
       1) ^
      ((hash<ServiceListener>()(d->listener)) ^
       (hash<ListenerTokenId>()(d->tokenId) << 1) << 1);
  }
  return static_cast<ServiceListenerEntryData*>(d.Data())->hashValue;
}
}

US_MSVC_POP_WARNING

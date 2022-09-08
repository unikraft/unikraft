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

#ifndef CPPMICROSERVICES_SERVICELISTENERENTRY_H
#define CPPMICROSERVICES_SERVICELISTENERENTRY_H

#include "cppmicroservices/ListenerFunctors.h"
#include "cppmicroservices/ListenerToken.h"
#include "cppmicroservices/ServiceListenerHook.h"

#include "LDAPExpr.h"
#include "Utils.h"

namespace cppmicroservices {

class BundleContextPrivate;
class ServiceListenerEntryData;

/**
 * Data structure for saving service listener info. Contains
 * the optional service listener filter, in addition to the info
 * in ListenerEntry.
 */
class ServiceListenerEntry : public ServiceListenerHook::ListenerInfo
{

public:
  ServiceListenerEntry();
  ServiceListenerEntry(const ServiceListenerEntry& other);
  ServiceListenerEntry(const ServiceListenerHook::ListenerInfo& info);

  ~ServiceListenerEntry();
  ServiceListenerEntry& operator=(const ServiceListenerEntry& other);

  void SetRemoved(bool removed) const;

  ServiceListenerEntry(const std::shared_ptr<BundleContextPrivate>& context,
                       const ServiceListener& l,
                       void* data,
                       ListenerTokenId tokenId,
                       const std::string& filter = "");

  const LDAPExpr& GetLDAPExpr() const;

  LDAPExpr::LocalCache& GetLocalCache() const;

  void CallDelegate(const ServiceEvent& event) const;

  bool operator==(const ServiceListenerEntry& other) const;

  bool Contains(const std::shared_ptr<BundleContextPrivate>& context,
                ListenerTokenId tokenId) const;

  bool Contains(const std::shared_ptr<BundleContextPrivate>& context,
                const ServiceListener& listener,
                void* data) const;

  ListenerTokenId Id() const;

  std::size_t Hash() const;
};
}

US_HASH_FUNCTION_BEGIN(cppmicroservices::ServiceListenerEntry)
return arg.Hash();
US_HASH_FUNCTION_END

#endif // CPPMICROSERVICES_SERVICELISTENERENTRY_H

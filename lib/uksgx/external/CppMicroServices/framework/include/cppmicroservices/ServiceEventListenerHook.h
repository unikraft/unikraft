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

#ifndef CPPMICROSERVICES_SERVICEEVENTLISTENERHOOK_H
#define CPPMICROSERVICES_SERVICEEVENTLISTENERHOOK_H

#include "cppmicroservices/ServiceInterface.h"
#include "cppmicroservices/ServiceListenerHook.h"
#include "cppmicroservices/ShrinkableMap.h"
#include "cppmicroservices/ShrinkableVector.h"

namespace cppmicroservices {

class BundleContext;
class ServiceEvent;

/**
 * @ingroup MicroServices
 *
 * Service Event Listener Hook Service.
 *
 * <p>
 * Bundles registering this service will be called during service
 * (register, modify, and unregister service) operations.
 *
 * @remarks Implementations of this interface are required to be thread-safe.
 */
struct US_Framework_EXPORT ServiceEventListenerHook
{
  /**
   * ShrinkableMap type for filtering event listeners.
   */
  typedef ShrinkableMap<BundleContext,
                        ShrinkableVector<ServiceListenerHook::ListenerInfo>>
    ShrinkableMapType;

  virtual ~ServiceEventListenerHook();

  /**
   * Event listener hook method. This method is called prior to service event
   * delivery when a publishing bundle registers, modifies or unregisters a
   * service. This method can filter the listeners which receive the event.
   *
   * @param event The service event to be delivered.
   * @param listeners A map of Bundle Contexts to a list of Listener
   *        Infos for the bundle's listeners to which the specified event will
   *        be delivered. The implementation of this method may remove bundle
   *        contexts from the map and listener infos from the list
   *        values to prevent the event from being delivered to the associated
   *        listeners.
   */
  virtual void Event(const ServiceEvent& event,
                     ShrinkableMapType& listeners) = 0;
};
}

#endif // CPPMICROSERVICES_SERVICEEVENTLISTENERHOOK_H

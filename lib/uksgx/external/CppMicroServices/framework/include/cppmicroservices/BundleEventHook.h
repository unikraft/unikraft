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

#ifndef CPPMICROSERVICES_BUNDLEEVENTHOOK_H
#define CPPMICROSERVICES_BUNDLEEVENTHOOK_H

#include "cppmicroservices/ServiceInterface.h"
#include "cppmicroservices/ShrinkableVector.h"

namespace cppmicroservices {

class BundleContext;
class BundleEvent;

/**
 * @ingroup MicroServices
 *
 * %Bundle Event Hook Service.
 *
 * <p>
 * Bundles registering this service will be called during bundle lifecycle
 * (installed, starting, started, stopping, stopped, uninstalled) operations.
 *
 * @remarks Implementations of this interface are required to be thread-safe.
 */
struct US_Framework_EXPORT BundleEventHook
{

  virtual ~BundleEventHook();

  /**
   * Bundle event hook method. This method is called prior to bundle event
   * delivery when a bundle is installed, starting, started, stopping,
   * stopped, and uninstalled. This method can filter the bundles which receive
   * the event.
   * <p>
   * This method is called one and only one time for
   * each bundle event generated, this includes bundle events which are
   * generated when there are no bundle listeners registered.
   *
   * @param event The bundle event to be delivered.
   * @param contexts A list of Bundle Contexts for bundles which have
   *        listeners to which the specified event will be delivered. The
   *        implementation of this method may remove bundle contexts from the
   *        list to prevent the event from being delivered to the
   *        associated bundles.
   */
  virtual void Event(const BundleEvent& event,
                     ShrinkableVector<BundleContext>& contexts) = 0;
};
}

#endif // CPPMICROSERVICES_BUNDLEEVENTHOOK_H

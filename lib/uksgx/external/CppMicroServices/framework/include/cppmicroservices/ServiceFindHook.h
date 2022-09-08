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

#ifndef CPPMICROSERVICES_SERVICEFINDHOOK_H
#define CPPMICROSERVICES_SERVICEFINDHOOK_H

#include "cppmicroservices/ServiceInterface.h"
#include "cppmicroservices/ShrinkableVector.h"

#include <string>

namespace cppmicroservices {

class Bundle;
class BundleContext;
class ServiceReferenceBase;

/**
 * @ingroup MicroServices
 *
 * Service Find Hook Service.
 *
 * <p>
 * Bundles registering this service will be called during service find
 * (get service references) operations.
 *
 * @remarks Implementations of this interface are required to be thread-safe.
 */
struct US_Framework_EXPORT ServiceFindHook
{

  virtual ~ServiceFindHook();

  /**
   * Find hook method. This method is called during the service find operation
   * (for example, BundleContext::GetServiceReferences<S>()). This method can
   * filter the result of the find operation.
   *
   * @param context The bundle context of the bundle performing the find
   *        operation.
   * @param name The class name of the services to find or an empty string to
   *        find all services.
   * @param filter The filter criteria of the services to find or an empty string
   *        for no filter criteria.
   * @param references A list of Service References to be returned as a result of the
   *        find operation. The implementation of this method may remove
   *        service references from the list to prevent the references from being
   *        returned to the bundle performing the find operation.
   */
  virtual void Find(const BundleContext& context,
                    const std::string& name,
                    const std::string& filter,
                    ShrinkableVector<ServiceReferenceBase>& references) = 0;
};
}

#endif // CPPMICROSERVICES_SERVICEFINDHOOK_H

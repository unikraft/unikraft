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

#ifndef CPPMICROSERVICES_SERVICEFACTORY_H
#define CPPMICROSERVICES_SERVICEFACTORY_H

#include "cppmicroservices/ServiceInterface.h"
#include "cppmicroservices/ServiceRegistration.h"

namespace cppmicroservices {

/**
 * \ingroup MicroServices
 *
 * A factory for \link Constants::SCOPE_BUNDLE bundle scope\endlink services.
 * The factory can provide service objects unique to each bundle.
 *
 * When registering a service, a \c ServiceFactory object can be
 * used instead of a service object, so that the bundle developer can create
 * a customized service object for each bundle that is using the service.
 *
 * When a bundle requests the service object, the framework calls the
 * \c ServiceFactory::GetService method to return a service object
 * customized for the requesting bundle. The returned service object is
 * cached by the framework for subsequent calls to
 * <code>BundleContext::GetService(const ServiceReference&)</code> until
 * the bundle releases its use of the service.
 *
 * When the bundle's use count for the service is decremented to zero
 * (including the bundle stopping or the service being unregistered), the
 * framework will call the \c ServiceFactory::UngetService method.
 *
 * \c ServiceFactory objects are only used by the framework and are not
 * made available to other bundles in the bundle environment. The framework
 * may concurrently call a \c ServiceFactory.
 *
 * @see BundleContext#GetService
 * @see PrototypeServiceFactory
 * @remarks This class is thread safe.
 */
class ServiceFactory
{

public:
  virtual ~ServiceFactory() {}

  /**
   * Returns a service object for a bundle.
   *
   * The framework invokes this method the first time the specified
   * \c bundle requests a service object using the
   * <code>BundleContext::GetService(const ServiceReferenceBase&)</code> method. The
   * factory can then return a customized service object for each bundle.
   *
   * The framework checks that the returned service object is valid. If the
   * returned service object is null or does not contain entries for all the
   * classes named when the service was registered, a framework event of type
   * \c FrameworkEvent::FRAMEWORK_ERROR is fired containing a service exception
   * of type \c ServiceException::FACTORY_ERROR and null is returned to the
   * bundle. If this method throws an exception, a framework event of type
   * \c FrameworkEvent::FRAMEWORK_ERROR is fired containing a service exception
   * of type \c ServiceException::FACTORY_EXCEPTION with the thrown exception
   * as a nested exception and null is returned to the bundle. If this method is
   * recursively called for the specified bundle, a framework event of type
   * \c FrameworkEvent::FRAMEWORK_ERROR is fired containing a service exception
   * of type \c ServiceException::FACTORY_RECURSION and null is returned to the
   * bundle.
   *
   * The framework caches the valid service object, and will return the same
   * service object on any future call to \c BundleContext::GetService for the
   * specified bundle. This means the framework does not allow this method to
   * be concurrently called for the specified bundle.
   *
   * @param bundle The bundle requesting the service.
   * @param registration The \c ServiceRegistrationBase object for the
   *        requested service.
   * @return A service object that <strong>must</strong> contain entries for all
   *         the interfaces named when the service was registered.
   * @see BundleContext#GetService
   * @see InterfaceMapConstPtr
   */
  virtual InterfaceMapConstPtr GetService(
    const Bundle& bundle,
    const ServiceRegistrationBase& registration) = 0;

  /**
   * Releases a service object customized for a bundle.
   *
   * The Framework invokes this method when a service has been released by a
   * bundle. If this method throws an exception, a framework event of type
   * \c FrameworkEvent::FRAMEWORK_ERROR is fired containing a service
   * exception of type \c ServiceException::FACTORY_EXCEPTION with the thrown
   * exception as a nested exception.
   *
   * @param bundle The Bundle releasing the service.
   * @param registration The \c ServiceRegistration object for the
   *        service being released.
   * @param service The service object returned by a previous call to the
   *        \c ServiceFactory::GetService method.
   * @see InterfaceMapConstPtr
   */
  virtual void UngetService(const Bundle& bundle,
                            const ServiceRegistrationBase& registration,
                            const InterfaceMapConstPtr& service) = 0;
};
}

#endif // CPPMICROSERVICES_SERVICEFACTORY_H

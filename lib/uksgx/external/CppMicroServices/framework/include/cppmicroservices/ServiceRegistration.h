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

#ifndef CPPMICROSERVICES_SERVICEREGISTRATION_H
#define CPPMICROSERVICES_SERVICEREGISTRATION_H

#include "cppmicroservices/ServiceRegistrationBase.h"

namespace cppmicroservices {

/**
\defgroup gr_serviceregistration ServiceRegistration

\brief Groups ServiceRegistration related symbols.
*/

/**
 * \ingroup MicroServices
 * \ingroup gr_serviceregistration
 *
 * A registered service.
 *
 * <p>
 * The framework returns a <code>ServiceRegistration</code> object when a
 * <code>BundleContext#RegisterService()</code> method invocation is successful.
 * The <code>ServiceRegistration</code> object is for the private use of the
 * registering bundle and should not be shared with other bundles.
 * <p>
 * The <code>ServiceRegistration</code> object may be used to update the
 * properties of the service or to unregister the service.
 *
 * @tparam I1 Class type of the first service interface
 * @tparam Interfaces Template parameter pack containing zero or more service interfaces
 * @see BundleContext#RegisterService()
 */
template<class I1, class... Interfaces>
class ServiceRegistration : public ServiceRegistrationBase
{

public:
  /**
   * Creates an invalid ServiceRegistration object. You can use
   * this object in boolean expressions and it will evaluate to
   * <code>false</code>.
   */
  ServiceRegistration()
    : ServiceRegistrationBase()
  {}

  /**
   * Returns a <code>ServiceReference</code> object for a service being
   * registered.
   * <p>
   * The <code>ServiceReference</code> object may be shared with other
   * bundles.
   *
   * @throws std::logic_error If this
   *         <code>ServiceRegistration</code> object has already been
   *         unregistered or if it is invalid.
   * @return <code>ServiceReference</code> object.
   */
  template<class Interface>
  ServiceReference<Interface> GetReference() const
  {
    static_assert(detail::Contains<Interface, I1, Interfaces...>::value,
                  "Requested interface type not available");
    return this->ServiceRegistrationBase::GetReference(
      us_service_interface_iid<Interface>());
  }

  /**
   * Returns a <code>ServiceReference</code> object for a service being
   * registered.
   * <p>
   * The <code>ServiceReference</code> object refers to the first interface
   * type and may be shared with other bundles.
   *
   * @throws std::logic_error If this
   *         <code>ServiceRegistration</code> object has already been
   *         unregistered or if it is invalid.
   * @return <code>ServiceReference</code> object.
   */
  ServiceReference<I1> GetReference() const
  {
    return this->ServiceRegistrationBase::GetReference(
      us_service_interface_iid<I1>());
  }

  using ServiceRegistrationBase::operator=;

private:
  friend class BundleContext;

  ServiceRegistration(const ServiceRegistrationBase& base)
    : ServiceRegistrationBase(base)
  {}
};

/// \cond

template<>
class ServiceRegistration<void> : public ServiceRegistrationBase
{
public:
  /**
   * Creates an invalid ServiceRegistration object. You can use
   * this object in boolean expressions and it will evaluate to
   * <code>false</code>.
   */
  ServiceRegistration()
    : ServiceRegistrationBase()
  {}

  ServiceRegistration(const ServiceRegistrationBase& base)
    : ServiceRegistrationBase(base)
  {}

  using ServiceRegistrationBase::operator=;
};
/// \endcond

/**
 * \ingroup MicroServices
 * \ingroup gt_serviceregistration
 *
 * A service registration object of unknown type.
 */
typedef ServiceRegistration<void> ServiceRegistrationU;
}

#endif // CPPMICROSERVICES_SERVICEREGISTRATION_H

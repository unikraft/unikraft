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

#ifndef CPPMICROSERVICES_SERVICEREFERENCEBASE_H
#define CPPMICROSERVICES_SERVICEREFERENCEBASE_H

#include "cppmicroservices/Any.h"

#include <atomic>
#include <memory>

namespace cppmicroservices {

class Bundle;
class ServiceRegistrationBasePrivate;
class ServiceReferenceBasePrivate;

/**
 * \ingroup MicroServices
 * \ingroup gr_servicereference
 *
 * A reference to a service.
 *
 * \rststar
 * .. note::
 *    This class is provided as public API for low-level service queries only.
 *    In almost all cases you should use the template ServiceReference instead.
 * \endrststar
 */
class US_Framework_EXPORT ServiceReferenceBase
{

public:
  ServiceReferenceBase(const ServiceReferenceBase& ref);

  /**
   * Converts this ServiceReferenceBase instance into a boolean
   * expression. If this instance was default constructed or
   * the service it references has been unregistered, the conversion
   * returns <code>false</code>, otherwise it returns <code>true</code>.
   */
  explicit operator bool() const;

  /**
   * Releases any resources held or locked by this
   * <code>ServiceReferenceBase</code> and renders it invalid.
   */
  ServiceReferenceBase& operator=(std::nullptr_t);

  ~ServiceReferenceBase();

  /**
   * Returns the property value to which the specified property key is mapped
   * in the properties <code>ServiceProperties</code> object of the service
   * referenced by this <code>ServiceReferenceBase</code> object.
   *
   * <p>
   * Property keys are case-insensitive.
   *
   * <p>
   * This method continues to return property values after the service has
   * been unregistered. This is so references to unregistered services can
   * still be interrogated.
   *
   * @param key The property key.
   * @return The property value to which the key is mapped; an invalid Any
   *         if there is no property named after the key.
   */
  Any GetProperty(const std::string& key) const;

  /**
   * Returns a list of the keys in the <code>ServiceProperties</code>
   * object of the service referenced by this <code>ServiceReferenceBase</code>
   * object.
   *
   * <p>
   * This method will continue to return the keys after the service has been
   * unregistered. This is so references to unregistered services can
   * still be interrogated.
   *
   * @deprecated Since 3.0, use GetPropertyKeys() instead.
   *
   * @param keys A vector being filled with the property keys.
   */
  void GetPropertyKeys(std::vector<std::string>& keys) const;

  /**
   * Returns a list of the keys in the <code>ServiceProperties</code>
   * object of the service referenced by this <code>ServiceReferenceBase</code>
   * object.
   *
   * <p>
   * This method will continue to return the keys after the service has been
   * unregistered. This is so references to unregistered services can
   * still be interrogated.
   *
   * @return A vector being filled with the property keys.
   */
  std::vector<std::string> GetPropertyKeys() const;

  /**
   * Returns the bundle that registered the service referenced by this
   * <code>ServiceReferenceBase</code> object.
   *
   * <p>
   * This method must return an invalid bundle when the service has been
   * unregistered. This can be used to determine if the service has been
   * unregistered.
   *
   * @return The bundle that registered the service referenced by this
   *         <code>ServiceReferenceBase</code> object; an invalid bundle if that
   *         service has already been unregistered.
   * @see BundleContext::RegisterService(const InterfaceMap&, const ServiceProperties&)
   * @see Bundle::operator bool() const
   */
  Bundle GetBundle() const;

  /**
   * Returns the bundles that are using the service referenced by this
   * <code>ServiceReferenceBase</code> object. Specifically, this method returns
   * the bundles whose usage count for that service is greater than zero.
   *
   * @return A list of bundles whose usage count for the service referenced
   *         by this <code>ServiceReferenceBase</code> object is greater than
   *         zero.
   */
  std::vector<Bundle> GetUsingBundles() const;

  /**
   * Returns the interface identifier this ServiceReferenceBase object
   * is bound to.
   *
   * A default constructed ServiceReferenceBase object is not bound to
   * any interface identifier and calling this method will return an
   * empty string.
   *
   * @return The interface identifier for this ServiceReferenceBase object.
   */
  std::string GetInterfaceId() const;

  /**
   * Checks whether this ServiceReferenceBase object can be converted to
   * another ServiceReferenceBase object, which will be bound to the
   * given interface identifier.
   *
   * ServiceReferenceBase objects can be converted if the underlying service
   * implementation was registered under multiple service interfaces.
   *
   * @param interfaceid
   * @return \c true if this ServiceReferenceBase object can be converted,
   *         \c false otherwise.
   */
  bool IsConvertibleTo(const std::string& interfaceid) const;

  /**
   * Compares this <code>ServiceReferenceBase</code> with the specified
   * <code>ServiceReferenceBase</code> for order.
   *
   * <p>
   * If this <code>ServiceReferenceBase</code> and the specified
   * <code>ServiceReferenceBase</code> have the same \link Constants::SERVICE_ID
   * service id\endlink they are equal. This <code>ServiceReferenceBase</code> is less
   * than the specified <code>ServiceReferenceBase</code> if it has a lower
   * {@link Constants::SERVICE_RANKING service ranking} and greater if it has a
   * higher service ranking. Otherwise, if this <code>ServiceReferenceBase</code>
   * and the specified <code>ServiceReferenceBase</code> have the same
   * {@link Constants::SERVICE_RANKING service ranking}, this
   * <code>ServiceReferenceBase</code> is less than the specified
   * <code>ServiceReferenceBase</code> if it has a higher
   * {@link Constants::SERVICE_ID service id} and greater if it has a lower
   * service id.
   *
   * @param reference The <code>ServiceReferenceBase</code> to be compared.
   * @return Returns a false or true if this
   *         <code>ServiceReferenceBase</code> is less than or greater
   *         than the specified <code>ServiceReferenceBase</code>.
   */
  bool operator<(const ServiceReferenceBase& reference) const;

  bool operator==(const ServiceReferenceBase& reference) const;

  ServiceReferenceBase& operator=(const ServiceReferenceBase& reference);

private:
  friend class BundlePrivate;
  friend class BundleContext;
  friend class BundleHooks;
  friend class ServiceHooks;
  friend class ServiceObjectsBase;
  friend struct UngetHelper;
  friend class ServiceObjectsBasePrivate;
  friend class ServiceRegistrationBase;
  friend class ServiceRegistrationBasePrivate;
  friend class ServiceListeners;
  friend class ServiceRegistry;
  friend class LDAPFilter;

  template<class S>
  friend struct ServiceHolder;
  template<class S>
  friend class ServiceReference;

  friend struct ::std::hash<ServiceReferenceBase>;

  std::size_t Hash() const;

  /**
   * Creates an invalid ServiceReferenceBase object. You can use
   * this object in boolean expressions and it will evaluate to
   * <code>false</code>.
   */
  ServiceReferenceBase();

  ServiceReferenceBase(ServiceRegistrationBasePrivate* reg);

  void SetInterfaceId(const std::string& interfaceId);

  // This class is not thread-safe, but we support thread-safe
  // copying and assignment.
  std::atomic<ServiceReferenceBasePrivate*> d;
};

/**
 * \ingroup MicroServices
 * \ingroup gr_servicereference
 *
 * Writes a string representation of \c serviceRef to the stream \c os.
 */
US_Framework_EXPORT std::ostream& operator<<(
  std::ostream& os,
  const ServiceReferenceBase& serviceRef);
}

/**
 * \ingroup MicroServices
 * \ingroup gr_servicereference
 *
 * \struct std::hash<cppmicroservices::ServiceReferenceBase> ServiceReferenceBase.h <cppmicroservices/ServiceReferenceBase.h>
 *
 * Hash functor specialization for \link cppmicroservices#ServiceReferenceBase ServiceReferenceBase\endlink objects.
 */

US_HASH_FUNCTION_BEGIN(cppmicroservices::ServiceReferenceBase)
return arg.Hash();
US_HASH_FUNCTION_END

#endif // CPPMICROSERVICES_SERVICEREFERENCEBASE_H

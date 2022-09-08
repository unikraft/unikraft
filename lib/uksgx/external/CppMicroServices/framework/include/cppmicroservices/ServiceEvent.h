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

#ifndef CPPMICROSERVICES_SERVICEEVENT_H
#define CPPMICROSERVICES_SERVICEEVENT_H

#include "cppmicroservices/ServiceReference.h"

US_MSVC_PUSH_DISABLE_WARNING(
  4251) // 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'

namespace cppmicroservices {

class ServiceEventData;

/**
\defgroup gr_serviceevent ServiceEvent

\brief Groups ServiceEvent class related symbols.
*/

/**
 * \ingroup MicroServices
 * \ingroup gr_serviceevent
 *
 * An event from the Micro Services framework describing a service lifecycle change.
 * <p>
 * <code>ServiceEvent</code> objects are delivered to
 * listeners connected via BundleContext::AddServiceListener() when a
 * change occurs in this service's lifecycle. A type code is used to identify
 * the event type for future extendability.
 */
class US_Framework_EXPORT ServiceEvent
{

  std::shared_ptr<ServiceEventData> d;

public:
  /**
   * The service event type.
   */
  enum Type : uint32_t
  {

    /**
     * This service has been registered.
     * <p>
     * This event is delivered <strong>after</strong> the service
     * has been registered with the framework.
     *
     * @see BundleContext#RegisterService
     */
    SERVICE_REGISTERED = 0x00000001,

    /**
     * The properties of a registered service have been modified.
     * <p>
     * This event is delivered <strong>after</strong> the service
     * properties have been modified.
     *
     * @see ServiceRegistration#SetProperties
     */
    SERVICE_MODIFIED = 0x00000002,

    /**
     * This service is in the process of being unregistered.
     * <p>
     * This event is delivered <strong>before</strong> the service
     * has completed unregistering.
     *
     * <p>
     * If a bundle is using a service that is <code>SERVICE_UNREGISTERING</code>, the
     * bundle should release its use of the service when it receives this event.
     * If the bundle does not release its use of the service when it receives
     * this event, the framework will automatically release the bundle's use of
     * the service while completing the service unregistration operation.
     *
     * @see ServiceRegistration#Unregister
     */
    SERVICE_UNREGISTERING = 0x00000004,

    /**
     * The properties of a registered service have been modified and the new
     * properties no longer match the listener's filter.
     * <p>
     * This event is delivered <strong>after</strong> the service
     * properties have been modified. This event is only delivered to listeners
     * which were added with a non-empty filter where the filter
     * matched the service properties prior to the modification but the filter
     * does not match the modified service properties.
     *
     * @see ServiceRegistration#SetProperties
     */
    SERVICE_MODIFIED_ENDMATCH = 0x00000008

  };

  /**
   * Creates an invalid instance.
   */
  ServiceEvent();

  /**
   * Can be used to check if this ServiceEvent instance is valid,
   * or if it has been constructed using the default constructor.
   *
   * @return <code>true</code> if this event object is valid,
   *         <code>false</code> otherwise.
   */
  explicit operator bool() const;

  /**
   * Creates a new service event object.
   *
   * @param type The event type.
   * @param reference A <code>ServiceReference</code> object to the service
   *        that had a lifecycle change.
   */
  ServiceEvent(Type type, const ServiceReferenceBase& reference);

  ServiceEvent(const ServiceEvent& other);

  ServiceEvent& operator=(const ServiceEvent& other);

  /**
   * Returns a reference to the service that had a change occur in its
   * lifecycle.
   * <p>
   * This reference is the source of the event.
   *
   * @return Reference to the service that had a lifecycle change.
   */
  ServiceReferenceU GetServiceReference() const;

  template<class S>
  ServiceReference<S> GetServiceReference() const
  {
    return GetServiceReference();
  }

  /**
   * Returns the type of event. The event type values are:
   * <ul>
   * <li>{@link #SERVICE_REGISTERED} </li>
   * <li>{@link #SERVICE_MODIFIED} </li>
   * <li>{@link #SERVICE_MODIFIED_ENDMATCH} </li>
   * <li>{@link #SERVICE_UNREGISTERING} </li>
   * </ul>
   *
   * @return Type of service lifecycle change.
   */
  Type GetType() const;
};

/**
 * \ingroup MicroServices
 * \ingroup gr_serviceevent
 *
 * Writes a string representation of \c type to the stream \c os.
 */
US_Framework_EXPORT std::ostream& operator<<(std::ostream& os,
                                             const ServiceEvent::Type& type);

/**
 * \ingroup MicroServices
 * \ingroup gr_serviceevent
 *
 * Writes a string representation of \c event to the stream \c os.
 */
US_Framework_EXPORT std::ostream& operator<<(std::ostream& os,
                                             const ServiceEvent& event);
}

US_MSVC_POP_WARNING

#endif // CPPMICROSERVICES_SERVICEEVENT_H

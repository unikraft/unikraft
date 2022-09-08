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

#ifndef CPPMICROSERVICES_FRAMEWORKEVENT_H
#define CPPMICROSERVICES_FRAMEWORKEVENT_H

#include "cppmicroservices/FrameworkExport.h"

#include <iostream>
#include <memory>

US_MSVC_PUSH_DISABLE_WARNING(
  4251) // 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'

namespace cppmicroservices {

class Bundle;
class FrameworkEventData;

/**
\defgroup gr_frameworkevent FrameworkEvent

\brief Groups FrameworkEvent class related symbols.
*/

/**
 * \ingroup MicroServices
 * \ingroup gr_frameworkevent
 *
 * An event from the Micro Services framework describing a Framework event.
 * <p>
 * <code>FrameworkEvent</code> objects are delivered to listeners connected
 * via BundleContext::AddFrameworkListener() when an event occurs within the
 * Framework which a user would be interested in. A <code>Type</code> code
 * is used to identify the event type for future extendability.
 *
 * @see BundleContext#AddFrameworkListener
 */
class US_Framework_EXPORT FrameworkEvent
{

  std::shared_ptr<FrameworkEventData> d;

public:
  /**
   * A type code used to identify the event type for future extendability.
   */
  enum Type : uint32_t
  {

    /**
     * The Framework has started.
     *
     * <p>
     * This event is fired when the Framework has started after all installed
     * bundles that are marked to be started have been started. The source of
     * this event is the System Bundle.
     *
     */
    FRAMEWORK_STARTED = 0x00000001,

    /**
     * The Framework has been started.
     * <p>
     * The Framework's
     * \link BundleActivator::Start(BundleContext) BundleActivator Start\endlink method
     * has been executed.
     */
    FRAMEWORK_ERROR = 0x00000002,

    /**
     * A warning has occurred.
     *
     * <p>
     * There was a warning associated with a bundle.
     *
     */
    FRAMEWORK_WARNING = 0x00000010,

    /**
     * An informational event has occurred.
     *
     * <p>
     * There was an informational event associated with a bundle.
     */
    FRAMEWORK_INFO = 0x00000020,

    /**
     * The Framework has been stopped.
     * <p>
     * This event is fired when the Framework has been stopped because of a stop
     * operation on the system bundle. The source of this event is the System
     * Bundle.
     */
    FRAMEWORK_STOPPED = 0x00000040,

    /**
     * The Framework is about to be stopped.
     * <p>
     * This event is fired when the Framework has been stopped because of an
     * update operation on the system bundle. The Framework will be restarted
     * after this event is fired. The source of this event is the System Bundle.
     */
    FRAMEWORK_STOPPED_UPDATE = 0x00000080,

    /**
     * The Framework did not stop before the wait timeout expired.
     * <p>
     * This event is fired when the Framework did not stop before the wait timeout expired.
     * The source of this event is the System Bundle.
     */
    FRAMEWORK_WAIT_TIMEDOUT = 0x00000200

  };

  /**
   * Creates an invalid instance.
   */
  FrameworkEvent();

  /**
   * Returns <code>false</code> if the FrameworkEvent is empty (i.e invalid) and
   * <code>true</code> if the FrameworkEvent is not null and contains valid data.
   *
   * @return <code>true</code> if this event object is valid,
   *         <code>false</code> otherwise.
   */
  explicit operator bool() const;

  /**
   * Creates a Framework event of the specified type.
   *
   * @param type The event type.
   * @param bundle The bundle associated with the event. This bundle is also the source of the event.
   * @param message The message associated with the event.
   * @param exception The exception associated with this event. Should be nullptr if there is no exception.
   */
  FrameworkEvent(Type type,
                 const Bundle& bundle,
                 const std::string& message,
                 const std::exception_ptr exception = nullptr);

  /**
   * Returns the bundle associated with the event.
   *
   * @return The bundle associated with the event.
   */
  Bundle GetBundle() const;

  /**
   * Returns the message associated with the event.
   *
   *@return the message associated with the event.
   */
  std::string GetMessage() const;

  /**
   * Returns the exception associated with this event.
   *
   * @remarks Use <code>std::rethrow_exception</code> to throw the exception returned.
   *
   * @return The exception. May be <code>nullptr</code> if there is no related exception.
   */
  std::exception_ptr GetThrowable() const;

  /**
   * Returns the type of framework event. The type values are:
   * <ul>
   * <li>{@link #FRAMEWORK_STARTED}
   * <li>{@link #FRAMEWORK_ERROR}
   * <li>{@link #FRAMEWORK_WARNING}
   * <li>{@link #FRAMEWORK_INFO}
   * <li>{@link #FRAMEWORK_STOPPED}
   * <li>{@link #FRAMEWORK_STOPPED_UPDATE}
   * <li>{@link #FRAMEWORK_WAIT_TIMEDOUT}
   * </ul>
   *
   * @return The type of Framework event.
   */
  Type GetType() const;
};

/**
 * \ingroup MicroServices
 * \ingroup gr_frameworkevent
 *
 * Writes a string representation of \c eventType to the stream \c os.
 */
US_Framework_EXPORT std::ostream& operator<<(std::ostream& os,
                                             FrameworkEvent::Type eventType);

/**
 * \ingroup MicroServices
 * \ingroup gr_frameworkevent
 *
 * Writes a string representation of \c evt to the stream \c os.
 */
US_Framework_EXPORT std::ostream& operator<<(std::ostream& os,
                                             const FrameworkEvent& evt);

/**
 * \ingroup MicroServices
 * \ingroup gr_frameworkevent
 *
 * Compares two framework events for equality.
 */
US_Framework_EXPORT bool operator==(const FrameworkEvent& rhs,
                                    const FrameworkEvent& lhs);
/** @}*/
}

US_MSVC_POP_WARNING

#endif // CPPMICROSERVICES_FRAMEWORKEVENT_H

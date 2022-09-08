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

#ifndef CPPMICROSERVICES_FRAMEWORK_H
#define CPPMICROSERVICES_FRAMEWORK_H

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/FrameworkConfig.h"

#include <chrono>
#include <map>
#include <ostream>
#include <string>

namespace cppmicroservices {

class FrameworkEvent;
class FrameworkPrivate;

/**
 * \ingroup MicroServices
 *
 * A Framework instance.
 *
 * <p>
 * A Framework is itself a bundle and is known as the "System Bundle".
 * The System Bundle differs from other bundles in the following ways:
 * - The system bundle is always assigned a bundle identifier of zero (0).
 * - The system bundle <code>GetLocation</code> method returns the string: "System Bundle".
 * - The system bundle's life cycle cannot be managed like normal bundles. Its life cycle methods
 *   behave as follows:
 *   - Start - Initialize the framework and start installed bundles.
 *   - Stop - Stops all installed bundles.
 *   - Uninstall - The Framework throws a std::runtime_error exception indicating that the
 *     system bundle cannot be uninstalled.
 *
 * Framework instances are created using a FrameworkFactory. The methods of this class can be
 * used to manage and control the created framework instance.
 *
 * @remarks This class is thread-safe.
 *
 * @see FrameworkFactory::NewFramework(const std::map<std::string, Any>& configuration)
 */
class US_Framework_EXPORT Framework : public Bundle
{
public:
  /**
     * Convert a \c Bundle representing the system bundle to a
     * \c Framework instance.
     *
     * @param b The system bundle
     *
     * @throws std::logic_error If the bundle \b is not the system bundle.
     */
  explicit Framework(Bundle b);

  Framework(const Framework& fw);            // = default
  Framework(Framework&& fw);                 // = default
  Framework& operator=(const Framework& fw); // = default
  Framework& operator=(Framework&& fw);      // = default

  /**
     * Initialize this Framework. After calling this method, this Framework
     * has:
     * - Generated a new {@link Constants#FRAMEWORK_UUID framework UUID}.
     * - Moved to the {@link #STATE_STARTING} state.
     * - A valid Bundle Context.
     * - Event handling enabled.
     * - Reified Bundle objects for all installed bundles.
     * - Registered any framework services.
     *
     * <p>
     * This Framework will not actually be started until {@link #Start() Start}
     * is called.
     *
     * <p>
     * This method does nothing if called when this Framework is in the
     * {@link #STATE_STARTING}, {@link #STATE_ACTIVE} or {@link #STATE_STOPPING} states.
     *
     * @throws std::runtime_error If this Framework could not be initialized.
     */
  void Init();

  /**
     * Wait until this Framework has completely stopped. The \c Stop
     * method on a Framework performs an asynchronous stop of the Framework
     * if it was built with threading support.
     *
     * This method can be used to wait until the asynchronous
     * stop of this Framework has completed. This method will only wait if
     * called when this Framework is in the {@link #STATE_STARTING}, {@link #STATE_ACTIVE},
     * or {@link #STATE_STOPPING} states. Otherwise it will return immediately.
     * <p>
     * A Framework Event is returned to indicate why this Framework has stopped.
     *
     * @param timeout Maximum time duration to wait until this
     *        Framework has completely stopped. A value of zero will wait
     *        indefinitely.
     * @return A Framework Event indicating the reason this method returned. The
     *         following \c FrameworkEvent types may be returned by this
     *         method.
     *         <ul>
     *         <li>{@link FrameworkEvent#FRAMEWORK_STOPPED FRAMEWORK_STOPPED} - This Framework has
     *         been stopped. </li>
     *
     *         <li>{@link FrameworkEvent#FRAMEWORK_ERROR FRAMEWORK_ERROR} - The Framework
     *         encountered an error while shutting down or an error has occurred
     *         which forced the framework to shutdown. </li>
     *
     *         <li> {@link FrameworkEvent#FRAMEWORK_WAIT_TIMEDOUT FRAMEWORK_WAIT_TIMEDOUT} - This
     *         method has timed out and returned before this Framework has
     *         stopped.</li>
     *         </ul>
     */
  FrameworkEvent WaitForStop(const std::chrono::milliseconds& timeout);

  /**
     * Start this Framework.
     *
     * <p>
     * The following steps are taken to start this Framework:
     * -# If this Framework is not in the {@link #STATE_STARTING} state,
     *    {@link #Init() initialize} this Framework.
     * -# All installed bundles must be started in accordance with each
     *    bundle's persistent <i>autostart setting</i>. This means some bundles
     *    will not be started, some will be started with <i>eager activation</i>
     *    and some will be started with their <i>declared activation</i> policy.
     *    Any exceptions that occur during bundle starting are wrapped in a
     *    \c std::runtime_error and then published as a framework event of type
     *    {@link FrameworkEvent#FRAMEWORK_ERROR}
     * -# This Framework's state is set to {@link #STATE_ACTIVE}.
     * -# A framework event of type {@link FrameworkEvent#FRAMEWORK_STARTED} is fired
     *
     * @throws std::runtime_error If this Framework could not be started.
     */
#ifdef DOXYGEN_RUN
  void Start();
#endif

  /**
     * Start this Framework.
     *
     * <p>
     * Calling this method is the same as calling {@link #Start()}. There are no
     * start options for the Framework.
     *
     * @param options Ignored. There are no start options for the Framework.
     * @throws std::runtime_error If this Framework could not be started.
     * @see #Start()
     */
#ifdef DOXYGEN_RUN
  void Start(uint32_t options);
#endif

  /**
     * Stop this Framework.
     *
     * <p>
     * The method returns immediately to the caller after initiating the
     * following steps to be taken on another thread. If the Framework was not
     * built with threading enabled, the steps are executed in the main thread.
     * -# This Framework's state is set to {@link #STATE_STOPPING}.
     * -# All installed bundles are stopped without changing each bundle's
     *    persistent <i>autostart setting</i>. Any exceptions that occur
     *    during bundle stopping are wrapped in a \c std::runtime_error and
     *    then published as a framework event of type {@link FrameworkEvent#FRAMEWORK_ERROR}
     * -# Unregister all services registered by this Framework.
     * -# Event handling is disabled.
     * -# This Framework's state is set to {@link #STATE_RESOLVED}.
     * -# All resources held by this Framework are released. This includes
     *    threads, open files, etc.
     * -# Notify all threads that are waiting at {@link #WaitForStop(const std::chrono::milliseconds&)
     *    WaitForStop} that the stop operation has completed.
     *
     * <p>
     * After being stopped, this Framework may be discarded, initialized or
     * started.
     *
     * @throws std::runtime_error If stopping this Framework could not be initiated.
     */
#ifdef DOXYGEN_RUN
  void Stop();
#endif

  /**
     * Stop this Framework.
     *
     * <p>
     * Calling this method is the same as calling {@link #Stop()}. There are no
     * stop options for the Framework.
     *
     * @param options Ignored. There are no stop options for the Framework.
     * @throws std::runtime_error If stopping this Framework could not be
     *         initiated.
     * @see #Stop()
     */
#ifdef DOXYGEN_RUN
  void Stop(uint32_t options);
#endif

  /**
     * The Framework cannot be uninstalled.
     *
     * This method always throws a std::runtime_error exception.
     *
     * @throws std::runtime_error This Framework cannot be uninstalled.
     */
#ifdef DOXYGEN_RUN
  void Uninstall();
#endif

  /**
    * Returns this Framework's location.
    *
    * <p>
    * This Framework is assigned the unique location "System Bundle"
    * since this Framework is also a System Bundle.
    *
    * @return The string "System Bundle".
    */
#ifdef DOXYGEN_RUN
  std::string GetLocation() const;
#endif

private:
  // Framework instances are exclusively constructed by the FrameworkFactory class
  friend class FrameworkFactory;

  Framework(const std::shared_ptr<FrameworkPrivate>& d);
};
}

#endif // CPPMICROSERVICES_FRAMEWORK_H

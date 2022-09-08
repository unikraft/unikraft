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

#ifndef CPPMICROSERVICES_LISTENERFUNCTORS_H
#define CPPMICROSERVICES_LISTENERFUNCTORS_H

#include "cppmicroservices/FrameworkExport.h"
#include "cppmicroservices/GlobalConfig.h"

#include <cstring>
#include <functional>

namespace cppmicroservices {

class ServiceEvent;
class BundleEvent;
class FrameworkEvent;
class ServiceListeners;

/**
  \defgroup gr_listeners Listeners

  \brief Groups Listener related symbols.
  */

/**
   * \ingroup MicroServices
   * \ingroup gr_listeners
   *
   * A \c ServiceEvent listener.
   *
   * A \c ServiceListener can be any callable object and is registered
   * with the Framework using the
   * {@link BundleContext#AddServiceListener(const ServiceListener&, const std::string&)} method.
   * \c ServiceListener instances are called with a \c ServiceEvent object when a
   * service has been registered, unregistered, or modified.
   *
   * @see ServiceEvent
   */
typedef std::function<void(const ServiceEvent&)> ServiceListener;

/**
   * \ingroup MicroServices
   * \ingroup gr_listeners
   *
   * A \c BundleEvent listener. When a \c BundleEvent is fired, it is
   * asynchronously (if threading support is enabled) delivered to a
   * \c BundleListener. The Framework delivers \c BundleEvent objects to
   * a \c BundleListener in order and does not concurrently call a
   * \c BundleListener.
   *
   * A \c BundleListener can be any callable object and is registered
   * with the Framework using the
   * {@link BundleContext#AddBundleListener(const BundleListener&)} method.
   * \c BundleListener instances are called with a \c BundleEvent object when a
   * bundle has been installed, resolved, started, stopped, updated, unresolved,
   * or uninstalled.
   *
   * @see BundleEvent
   */
typedef std::function<void(const BundleEvent&)> BundleListener;

/**
   * \ingroup MicroServices
   * \ingroup gr_listeners
   *
   * A \c FrameworkEvent listener. When a \c BundleEvent is fired, it is
   * asynchronously (if threading support is enabled) delivered to a
   * \c FrameworkListener. The Framework delivers \c FrameworkEvent objects to
   * a \c FrameworkListener in order and does not concurrently call a
   * \c FrameworkListener.
   *
   * A \c FrameworkListener can be any callable object and is registered
   * with the Framework using the
   * {@link BundleContext#AddFrameworkListener(const FrameworkListener&)} method.
   * \c FrameworkListener instances are called with a \c FrameworkEvent object when a
   * framework life-cycle event or notification message occured.
   *
   * @see FrameworkEvent
   */
typedef std::function<void(const FrameworkEvent&)> FrameworkListener;

/**
   * \ingroup MicroServices
   * \ingroup gr_listeners
   *
   * A convenience function that binds the member function <code>callback</code> of
   * an object of type <code>R</code> and returns a <code>ServiceListener</code> object.
   * This object can then be passed into <code>AddServiceListener()</code>.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *    This function exists only to maintain backwards compatibility
   *     and will be removed in the next major release. Use std::bind instead.
   * \endrststar
   *
   * @tparam R The type containing the member function.
   * @param receiver The object of type R.
   * @param callback The member function pointer.
   * @returns a ServiceListener object.
   */
template<class R>
US_DEPRECATED ServiceListener
ServiceListenerMemberFunctor(R* receiver,
                             void (R::*callback)(const ServiceEvent&))
{
  return std::bind(callback, receiver, std::placeholders::_1);
}

/**
   * \ingroup MicroServices
   * \ingroup gr_listeners
   *
   * A convenience function that binds the member function <code>callback</code> of
   * an object of type <code>R</code> and returns a <code>BundleListener</code> object.
   * This object can then be passed into <code>AddBundleListener()</code>.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *    This function exists only to maintain backwards compatibility
   *     and will be removed in the next major release. Use std::bind instead.
   * \endrststar
   *
   * @tparam R The type containing the member function.
   * @param receiver The object of type R.
   * @param callback The member function pointer.
   * @returns a BundleListener object.
   */
template<class R>
US_DEPRECATED BundleListener
BundleListenerMemberFunctor(R* receiver,
                            void (R::*callback)(const BundleEvent&))
{
  return std::bind(callback, receiver, std::placeholders::_1);
}

/**
   * \ingroup MicroServices
   * \ingroup gr_listeners
   *
   * A convenience function that binds the member function <code>callback</code> of
   * an object of type <code>R</code> and returns a <code>FrameworkListener</code> object.
   * This object can then be passed into <code>AddFrameworkListener()</code>.
   *
   * \rststar
   * .. deprecated:: 3.1.0
   *    This function exists only to maintain backwards compatibility
   *     and will be removed in the next major release. Use std::bind instead.
   * \endrststar
   *
   * @tparam R The type containing the member function.
   * @param receiver The object of type R.
   * @param callback The member function pointer.
   * @returns a FrameworkListener object.
   */
template<class R>
US_DEPRECATED FrameworkListener
BindFrameworkListenerToFunctor(R* receiver,
                               void (R::*callback)(const FrameworkEvent&))
{
  return std::bind(callback, receiver, std::placeholders::_1);
}
}

US_HASH_FUNCTION_BEGIN(cppmicroservices::ServiceListener)
typedef void (*TargetType)(const cppmicroservices::ServiceEvent&);
const TargetType* targetFunc = arg.target<TargetType>();
void* targetPtr = nullptr;
std::memcpy(&targetPtr, &targetFunc, sizeof(void*));
return hash<void*>()(targetPtr);
US_HASH_FUNCTION_END

#endif // CPPMICROSERVICES_LISTENERFUNCTORS_H

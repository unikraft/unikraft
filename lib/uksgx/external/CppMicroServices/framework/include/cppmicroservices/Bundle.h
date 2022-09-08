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

#ifndef CPPMICROSERVICES_BUNDLE_H
#define CPPMICROSERVICES_BUNDLE_H

#include "cppmicroservices/AnyMap.h"
#include "cppmicroservices/BundleVersion.h"
#include "cppmicroservices/GlobalConfig.h"

#include <map>
#include <memory>
#include <vector>
#include <chrono>

namespace cppmicroservices {

class Any;
class CoreBundleContext;
class BundleContext;
class BundleResource;
class BundlePrivate;

template<class S>
class ServiceReference;

using ServiceReferenceU = ServiceReference<void>;

/**
\defgroup gr_bundle Bundle

\brief Groups Bundle class related symbols.
*/

/**
 * \ingroup MicroServices
 * \ingroup gr_bundle
 *
 * An installed bundle in the Framework.
 *
 * A <code>%Bundle</code> object is the access point to define the lifecycle of an
 * installed bundle. Each bundle installed in the CppMicroServices environment has
 * an associated <code>%Bundle</code> object.
 *
 * A bundle has a unique identity, a <code>long</code>, chosen by the
 * Framework. This identity does not change during the lifecycle of a bundle.
 * Uninstalling and then reinstalling the bundle creates a new unique identity.
 *
 * A bundle can be in one of six states:
 * - STATE_UNINSTALLED
 * - STATE_INSTALLED
 * - STATE_RESOLVED
 * - STATE_STARTING
 * - STATE_STOPPING
 * - STATE_ACTIVE
 *
 * Values assigned to these states have no specified ordering; they represent
 * bit values that may be ORed together to determine if a bundle is in one of
 * the valid states.
 *
 * A bundle should only have active threads of execution when its state is one
 * of \c STATE_STARTING,\c STATE_ACTIVE, or \c STATE_STOPPING.
 * A \c STATE_UNINSTALLED bundle can not be set to another state; it is a
 * zombie and can only be reached because references are kept somewhere.
 *
 * The framework is the only entity that is allowed to create
 * <code>%Bundle</code> objects, and these objects are only valid within the
 * Framework that created them.
 *
 * Bundles have a natural ordering such that if two \c Bundles have the
 * same {@link #GetBundleId() bundle id} they are equal. A \c Bundle is
 * less than another \c Bundle if it has a lower {@link #GetBundleId()
 * bundle id} and is greater if it has a higher bundle id.
 *
 *
 * @remarks This class is thread safe.
 */
class US_Framework_EXPORT Bundle
{

public:
  using TimeStamp = std::chrono::steady_clock::time_point;

  /**
   * The bundle state.
   */
  enum State : uint32_t
  {

    /**
     * The bundle is uninstalled and may not be used.
     *
     * The \c STATE_UNINSTALLED state is only visible after a bundle is
     * uninstalled; the bundle is in an unusable state but references to the
     * \c Bundle object may still be available and used for introspection.
     *
     * The value of \c STATE_UNINSTALLED is 0x00000001.
     */
    STATE_UNINSTALLED = 0x00000001,

    /**
     * The bundle is installed but not yet resolved.
     *
     * A bundle is in the \c STATE_INSTALLED state when it has been installed in
     * the Framework but is not or cannot be resolved.
     *
     * This state is visible if the bundle's code dependencies are not resolved.
     * The Framework may attempt to resolve a \c STATE_INSTALLED bundle's code
     * dependencies and move the bundle to the \c STATE_RESOLVED state.
     *
     * The value of \c STATE_INSTALLED is 0x00000002.
     */
    STATE_INSTALLED = 0x00000002,

    /**
     * The bundle is resolved and is able to be started.
     *
     * A bundle is in the \c STATE_RESOLVED state when the Framework has
     * successfully resolved the bundle's code dependencies. These dependencies
     * include:
     * - None (this may change in future versions)
     *
     * Note that the bundle is not active yet. A bundle is put in the
     * \c STATE_RESOLVED state before it can be started. The Framework may
     * attempt to resolve a bundle at any time.
     *
     * The value of \c STATE_RESOLVED is 0x00000004.
     */
    STATE_RESOLVED = 0x00000004,

    /**
     * The bundle is in the process of starting.
     *
     * A bundle is in the \c STATE_STARTING state when its {@link #Start(uint32_t)
     * Start} method is active. A bundle must be in this state when the bundle's
     * BundleActivator#Start(BundleContext) is called.
     * If the \c BundleActivator#Start method completes without exception, then
     * the bundle has successfully started and moves to the \c STATE_ACTIVE
     * state.
     *
     * If the bundle has a {@link Constants#ACTIVATION_LAZY lazy activation
     * policy}, then the bundle may remain in this state for some time until the
     * activation is triggered.
     *
     * The value of \c STATE_STARTING is 0x00000008.
     */
    STATE_STARTING = 0x00000008,

    /**
     * The bundle is in the process of stopping.
     *
     * A bundle is in the \c STATE_STOPPING state when its {@link #Stop(uint32_t)
     * Stop} method is active. A bundle is in this state when the bundle's
     * {@link BundleActivator#Stop(BundleContext)} method
     * is called. When the \c BundleActivator#Stop method completes the bundle
     * is stopped and moves to the \c STATE_RESOLVED state.
     *
     * The value of \c STATE_STOPPING is 0x00000010.
     */
    STATE_STOPPING = 0x00000010,

    /**
     * The bundle is now running.
     *
     * A bundle is in the \c STATE_ACTIVE state when it has been successfully
     * started and activated.
     *
     * The value of \c STATE_ACTIVE is 0x00000020.
     */
    STATE_ACTIVE = 0x00000020
  };

  enum StartOptions : uint32_t
  {

    /**
     * The bundle start operation is transient and the persistent autostart
     * setting of the bundle is not modified.
     *
     * This bit may be set when calling {@link #Start(uint32_t)} to notify the
     * framework that the autostart setting of the bundle must not be modified.
     * If this bit is not set, then the autostart setting of the bundle is
     * modified.
     *
     * @see #Start(uint32_t)
     * @note This option is reserved for future use and not supported yet.
     */
    START_TRANSIENT = 0x00000001,

    /**
     * The bundle start operation must activate the bundle according to the
     * bundle's declared
     * {@link Constants#BUNDLE_ACTIVATIONPOLICY activation policy}.
     *
     * This bit may be set when calling {@link #Start(uint32_t)} to notify the
     * framework that the bundle must be activated using the bundle's declared
     * activation policy.
     *
     * @see Constants#BUNDLE_ACTIVATIONPOLICY
     * @see #Start(uint32_t)
     * @note This option is reserved for future use and not supported yet.
     */
    START_ACTIVATION_POLICY = 0x00000002
  };

  enum StopOptions : uint32_t
  {

    /**
     * The bundle stop is transient and the persistent autostart setting of the
     * bundle is not modified.
     *
     * This bit may be set when calling {@link #Stop(uint32_t)} to notify the
     * framework that the autostart setting of the bundle must not be modified.
     * If this bit is not set, then the autostart setting of the bundle is
     * modified.
     *
     * @see #Stop(uint32_t)
     * @note This option is reserved for future use and not supported yet.
     */
    STOP_TRANSIENT = 0x00000001
  };

  Bundle(const Bundle&);            // = default
  Bundle(Bundle&&);                 // = default
  Bundle& operator=(const Bundle&); // = default
  Bundle& operator=(Bundle&&);      // = default

  /**
   * Constructs an invalid %Bundle object.
   *
   * Valid bundle objects can only be created by the framework in
   * response to certain calls to a \c BundleContext object.
   * A \c BundleContext object is supplied to a bundle via its
   * \c BundleActivator or as a return value of the
   * \c GetBundleContext() method.
   *
   * @see operator bool() const
   */
  Bundle();

  virtual ~Bundle();

  /**
   * Compares this \c Bundle object with the specified bundle.
   *
   * Valid \c Bundle objects compare equal if and only if they
   * are installed in the same framework instance and their
   * bundle id is equal. Invalid \c Bundle objects are always
   * considered to be equal.
   *
   * @param rhs The \c Bundle object to compare this object with.
   * @return \c true if this \c Bundle object is equal to \c rhs,
   *         \c false otherwise.
   */
  bool operator==(const Bundle& rhs) const;

  /**
   * Compares this \c Bundle object with the specified bundle
   * for inequality.
   *
   * @param rhs The \c Bundle object to compare this object with.
   * @return Returns the result of <code>!(*this == rhs)</code>.
   */
  bool operator!=(const Bundle& rhs) const;

  /**
   * Compares this \c Bundle with the specified bundle for order.
   *
   * %Bundle objects are ordered first by their framework id and
   * then according to their bundle id.
   * Invalid \c Bundle objects will always compare greater then
   * valid \c Bundle objects.
   *
   * @param rhs The \c Bundle object to compare this object with.
   * @return \c
   */
  bool operator<(const Bundle& rhs) const;

  /**
   * Tests this %Bundle object for validity.
   *
   * Invalid \c Bundle objects are created by the default constructor or
   * can be returned by certain framework methods if the bundle has been
   * uninstalled.
   *
   * @return \c true if this %Bundle object is valid and can safely be used,
   *         \c false otherwise.
   */
  explicit operator bool() const;

  /**
   * Releases any resources held or locked by this
   * \c Bundle and renders it invalid.
   */
  Bundle& operator=(std::nullptr_t);

  /**
   * Returns this bundle's current state.
   *
   * A bundle can be in only one state at any time.
   *
   * @return An element of \c STATE_UNINSTALLED,\c STATE_INSTALLED,
   *         \c STATE_RESOLVED, \c STATE_STARTING, \c STATE_STOPPING,
   *         \c STATE_ACTIVE.
   */
  State GetState() const;

  /**
   * Returns this bundle's {@link BundleContext}. The returned
   * <code>BundleContext</code> can be used by the caller to act on behalf
   * of this bundle.
   *
   * If this bundle is not in the \c STATE_STARTING, \c STATE_ACTIVE, or
   * \c STATE_STOPPING states, then this bundle has no valid \c BundleContext
   * and this method will return an invalid \c BundleContext object.
   *
   * @return A valid or invalid <code>BundleContext</code> for this bundle.
   */
  BundleContext GetBundleContext() const;

  /**
   * Returns this bundle's unique identifier. This bundle is assigned a unique
   * identifier by the framework when it was installed.
   *
   * A bundle's unique identifier has the following attributes:
   * - Is unique.
   * - Is a <code>long</code>.
   * - Its value is not reused for another bundle, even after a bundle is
   *   uninstalled.
   * - Does not change while a bundle remains installed.
   * - Does not change when a bundle is re-started.
   *
   * This method continues to return this bundle's unique identifier while
   * this bundle is in the <code>STATE_UNINSTALLED</code> state.
   *
   * @return The unique identifier of this bundle.
   */
  long GetBundleId() const;

  /**
   * Returns this bundle's location.
   *
   * The location is the full path to the bundle's shared library.
   * This method continues to return this bundle's location
   * while this bundle is in the <code>STATE_UNINSTALLED</code> state.
   *
   * @return The string representation of this bundle's location.
   */
  std::string GetLocation() const;

  /**
   * Returns the symbolic name of this bundle as specified by the
   * US_BUNDLE_NAME preprocessor definition. The bundle symbolic
   * name together with a version must identify a unique bundle.
   *
   * This method continues to return this bundle's symbolic name while
   * this bundle is in the <code>STATE_UNINSTALLED</code> state.
   *
   * @return The symbolic name of this bundle.
   */
  std::string GetSymbolicName() const;

  /**
   * Returns the version of this bundle as specified in its
   * <code>manifest.json</code> file. If this bundle does not have a
   * specified version then {@link BundleVersion::EmptyVersion} is returned.
   *
   * This method continues to return this bundle's version while
   * this bundle is in the <code>STATE_UNINSTALLED</code> state.
   *
   * @return The version of this bundle.
   */
  BundleVersion GetVersion() const;

  /**
   * Returns this bundle's Manifest properties as key/value pairs.
   *
   * @return A map containing this bundle's Manifest properties as
   *         key/value pairs.
   *
   * @deprecated Since 3.0, use GetHeaders() instead.
   *
   * \rststar
   * .. seealso::
   *
   *    :any:`concept-bundle-properties`
   * \endrststar
   */
  US_DEPRECATED std::map<std::string, Any> GetProperties() const;

  /**
   * Returns this bundle's Manifest headers and values.
   *
   * Manifest header names are case-insensitive. The methods of the returned
   * \c AnyMap object operate on header names in a case-insensitive
   * manner.
   *
   * If a Manifest header value starts with &quot;%&quot;, it is
   * localized according to the default locale. If no localization is found
   * for a header value, the header value without the leading &quot;%&quot; is
   * returned.
   *
   * \note Localization is not yet supported, hence the leading &quot;%&quot;
   * is always removed.
   *
   * This method continues to return Manifest header information while
   * this bundle is in the \c UNINSTALLED state.
   *
   * @return A map containing this bundle's Manifest headers and values.
   *
   * @see Constants#BUNDLE_LOCALIZATION
   */
  AnyMap GetHeaders() const;

  /**
   * Returns the value of the specified property for this bundle.
   * If not found, the framework's properties are searched.
   * The method returns an empty Any if the property is not found.
   *
   * @param key The name of the requested property.
   * @return The value of the requested property, or an empty string
   *         if the property is undefined.
   *
   * \rststar
   * .. deprecated:: 3.0
   *    Use :any:`GetHeaders() <cppmicroservices::Bundle::GetHeaders>` or :any:`BundleContext::GetProperty(const std::string&) <cppmicroservices::BundleContext::GetProperty>` instead.
   *
   * .. seealso::
   *
   *    | :any:`cppmicroservices::Bundle::GetPropertyKeys`
   *    | :any:`concept-bundle-properties`
   * \endrststar
   */
  US_DEPRECATED Any GetProperty(const std::string& key) const;

  /**
   * Returns a list of top-level property keys for this bundle.
   *
   * @return A list of available property keys.
   *
   * @deprecated Since 3.0, use GetHeaders() or BundleContext::GetProperties()
   * instead.
   *
   * \rststar
   * .. seealso::
   *
   *    :any:`concept-bundle-properties`
   * \endrststar
   */
  US_DEPRECATED std::vector<std::string> GetPropertyKeys() const;

  /**
   * Returns this bundle's ServiceReference list for all services it
   * has registered or an empty list if this bundle has no registered
   * services.
   *
   * The list is valid at the time of the call to this method, however,
   * as the framework is a very dynamic environment, services can be
   * modified or unregistered at anytime.
   *
   * @return A list of ServiceReference objects for services this
   * bundle has registered.
   *
   * @throws std::logic_error If this bundle has been uninstalled, if
   *         the ServiceRegistrationBase object is invalid, or if the service is unregistered.
   */
  std::vector<ServiceReferenceU> GetRegisteredServices() const;

  /**
   * Returns this bundle's ServiceReference list for all services it is
   * using or returns an empty list if this bundle is not using any
   * services. A bundle is considered to be using a service if its use
   * count for that service is greater than zero.
   *
   * The list is valid at the time of the call to this method, however,
   * as the framework is a very dynamic environment, services can be
   * modified or unregistered at anytime.
   *
   * @return A list of ServiceReference objects for all services this
   * bundle is using.
   *
   * @throws std::logic_error If this bundle has been uninstalled, if
   *         the ServiceRegistrationBase object is invalid, or if the service is unregistered.
   */
  std::vector<ServiceReferenceU> GetServicesInUse() const;

  /**
   * Returns the resource at the specified \c path in this bundle.
   * The specified \c path is always relative to the root of this bundle and may
   * begin with '/'. A path value of "/" indicates the root of this bundle.
   *
   * @param path The path name of the resource.
   * @return A BundleResource object for the given \c path. If the \c path cannot
   * be found in this bundle an invalid BundleResource object is returned.
   *
   * @throws std::logic_error If this bundle has been uninstalled.
   */
  BundleResource GetResource(const std::string& path) const;

  /**
   * Returns resources in this bundle.
   *
   * This method is intended to be used to obtain configuration, setup, localization
   * and other information from this bundle.
   *
   * This method can either return only resources in the specified \c path or recurse
   * into subdirectories returning resources in the directory tree beginning at the
   * specified path.
   *
   * Examples:
   * \snippet uServices-resources/main.cpp 0
   *
   * @param path The path name in which to look. The path is always relative to the root
   * of this bundle and may begin with '/'. A path value of "/" indicates the root of this bundle.
   * @param filePattern The resource name pattern for selecting entries in the specified path.
   * The pattern is only matched against the last element of the resource path. Substring
   * matching is supported using the wildcard charachter ('*'). If \c filePattern is empty,
   * this is equivalent to "*" and matches all resources.
   * @param recurse If \c true, recurse into subdirectories. Otherwise only return resources
   * from the specified path.
   * @return A vector of BundleResource objects for each matching entry.
   *
   * @throws std::logic_error If this bundle has been uninstalled.
   */
  std::vector<BundleResource> FindResources(const std::string& path,
                                            const std::string& filePattern,
                                            bool recurse) const;

  /**
   * Returns the time when this bundle was last modified. A bundle is
   * considered to be modified when it is installed, updated or uninstalled.
   *
   * @return The time when this bundle was last modified.
   */
  TimeStamp GetLastModified() const;

  /**
   * Starts this bundle.
   *
   * If this bundle's state is \c STATE_UNINSTALLED then a
   * \c std::logic_error is thrown.
   *
   * The Framework sets this bundle's persistent autostart
   * setting to <em>Started with declared activation</em> if the
   * {@link #START_ACTIVATION_POLICY} option is set or
   * <em>Started with eager activation</em> if not set.
   *
   * The following steps are executed to start this bundle:
   * -# If this bundle is in the process of being activated or deactivated,
   *    then this method waits for activation or deactivation to complete
   *    before continuing. If this does not occur in a reasonable time, a
   *    \c std::runtime_error is thrown to indicate this bundle was unable to
   *    be started.
   * -# If this bundle's state is \c STATE_ACTIVE, then this method returns
   *    immediately.
   * -# If the {@link #START_TRANSIENT} option is not set, then set this
   *    bundle's autostart setting to <em>Started with declared activation</em>
   *    if the {@link #START_ACTIVATION_POLICY} option is set or
   *    <em>Started with eager activation</em> if not set. When the Framework is
   *    restarted and this bundle's autostart setting is not <em>Stopped</em>,
   *    this bundle must be automatically started.
   * -# If this bundle's state is not \c STATE_RESOLVED, an attempt is made to
   *    resolve this bundle. If the Framework cannot resolve this bundle, a
   *    \c std::runtime_error is thrown.
   * -# If the {@link #START_ACTIVATION_POLICY} option is set and this
   *    bundle's declared activation policy is {@link Constants#ACTIVATION_LAZY
   *    lazy} then:
   *    - If this bundle's state is \c STATE_STARTING, then this method returns
   *      immediately.
   *    - This bundle's state is set to \c STATE_STARTING.
   *    - A bundle event of type {@link BundleEvent#BUNDLE_LAZY_ACTIVATION} is fired.
   *    - This method returns immediately and the remaining steps will be
   *      followed when this bundle's activation is later triggered.
   * -# This bundle's state is set to \c STATE_STARTING.
   * -# A bundle event of type {@link BundleEvent#BUNDLE_STARTING} is fired.
   * -# If the bundle is contained in a shared library, the library is loaded
   *    and the {@link BundleActivator#Start(BundleContext)}
   *    method of this bundle's \c BundleActivator (if one is specified) is
   *    called. If the shared library could not be loaded, or the \c BundleActivator
   *    is invalid or throws an exception then:
   *    - This bundle's state is set to \c STATE_STOPPING.
   *    - A bundle event of type {@link BundleEvent#BUNDLE_STOPPING} is fired.
   *    - %Any services registered by this bundle are unregistered.
   *    - %Any services used by this bundle are released.
   *    - %Any listeners registered by this bundle are removed.
   *    - This bundle's state is set to \c STATE_RESOLVED.
   *    - A bundle event of type {@link BundleEvent#BUNDLE_STOPPED} is fired.
   *    - A \c std::runtime_error exception is then thrown.
   * -# If this bundle's state is \c STATE_UNINSTALLED, because this bundle
   *    was uninstalled while the \c BundleActivator#Start method was
   *    running, a \c std::logic_error is thrown.
   * -# This bundle's state is set to \c STATE_ACTIVE.
   * -# A bundle event of type {@link BundleEvent#BUNDLE_STARTED} is fired.
   *
   *
   * <b>Preconditions</b>
   * -# \c GetState() in { \c STATE_INSTALLED, \c STATE_RESOLVED }
   *    or { \c STATE_INSTALLED, \c STATE_RESOLVED, \c STATE_STARTING }
   *    if this bundle has a lazy activation policy.
   *
   * <b>Postconditions, no exceptions thrown </b>
   * -# Bundle autostart setting is modified unless the
   *    {@link #START_TRANSIENT} option was set.
   * -# \c GetState() in { \c STATE_ACTIVE } unless the
   *    lazy activation policy was used.
   * -# \c BundleActivator#Start() has been called and did not throw an
   *    exception unless the lazy activation policy was used.
   *
   * <b>Postconditions, when an exception is thrown </b>
   * -# Depending on when the exception occurred, the bundle autostart setting is
   *    modified unless the {@link #START_TRANSIENT} option was set.
   * -# \c GetState() not in { \c STATE_STARTING, \c STATE_ACTIVE }.
   *
   *
   * @param options The options for starting this bundle. See
   *        {@link #START_TRANSIENT} and {@link #START_ACTIVATION_POLICY}. The
   *        Framework ignores unrecognized options.
   * @throws std::runtime_error If this bundle could not be started.
   * @throws std::logic_error If this bundle has been uninstalled or this
   *         bundle tries to change its own state.
   */
  void Start(uint32_t options);

  /**
   * Starts this bundle with no options.
   *
   * This method performs the same function as calling \c Start(0).
   *
   * @throws std::runtime_error If this bundle could not be started.
   * @throws std::logic_error If this bundle has been uninstalled or this
   *         bundle tries to change its own state.
   * @see #Start(uint32_t)
   */
  void Start();

  /**
   * Stops this bundle.
   *
   * The following steps are executed when stopping a bundle:
   * -# If this bundle's state is \c STATE_UNINSTALLED then a
   *    \c std::logic_error is thrown.
   * -# If this bundle is in the process of being activated or deactivated
   *    then this method waits for activation or deactivation to complete
   *    before continuing. If this does not occur in a reasonable time, a
   *    \c std::runtime_error is thrown to indicate this bundle was unable to
   *    be stopped.
   * -# If the {@link #STOP_TRANSIENT} option is not set then set this
   *    bundle's persistent autostart setting to <em>Stopped</em>. When the
   *    Framework is restarted and this bundle's autostart setting is
   *    <em>Stopped</em>, this bundle will not be automatically started.
   * -# If this bundle's state is not \c STATE_STARTING or \c STATE_ACTIVE then
   *    this method returns immediately.
   * -# This bundle's state is set to \c STATE_STOPPING.
   * -# A bundle event of type {@link BundleEvent#BUNDLE_STOPPING} is fired.
   * -# If this bundle's state was \c STATE_ACTIVE prior to setting the state
   *    to \c STATE_STOPPING, the {@link BundleActivator#Stop(BundleContext)}
   *    method of this bundle's \c BundleActivator, if one is specified, is
   *    called. If that method throws an exception, this method continues to
   *    stop this bundle and a std::runtime_error is thrown after
   *    completion of the remaining steps.
   * -# %Any services registered by this bundle are unregistered.
   * -# %Any services used by this bundle are released.
   * -# %Any listeners registered by this bundle are removed.
   * -# If this bundle's state is \c STATE_UNINSTALLED, because this bundle
   *    was uninstalled while the \c BundleActivator#Stop method was
   *    running, a std::runtime_error is thrown.
   * -# This bundle's state is set to \c STATE_RESOLVED.
   * -# A bundle event of type {@link BundleEvent#BUNDLE_STOPPED} is fired.
   *
   * <b>Preconditions </b>
   * -# \c GetState() in { \c STATE_ACTIVE }.
   *
   * <b>Postconditions, no exceptions thrown </b>
   * -# Bundle autostart setting is modified unless the
   *    {@link #STOP_TRANSIENT} option was set.
   * -# \c GetState() not in { \c STATE_ACTIVE, \c STATE_STOPPING }.
   * -# \c BundleActivator#Stop has been called and did not throw an
   *    exception.
   *
   * <b>Postconditions, when an exception is thrown </b>
   * -# Bundle autostart setting is modified unless the
   *    {@link #STOP_TRANSIENT} option was set.
   *
   * @param options The options for stopping this bundle. See
   *        {@link #STOP_TRANSIENT}. The Framework ignores unrecognized
   *        options.
   * @throws std::runtime_error If the bundle failed to stop.
   * @throws std::logic_error If this bundle has been uninstalled or this
   *         bundle tries to change its own state.
   */
  void Stop(uint32_t options);

  /**
   * Stops this bundle with no options.
   *
   * This method performs the same function as calling \c Stop(0).
   *
   * @throws std::runtime_error If the bundle failed to stop.
   * @throws std::logic_error If this bundle has been uninstalled or this
   *         bundle tries to change its own state.
   * @see #Stop(uint32_t)
   */
  void Stop();

  /**
   * Uninstalls this bundle.
   *
   * This method causes the Framework to notify other bundles that this bundle
   * is being uninstalled, and then puts this bundle into the
   * \c STATE_UNINSTALLED state. The Framework removes any resources
   * related to this bundle that it is able to remove.
   *
   * The following steps are executed to uninstall a bundle:
   * -# If this bundle's state is \c STATE_UNINSTALLED, then a
   *    std::logic_error is thrown.
   * -# If this bundle's state is \c STATE_ACTIVE, \c STATE_STARTING or
   *    \c STATE_STOPPING, this bundle is stopped as described in the
   *    \c Bundle#Stop method. If \c Bundle#Stop throws an exception, a
   *    Framework event of type {@link FrameworkEvent#FRAMEWORK_ERROR} is fired containing
   *    the exception.
   * -# This bundle's state is set to \c STATE_UNINSTALLED.
   * -# A bundle event of type {@link BundleEvent#BUNDLE_UNINSTALLED} is fired.
   * -# This bundle and any persistent storage area provided for this bundle
   *    by the Framework are removed.
   *
   * <b>Preconditions </b>
   * - \c GetState() not in { \c STATE_UNINSTALLED }.
   *
   * <b>Postconditions, no exceptions thrown </b>
   * - \c GetState() in { \c STATE_UNINSTALLED }.
   * - This bundle has been uninstalled.
   *
   * <b>Postconditions, when an exception is thrown </b>
   * - \c GetState() not in { \c STATE_UNINSTALLED }.
   * - This Bundle has not been uninstalled.
   *
   * @throws std::runtime_error If the uninstall failed. This can occur if
   *         another thread is attempting to change this bundle's state and
   *         does not complete in a timely manner.
   * @throws std::logic_error If this bundle has been uninstalled or this
   *         bundle tries to change its own state.
   * @see #Stop()
   */
  void Uninstall();

protected:
  Bundle(const std::shared_ptr<BundlePrivate>& d);

  std::shared_ptr<BundlePrivate> d;
  std::shared_ptr<CoreBundleContext> c;

  friend class BundleRegistry;
  friend Bundle MakeBundle(const std::shared_ptr<BundlePrivate>&);
  friend std::shared_ptr<BundlePrivate> GetPrivate(const Bundle&);
};

/**
 * \ingroup MicroServices
 * \ingroup gr_bundle
 *
 * Streams a textual representation of ``bundle`` into the stream ``os``.
 */
US_Framework_EXPORT std::ostream& operator<<(std::ostream& os,
                                             const Bundle& bundle);
/**
 * \ingroup MicroServices
 * \ingroup gr_bundle
 *
 * This is the same as calling ``os << *bundle``.
 */
US_Framework_EXPORT std::ostream& operator<<(std::ostream& os,
                                             Bundle const* bundle);
/**
 * \ingroup MicroServices
 * \ingroup gr_bundle
 *
 * Streams a textual representation of the bundle state enumeration.
 */
US_Framework_EXPORT std::ostream& operator<<(std::ostream& os,
                                             Bundle::State state);
}

#endif // CPPMICROSERVICES_BUNDLE_H

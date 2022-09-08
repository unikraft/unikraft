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

#ifndef CPPMICROSERVICES_BUNDLEACTIVATOR_H
#define CPPMICROSERVICES_BUNDLEACTIVATOR_H

#ifndef US_BUNDLE_NAME
#  error Missing US_BUNDLE_NAME preprocessor define
#endif

#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/GlobalConfig.h"

namespace cppmicroservices {

class BundleContext;

/**
 * \ingroup MicroServices
 *
 * Customizes the starting and stopping of a CppMicroServices bundle.
 * <p>
 * <code>%BundleActivator</code> is an interface that can be implemented by
 * CppMicroServices bundles. The CppMicroServices library can create instances of a
 * bundle's <code>%BundleActivator</code> as required. If an instance's
 * <code>BundleActivator::Start</code> method executes successfully, it is
 * guaranteed that the same instance's <code>%BundleActivator::Stop</code>
 * method will be called when the bundle is to be stopped. The CppMicroServices
 * library does not concurrently call a <code>%BundleActivator</code> object.
 *
 * <p>
 * <code>%BundleActivator</code> is an abstract class interface whose implementations
 * must be exported via a special macro. Implementations are usually declared
 * and defined directly in .cpp files.
 *
 * <p>
 * \snippet uServices-activator/main.cpp 0
 *
 * <p>
 * The class implementing the <code>%BundleActivator</code> interface must have a public
 * default constructor so that a <code>%BundleActivator</code>
 * object can be created by the CppMicroServices library.
 *
 * \rststar
 * .. note::
 *
 *    A bundle activator needs to be *exported* by using the
 *    :any:`CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR` macro.
 *    The bundle *manifest.json* :any:`resource <concept-resources>`
 *    also needs to contain a ::
 *
 *       "bundle.activator" : true
 *
 *    element.
 * \endrststar
 */
struct BundleActivator
{

  virtual ~BundleActivator() {}

  /**
   * Called when this bundle is started. This method
   * can be used to register services or to allocate any resources that this
   * bundle may need globally (during the whole bundle lifetime).
   *
   * <p>
   * This method must complete and return to its caller in a timely manner.
   *
   * @param context The execution context of the bundle being started.
   * @throws std::exception If this method throws an exception, this
   *         bundle is marked as stopped and the framework will remove this
   *         bundle's listeners, unregister all services registered by this
   *         bundle, and release all services used by this bundle.
   */
  virtual void Start(BundleContext context) = 0;

  /**
   * Called when this bundle is stopped. In general, this
   * method should undo the work that the <code>BundleActivator::Start</code>
   * method started. There should be no active threads that were started by
   * this bundle when this method returns.
   *
   * <p>
   * This method must complete and return to its caller in a timely manner.
   *
   * @param context The execution context of the bundle being stopped.
   * @throws std::exception If this method throws an exception, the
   *         bundle is still marked as stopped, and the framework will remove
   *         the bundle's listeners, unregister all services registered by the
   *         bundle, and release all services used by the bundle.
   */
  virtual void Stop(BundleContext context) = 0;
};
}

/**
 * \ingroup gr_macros
 *
 * \brief Export a bundle activator class.
 *
 * \param _activator_type The fully-qualified type-name of the bundle activator class.
 *
 * Call this macro after the definition of your bundle activator to make it
 * accessible by the CppMicroServices library.
 *
 * Example:
 * \snippet uServices-activator/main.cpp 0
 */
#define CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(_activator_type)              \
  extern "C" US_ABI_EXPORT cppmicroservices::BundleActivator*                  \
    US_CREATE_ACTIVATOR_FUNC(US_BUNDLE_NAME)()                                 \
  {                                                                            \
    return new _activator_type();                                              \
  }                                                                            \
  extern "C" US_ABI_EXPORT void US_DESTROY_ACTIVATOR_FUNC(US_BUNDLE_NAME)(     \
    cppmicroservices::BundleActivator * activator)                             \
  {                                                                            \
    delete activator;                                                          \
  }

#endif /* CPPMICROSERVICES_BUNDLEACTIVATOR_H */

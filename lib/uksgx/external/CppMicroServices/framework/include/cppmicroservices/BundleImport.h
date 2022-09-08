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

#ifndef CPPMICROSERVICES_BUNDLEIMPORT_H
#define CPPMICROSERVICES_BUNDLEIMPORT_H

#include "cppmicroservices/GlobalConfig.h"

#include <string>

namespace cppmicroservices {

struct BundleActivator;
class BundleContext;
class BundleContextPrivate;
}

/**

\defgroup gr_macros Macros

\brief Preprocessor macros.

*/

/**
 * \ingroup MicroServices
 * \ingroup gr_macros
 *
 * \brief Initialize a static bundle.
 *
 * \param _bundle_name The name of the bundle to initialize.
 *
 * This macro initializes the static bundle named \c _bundle_name.
 *
 * If the bundle provides an activator, use the #CPPMICROSERVICES_IMPORT_BUNDLE
 * macro instead, to ensure that the activator is referenced and can be called.
 * Do not forget to actually link the static bundle to the importing executable or
 * shared library.
 *
 * \rststar
 * .. seealso::
 *
 *    | :any:`CPPMICROSERVICES_IMPORT_BUNDLE`
 *    | :any:`concept-static-bundles`
 * \endrststar
 */
#define CPPMICROSERVICES_INITIALIZE_STATIC_BUNDLE(_bundle_name)                \
  extern "C" cppmicroservices::BundleContext* US_GET_CTX_FUNC(_bundle_name)(); \
  extern "C" void US_SET_CTX_FUNC(_bundle_name)(                               \
    cppmicroservices::BundleContextPrivate*);                                  \
  void _dummy_reference_to_##_bundle_name##_bundle_context()                   \
  {                                                                            \
    US_GET_CTX_FUNC(_bundle_name)();                                           \
    US_SET_CTX_FUNC(_bundle_name)(nullptr);                                    \
  }

/**
 * \ingroup gr_macros
 *
 * \brief Import a static bundle.
 *
 * \param _bundle_name The name of the bundle to import.
 *
 * This macro imports the static bundle named \c _bundle_name.
 *
 * Inserting this macro into your application's source code will allow you to make use of
 * a static bundle. It will initialize the static bundle and reference its
 * BundleActivator. If the bundle does not provide an activator, use the
 * #CPPMICROSERVICES_INITIALIZE_STATIC_BUNDLE macro instead. Do not forget to actually link
 * the static bundle to the importing executable or shared library.
 *
 * Example:
 * \snippet uServices-staticbundles/main.cpp ImportStaticBundleIntoMain
 *
 * \rststar
 * .. seealso::
 *
 *    | :any:`CPPMICROSERVICES_INITIALIZE_STATIC_BUNDLE`
 *    | :any:`concept-static-bundles`
 * \endrststar
 */
#define CPPMICROSERVICES_IMPORT_BUNDLE(_bundle_name)                           \
  CPPMICROSERVICES_INITIALIZE_STATIC_BUNDLE(_bundle_name)                      \
  extern "C" cppmicroservices::BundleActivator* US_CREATE_ACTIVATOR_FUNC(      \
    _bundle_name)();                                                           \
  extern "C" void US_DESTROY_ACTIVATOR_FUNC(_bundle_name)(                     \
    cppmicroservices::BundleActivator*);                                       \
  void _dummy_reference_to_##_bundle_name##_activator()                        \
  {                                                                            \
    auto dummyActivator = US_CREATE_ACTIVATOR_FUNC(_bundle_name)();            \
    US_DESTROY_ACTIVATOR_FUNC(_bundle_name)(dummyActivator);                   \
  }

#endif // CPPMICROSERVICES_BUNDLEIMPORT_H

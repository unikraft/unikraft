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

#ifndef CPPMICROSERVICES_GETBUNDLECONTEXT_H
#define CPPMICROSERVICES_GETBUNDLECONTEXT_H

#ifndef US_BUNDLE_NAME
#  error Missing preprocessor define US_BUNDLE_NAME
#endif

#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/BundleInitialization.h"

extern "C" cppmicroservices::BundleContextPrivate* US_GET_CTX_FUNC(
  US_BUNDLE_NAME)();

namespace cppmicroservices {

namespace detail {

US_Framework_EXPORT BundleContext MakeBundleContext(BundleContextPrivate* d);
}

/**
 * \ingroup MicroServices
 *
 * \brief Returns the bundle context of the calling bundle.
 *
 * This function allows easy access to the BundleContext instance from
 * inside a C++ Micro Services bundle.
 *
 * \return The BundleContext of the calling bundle. If the caller is not
 * part of an active bundle, an invalid BundleContext is returned.
 */
static inline BundleContext GetBundleContext()
{
  auto ctx = US_GET_CTX_FUNC(US_BUNDLE_NAME)();
  return ctx ? detail::MakeBundleContext(ctx) : BundleContext{};
}
}

#endif // CPPMICROSERVICES_GETBUNDLECONTEXT_H

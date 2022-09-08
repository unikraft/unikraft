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

#ifndef CPPMICROSERVICES_BUNDLEHOOKS_H
#define CPPMICROSERVICES_BUNDLEHOOKS_H

#include "ServiceListeners.h"

#include <memory>
#include <vector>

namespace cppmicroservices {

class CoreBundleContext;
class Bundle;
class BundleContext;
class BundleEvent;

class BundleHooks
{

private:
  CoreBundleContext* const coreCtx;

public:
  BundleHooks(CoreBundleContext* ctx);

  Bundle FilterBundle(const BundleContext& context, const Bundle& bundle) const;

  void FilterBundles(const BundleContext& context,
                     std::vector<Bundle>& bundles) const;

  void FilterBundleEventReceivers(
    const BundleEvent& evt,
    ServiceListeners::BundleListenerMap& bundleListeners);
};
}

#endif // CPPMICROSERVICES_BUNDLEHOOKS_H

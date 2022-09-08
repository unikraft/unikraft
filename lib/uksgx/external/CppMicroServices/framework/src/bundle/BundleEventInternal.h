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

#ifndef CPPMICROSERVICES_BUNDLEEVENTINTERNAL_H
#define CPPMICROSERVICES_BUNDLEEVENTINTERNAL_H

#include "cppmicroservices/BundleEvent.h"

#include "BundlePrivate.h"

#include <memory>

namespace cppmicroservices {

class BundlePrivate;

struct BundleEventInternal
{
  BundleEventInternal(BundleEvent::Type t,
                      std::shared_ptr<BundlePrivate> const& b)
    : type(t)
    , bundle(b)
  {}

  BundleEvent::Type type;
  std::shared_ptr<BundlePrivate> bundle;
};

inline BundleEvent MakeBundleEvent(const BundleEventInternal& be)
{
  return BundleEvent(be.type, MakeBundle(be.bundle));
}
}

#endif // CPPMICROSERVICES_BUNDLEEVENTINTERNAL_H

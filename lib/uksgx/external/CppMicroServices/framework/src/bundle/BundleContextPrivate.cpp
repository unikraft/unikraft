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

#include "BundleContextPrivate.h"

#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleContext.h"

#include "BundlePrivate.h"

#include <stdexcept>

namespace cppmicroservices {

namespace detail {

US_Framework_EXPORT BundleContext MakeBundleContext(BundleContextPrivate* d)
{
  return ::cppmicroservices::MakeBundleContext(d);
}
}

BundleContext MakeBundleContext(BundleContextPrivate* d)
{
  return BundleContext(d->shared_from_this());
}

BundleContext MakeBundleContext(const std::shared_ptr<BundleContextPrivate>& d)
{
  return BundleContext(d);
}

std::shared_ptr<BundleContextPrivate> GetPrivate(const BundleContext& c)
{
  return c.d;
}

BundleContextPrivate::BundleContextPrivate(BundlePrivate* bundle)
  : bundle(bundle)
  , valid(true)
{}

bool BundleContextPrivate::IsValid() const
{
  return valid;
}

void BundleContextPrivate::CheckValid() const
{
  if (!valid) {
    throw std::runtime_error("The bundle context is no longer valid");
  }
}

void BundleContextPrivate::Invalidate()
{
  valid = false;
  if (bundle->SetBundleContext)
    bundle->SetBundleContext(nullptr);
}
}

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

#include "cppmicroservices/Framework.h"

#include "cppmicroservices/FrameworkEvent.h"

#include "FrameworkPrivate.h"

namespace cppmicroservices {

namespace {

FrameworkPrivate* pimpl(const std::shared_ptr<BundlePrivate>& p)
{
  return static_cast<FrameworkPrivate*>(p.get());
}
}

Framework::Framework(const Framework& fw)
  : Bundle(fw)
{}

Framework::Framework(Framework&& fw)
  : Bundle(std::move(fw))
{}

Framework& Framework::operator=(const Framework& fw)
{
  Bundle::operator=(fw);
  return *this;
}

Framework& Framework::operator=(Framework&& fw)
{
  Bundle::operator=(std::move(fw));
  return *this;
}

Framework::Framework(Bundle b)
  : Bundle(std::move(b))
{
  if (GetBundleId() != 0) {
    throw std::logic_error("Not a framework bundle");
  }
}

Framework::Framework(const std::shared_ptr<FrameworkPrivate>& d)
  : Bundle(d)
{}

void Framework::Init()
{
  pimpl(d)->Init();
}

FrameworkEvent Framework::WaitForStop(const std::chrono::milliseconds& timeout)
{
  return pimpl(d)->WaitForStop(timeout);
}
}

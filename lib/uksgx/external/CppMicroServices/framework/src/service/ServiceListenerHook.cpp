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

#include "cppmicroservices/ServiceListenerHook.h"

#include "cppmicroservices/BundleContext.h"

#include "BundleContextPrivate.h"
#include "ServiceListenerHookPrivate.h"

namespace cppmicroservices {

ServiceListenerHook::~ServiceListenerHook() {}

ServiceListenerHook::ListenerInfoData::ListenerInfoData(
  const std::shared_ptr<BundleContextPrivate>& context,
  const ServiceListener& l,
  void* data,
  ListenerTokenId tokenId,
  const std::string& filter)
  : context(context)
  , listener(l)
  , data(data)
  , tokenId(tokenId)
  , filter(filter)
  , bRemoved(false)
{}

ServiceListenerHook::ListenerInfoData::~ListenerInfoData() {}

ServiceListenerHook::ListenerInfo::ListenerInfo(ListenerInfoData* data)
  : d(data)
{}

ServiceListenerHook::ListenerInfo::ListenerInfo()
  : d(nullptr)
{}

ServiceListenerHook::ListenerInfo::ListenerInfo(const ListenerInfo& other)
  : d(other.d)
{}

ServiceListenerHook::ListenerInfo::~ListenerInfo() {}

ServiceListenerHook::ListenerInfo& ServiceListenerHook::ListenerInfo::operator=(
  const ListenerInfo& other)
{
  d = other.d;
  return *this;
}

bool ServiceListenerHook::ListenerInfo::IsNull() const
{
  return !d;
}

BundleContext ServiceListenerHook::ListenerInfo::GetBundleContext() const
{
  return MakeBundleContext(d->context);
}

std::string ServiceListenerHook::ListenerInfo::GetFilter() const
{
  return d->filter;
}

bool ServiceListenerHook::ListenerInfo::IsRemoved() const
{
  return d->bRemoved;
}

bool ServiceListenerHook::ListenerInfo::operator==(
  const ListenerInfo& other) const
{
  return d == other.d;
}
}

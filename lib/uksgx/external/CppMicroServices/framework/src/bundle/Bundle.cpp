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

#include "cppmicroservices/Bundle.h"

#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleResource.h"
#include "cppmicroservices/FrameworkConfig.h"
#include "cppmicroservices/FrameworkEvent.h"
#include "cppmicroservices/SharedLibrary.h"

#include "BundleArchive.h"
#include "BundleContextPrivate.h"
#include "BundlePrivate.h"
#include "BundleThread.h"
#include "BundleUtils.h"
#include "CoreBundleContext.h"
#include "Resolver.h"
#include "Utils.h"

#include <chrono>
#include <thread>

namespace cppmicroservices {

Bundle::Bundle(const Bundle& b)
  : d(b.d)
  , c(b.c)
{}

Bundle::Bundle(Bundle&& b)
  : d(std::move(b.d))
  , c(std::move(b.c))
{}

Bundle& Bundle::operator=(const Bundle& b)
{
  this->d = b.d;
  this->c = b.c;
  return *this;
}

Bundle& Bundle::operator=(Bundle&& b)
{
  this->d = std::move(b.d);
  this->c = std::move(b.c);
  return *this;
}

Bundle::Bundle() {}

bool Bundle::operator==(const Bundle& rhs) const
{
  return *this
           ? (rhs ? d->coreCtx->id == rhs.d->coreCtx->id && d->id == rhs.d->id
                  : false)
           : !rhs;
}

bool Bundle::operator!=(const Bundle& rhs) const
{
  return !(*this == rhs);
}

bool Bundle::operator<(const Bundle& rhs) const
{
  return *this ? (rhs ? (d->coreCtx->id == rhs.d->coreCtx->id
                           ? d->id < rhs.d->id
                           : d->coreCtx->id < rhs.d->coreCtx->id)
                      : true)
               : false;
}

Bundle::operator bool() const
{
  return d != nullptr;
}

Bundle& Bundle::operator=(std::nullptr_t)
{
  d = nullptr;
  c = nullptr;
  return *this;
}

Bundle::Bundle(const std::shared_ptr<BundlePrivate>& d)
  : d(d)
  , c(d ? d->coreCtx->shared_from_this() : nullptr)
{}

Bundle::~Bundle() {}

Bundle::State Bundle::GetState() const
{
  return static_cast<State>(d->state.load());
}

void Bundle::Start()
{
  d->Start(0);
}

void Bundle::Start(uint32_t options)
{
  d->Start(options);
}

void Bundle::Stop()
{
  Stop(0);
}

void Bundle::Stop(uint32_t options)
{
  d->Stop(options);
}

void Bundle::Uninstall()
{
  d->Uninstall();
}

BundleContext Bundle::GetBundleContext() const
{
  return MakeBundleContext(d->bundleContext.Load());
}

long Bundle::GetBundleId() const
{
  return d->id;
}

std::string Bundle::GetLocation() const
{
  return d->GetLocation();
}

std::string Bundle::GetSymbolicName() const
{
  return d->symbolicName;
}

BundleVersion Bundle::GetVersion() const
{
  return d->version;
}

std::map<std::string, Any> Bundle::GetProperties() const
{
  return d->bundleManifest.GetPropertiesDeprecated();
}

AnyMap Bundle::GetHeaders() const
{
  return d->GetHeaders();
}

Any Bundle::GetProperty(const std::string& key) const
{
  Any property(d->bundleManifest.GetValueDeprecated(key));

  // Clients must be able to query both a bundle's properties
  // and the framework's properties through any Bundle's
  // GetProperty function.
  // The Framework's properties include both the launch properties
  // used to initialize the Framework and all relevant
  // "org.cppmicroservices.*" properties.
  if (property.Empty()) {
    auto props = d->coreCtx->frameworkProperties.find(key);
    if (props != d->coreCtx->frameworkProperties.end()) {
      property = (*props).second;
    }
  }
  return property;
}

std::vector<std::string> Bundle::GetPropertyKeys() const
{
  return d->bundleManifest.GetKeysDeprecated();
}

std::vector<ServiceReferenceU> Bundle::GetRegisteredServices() const
{
  d->CheckUninstalled();
  std::vector<ServiceRegistrationBase> sr;
  std::vector<ServiceReferenceU> res;
  d->coreCtx->services.GetRegisteredByBundle(d.get(), sr);
  for (std::vector<ServiceRegistrationBase>::const_iterator i = sr.begin();
       i != sr.end();
       ++i) {
    res.push_back(i->GetReference());
  }
  return res;
}

std::vector<ServiceReferenceU> Bundle::GetServicesInUse() const
{
  d->CheckUninstalled();
  std::vector<ServiceRegistrationBase> sr;
  std::vector<ServiceReferenceU> res;
  d->coreCtx->services.GetUsedByBundle(d.get(), sr);
  for (std::vector<ServiceRegistrationBase>::const_iterator i = sr.begin();
       i != sr.end();
       ++i) {
    res.push_back(i->GetReference());
  }
  return res;
}

BundleResource Bundle::GetResource(const std::string& path) const
{
  d->CheckUninstalled();
  return d->barchive ? d->barchive->GetResource(path) : BundleResource();
}

std::vector<BundleResource> Bundle::FindResources(
  const std::string& path,
  const std::string& filePattern,
  bool recurse) const
{
  d->CheckUninstalled();
  return d->barchive ? d->barchive->FindResources(path, filePattern, recurse)
                     : std::vector<BundleResource>();
}

Bundle::TimeStamp Bundle::GetLastModified() const
{
  return d->barchive ? d->barchive->GetLastModified() : d->timeStamp;
}

std::ostream& operator<<(std::ostream& os, Bundle::State state)
{
  switch (state) {
    case Bundle::STATE_UNINSTALLED:
      return os << "UNINSTALLED";
    case Bundle::STATE_INSTALLED:
      return os << "INSTALLED";
    case Bundle::STATE_RESOLVED:
      return os << "RESOLVED";
    case Bundle::STATE_STARTING:
      return os << "STARTING";
    case Bundle::STATE_ACTIVE:
      return os << "ACTIVE";
    case Bundle::STATE_STOPPING:
      return os << "STOPPING";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Bundle& bundle)
{
  os << "Bundle["
     << "id=" << bundle.GetBundleId() << ", loc=" << bundle.GetLocation()
     << ", name=" << bundle.GetSymbolicName() << ", state=" << bundle.GetState()
     << "]";
  return os;
}

std::ostream& operator<<(std::ostream& os, Bundle const* bundle)
{
  return operator<<(os, *bundle);
}
}

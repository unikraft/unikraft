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

#include "FooService.h"

#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/ServiceTracker.h"
#include "cppmicroservices/ServiceTrackerCustomizer.h"

#include "BundlePropsInterface.h"

namespace cppmicroservices {

class SL1BundlePropsImpl : public BundlePropsInterface
{

public:
  const Properties& GetProperties() const { return props; }

  void SetProperty(std::string propertyKey, bool propertyValue)
  {
    props[propertyKey] = propertyValue;
  }

private:
  BundlePropsInterface::Properties props;
};

class SL1ServiceTrackerCustomizer : public ServiceTrackerCustomizer<FooService>
{
private:
  std::shared_ptr<SL1BundlePropsImpl> bundlePropsService;
  BundleContext context;

public:
  SL1ServiceTrackerCustomizer(std::shared_ptr<SL1BundlePropsImpl> propService,
                              const BundleContext& bc)
    : bundlePropsService(propService)
    , context(bc)
  {}

  virtual ~SL1ServiceTrackerCustomizer() { context = nullptr; }

  std::shared_ptr<FooService> AddingService(
    const ServiceReference<FooService>& reference)
  {
    bundlePropsService->SetProperty("serviceAdded", true);

    std::shared_ptr<FooService> fooService =
      context.GetService<FooService>(reference);
    fooService->foo();
    return fooService;
  }

  void ModifiedService(const ServiceReference<FooService>& /*reference*/,
                       const std::shared_ptr<FooService>& /*service*/)
  {}

  void RemovedService(const ServiceReference<FooService>& /*reference*/,
                      const std::shared_ptr<FooService>& /*service*/)
  {
    bundlePropsService->SetProperty("serviceRemoved", true);
  }
};

class ActivatorSL1 : public BundleActivator
{

public:
  ActivatorSL1()
    : bundlePropsService(std::make_shared<SL1BundlePropsImpl>())
    , trackerCustomizer(nullptr)
    , tracker(nullptr)
    , context()
  {}

  ~ActivatorSL1() {}

  void Start(BundleContext context)
  {
    this->context = context;
    InterfaceMapPtr im =
      MakeInterfaceMap<BundlePropsInterface>(bundlePropsService);
    im->insert(std::make_pair(std::string("ActivatorSL1"), bundlePropsService));
    sr = context.RegisterService(im);
    trackerCustomizer.reset(
      new SL1ServiceTrackerCustomizer(bundlePropsService, context));
    tracker.reset(new FooTracker(context, trackerCustomizer.get()));
    tracker->Open();
  }

  void Stop(BundleContext /*context*/)
  {
    tracker->Close();
    tracker.reset();
    trackerCustomizer.reset();
    this->context = nullptr;
  }

private:
  std::shared_ptr<SL1BundlePropsImpl> bundlePropsService;

  ServiceRegistrationU sr;

  typedef ServiceTracker<FooService> FooTracker;
  std::unique_ptr<SL1ServiceTrackerCustomizer> trackerCustomizer;
  std::unique_ptr<FooTracker> tracker;
  BundleContext context;

}; // ActivatorSL1
}

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(cppmicroservices::ActivatorSL1)

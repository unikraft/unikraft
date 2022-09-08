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
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/PrototypeServiceFactory.h"
#include "cppmicroservices/ServiceFactory.h"
#include "cppmicroservices/ServiceInterface.h"

#include <iostream>
#include <mutex>
#include <thread>

namespace cppmicroservices {

struct TestBundleH
{
  virtual ~TestBundleH() {}
};

struct TestBundleH2
{
  virtual ~TestBundleH2() {}
};

struct TestBundleH3
{
  virtual ~TestBundleH3() {}
};

class TestProduct : public TestBundleH
{
  // Bundle caller;

public:
  TestProduct(const Bundle& /*caller*/)
  //: caller(caller)
  {}
};

class TestProduct2
  : public TestProduct
  , public TestBundleH2
{
public:
  TestProduct2(const Bundle& caller)
    : TestProduct(caller)
  {}
};

class FakeTestProduct : public TestBundleH3
{
public:
  FakeTestProduct() {}
};

class TestBundleHPrototypeServiceFactory : public PrototypeServiceFactory
{
public:
  InterfaceMapConstPtr GetService(const Bundle& caller,
                                  const ServiceRegistrationBase& /*sReg*/)
  {
    return MakeInterfaceMap<TestBundleH, TestBundleH2>(
      std::make_shared<TestProduct2>(caller));
  }

  void UngetService(const Bundle& /*caller*/,
                    const ServiceRegistrationBase& /*sReg*/,
                    const InterfaceMapConstPtr& /*service*/)
  {}
};

class TestBundleHServiceFactory : public ServiceFactory
{
public:
  InterfaceMapConstPtr GetService(const Bundle& caller,
                                  const ServiceRegistrationBase& /*sReg*/)
  {
    return MakeInterfaceMap<TestBundleH>(std::make_shared<TestProduct>(caller));
  }

  void UngetService(const Bundle& /*caller*/,
                    const ServiceRegistrationBase& /*sReg*/,
                    const InterfaceMapConstPtr& /*service*/)
  {}
};

// Simulate the ServiceFactory throwing an exception
class TestBundleHServiceFactoryGetServiceThrow : public ServiceFactory
{
public:
  InterfaceMapConstPtr GetService(const Bundle& /*caller*/,
                                  const ServiceRegistrationBase& /*sReg*/)
  {
    throw std::runtime_error(
      "Test exception thrown from TestBundleHServiceFactoryThrow::GetService");
  }

  void UngetService(const Bundle& /*caller*/,
                    const ServiceRegistrationBase& /*sReg*/,
                    const InterfaceMapConstPtr& /*service*/)
  {}
};

// Simulate the ServiceFactory throwing an exception
class TestBundleHServiceFactoryUngetServiceThrow : public ServiceFactory
{
public:
  InterfaceMapConstPtr GetService(const Bundle& caller,
                                  const ServiceRegistrationBase& /*sReg*/)
  {
    std::shared_ptr<TestProduct> product =
      std::make_shared<TestProduct>(caller);
    return MakeInterfaceMap<TestBundleH>(product);
  }

  void UngetService(const Bundle& /*caller*/,
                    const ServiceRegistrationBase& /*sReg*/,
                    const InterfaceMapConstPtr& /*service*/)
  {
    throw std::runtime_error(
      "Test exception thrown from "
      "TestBundleHServiceFactoryUngetServiceThrow::UngetService");
  }
};

// Simulate an error condition whereby the class cannot be found in the returned InterfaceMapConstPtr
class TestBundleHServiceFactoryInterfaceNotFound : public ServiceFactory
{
public:
  InterfaceMapConstPtr GetService(const Bundle& /*caller*/,
                                  const ServiceRegistrationBase& /*sReg*/)
  {
    std::shared_ptr<FakeTestProduct> product =
      std::make_shared<FakeTestProduct>();
    return MakeInterfaceMap<TestBundleH3>(product);
  }

  void UngetService(const Bundle& /*caller*/,
                    const ServiceRegistrationBase& /*sReg*/,
                    const InterfaceMapConstPtr& /*service*/)
  {}
};

// Simulates an error condition of a ServiceFactory returning a nullptr
class TestBundleHServiceFactoryReturnsNullPtr : public ServiceFactory
{
public:
  InterfaceMapConstPtr GetService(const Bundle& /* caller */,
                                  const ServiceRegistrationBase& /*sReg*/)
  {
    return nullptr;
  }

  void UngetService(const Bundle& /* caller */,
                    const ServiceRegistrationBase& /*sReg*/,
                    const InterfaceMapConstPtr& /* service */)
  {}
};

// Simulate an error condition whereby the service factory recursively tries
// to get the same service for the same bundle if called from the system bundle.
//
// If it is called from a non-system bundle, it will call GetService again,
// using a different bundle (the system bundle) which then returns a valid
// service object. This recursion is allowed.
class TestBundleHServiceFactoryRecursion : public ServiceFactory
{
public:
  InterfaceMapConstPtr GetService(const Bundle& caller,
                                  const ServiceRegistrationBase& sReg)
  {
    static bool validRecursion = false;

    // If not called from the framework, we call GetService again, but using a
    // different bundle context. This is required to work.
    if (caller.GetBundleId() > 0) {
      validRecursion = true;
      return caller.GetBundleContext()
        .GetBundle(0)
        .GetBundleContext()
        .GetService(ServiceReferenceU(sReg.GetReference()));
    }

    if (validRecursion) {
      // We were called by a non-system bundle initially, and then called
      // ourselve again using a different bundle and return a valid object now.
      validRecursion = false;
      return MakeInterfaceMap<TestBundleH>(std::make_shared<TestBundleH>());
    }

    // Sleep a little, to increase the likelyhood of race conditions when
    // this service factory is used concurrently.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // This code causes infinite recursion into this function.
    return caller.GetBundleContext().GetService(
      ServiceReferenceU(sReg.GetReference()));
  }

  void UngetService(const Bundle& /*caller*/,
                    const ServiceRegistrationBase& /*sReg*/,
                    const InterfaceMapConstPtr& /*service*/)
  {}
};

class TestBundleHActivator : public BundleActivator
{
  std::string thisServiceName;
  ServiceRegistration<TestBundleH> factoryService;
  ServiceRegistration<TestBundleH> factoryServiceReturningNullPtr;
  ServiceRegistration<TestBundleH> factoryServiceReturningWrongInterface;
  ServiceRegistration<TestBundleH> factoryServiceThrowFromGetService;
  ServiceRegistration<TestBundleH> factoryServiceThrowFromUngetService;
  ServiceRegistration<TestBundleH, TestBundleH2> prototypeFactoryService;
  std::shared_ptr<ServiceFactory> factoryObj;
  std::shared_ptr<ServiceFactory> factoryObjReturningNullPtr;
  std::shared_ptr<ServiceFactory> factoryObjReturningWrongInterface;
  std::shared_ptr<ServiceFactory> factoryObjThrowFromGetService;
  std::shared_ptr<ServiceFactory> factoryObjThrowFromUngetService;
  std::shared_ptr<TestBundleHPrototypeServiceFactory> prototypeFactoryObj;

public:
  TestBundleHActivator()
    : thisServiceName(us_service_interface_iid<TestBundleH>())
  {}

  void Start(BundleContext context)
  {
    factoryObj = std::make_shared<TestBundleHServiceFactory>();
    factoryService =
      context.RegisterService<TestBundleH>(ToFactory(factoryObj));

    factoryObjReturningNullPtr =
      std::make_shared<TestBundleHServiceFactoryReturnsNullPtr>();
    factoryServiceReturningNullPtr = context.RegisterService<TestBundleH>(
      ToFactory(factoryObjReturningNullPtr),
      ServiceProperties{ { std::string("returns_nullptr"), Any(true) } });

    factoryObjReturningWrongInterface =
      std::make_shared<TestBundleHServiceFactoryInterfaceNotFound>();
    factoryServiceReturningWrongInterface =
      context.RegisterService<TestBundleH>(
        ToFactory(factoryObjReturningWrongInterface),
        ServiceProperties{
          { std::string("returns_wrong_interface"), Any(true) } });

    factoryObjThrowFromGetService =
      std::make_shared<TestBundleHServiceFactoryGetServiceThrow>();
    factoryServiceThrowFromGetService = context.RegisterService<TestBundleH>(
      ToFactory(factoryObjThrowFromGetService),
      ServiceProperties{ { std::string("getservice_exception"), Any(true) } });

    factoryObjThrowFromUngetService =
      std::make_shared<TestBundleHServiceFactoryUngetServiceThrow>();
    factoryServiceThrowFromUngetService = context.RegisterService<TestBundleH>(
      ToFactory(factoryObjThrowFromUngetService),
      ServiceProperties{
        { std::string("ungetservice_exception"), Any(true) } });

    context.RegisterService<TestBundleH>(
      std::shared_ptr<ServiceFactory>(new TestBundleHServiceFactoryRecursion),
      ServiceProperties{ { std::string("getservice_recursion"), Any(true) } });

    prototypeFactoryObj =
      std::make_shared<TestBundleHPrototypeServiceFactory>();
    prototypeFactoryService =
      context.RegisterService<TestBundleH, TestBundleH2>(
        ToFactory(prototypeFactoryObj));
  }

  void Stop(BundleContext /*context*/)
  {
    factoryService.Unregister();
    factoryServiceReturningNullPtr.Unregister();
    factoryServiceReturningWrongInterface.Unregister();
    factoryServiceThrowFromGetService.Unregister();
    factoryServiceThrowFromUngetService.Unregister();
    prototypeFactoryService.Unregister();
  }
};
}

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(cppmicroservices::TestBundleHActivator)

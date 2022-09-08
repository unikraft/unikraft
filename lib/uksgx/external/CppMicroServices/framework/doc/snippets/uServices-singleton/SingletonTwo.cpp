#include "SingletonTwo.h"

#include "SingletonOne.h"

#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/GetBundleContext.h"

#include <iostream>

using namespace cppmicroservices;

SingletonTwo& SingletonTwo::GetInstance()
{
  static SingletonTwo instance;
  return instance;
}

SingletonTwo::SingletonTwo()
  : b(2)
{
  std::cout << "Constructing SingletonTwo" << std::endl;
}

SingletonTwo::~SingletonTwo()
{
  std::cout << "Deleting SingletonTwo" << std::endl;
  std::cout << "SingletonOne::a = " << SingletonOne::GetInstance().a
            << std::endl;
}

std::shared_ptr<SingletonTwoService> SingletonTwoService::GetInstance()
{
  static ServiceReference<SingletonTwoService> serviceRef;
  static auto context = GetBundleContext();

  if (!serviceRef) {
    // This is either the first time GetInstance() was called,
    // or a SingletonTwoService instance has not yet been registered.
    serviceRef = context.GetServiceReference<SingletonTwoService>();
  }

  if (serviceRef) {
    // We have a valid service reference. It always points to the service
    // with the lowest id (usually the one which was registered first).
    // This still might return a null pointer, if all SingletonTwoService
    // instances have been unregistered (during unloading of the library,
    // for example).
    return context.GetService(serviceRef);
  } else {
    // No SingletonTwoService instance was registered yet.
    return nullptr;
  }
}

SingletonTwoService::SingletonTwoService()
  : b(2)
{
  std::cout << "Constructing SingletonTwoService" << std::endl;
}

SingletonTwoService::~SingletonTwoService()
{
  std::cout << "Deleting SingletonTwoService" << std::endl;
}

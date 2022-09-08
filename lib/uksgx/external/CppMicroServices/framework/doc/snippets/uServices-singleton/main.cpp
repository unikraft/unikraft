#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"

#include "SingletonOne.h"
#include "SingletonTwo.h"

#include <iostream>

using namespace cppmicroservices;

class MyActivator : public BundleActivator
{

public:
  MyActivator()
    : m_SingletonOne(nullptr)
    , m_SingletonTwo(nullptr)
  {}

  //![0]
  void Start(BundleContext context)
  {
    // First create and register a SingletonTwoService instance.
    m_SingletonTwo = std::make_shared<SingletonTwoService>();
    m_SingletonTwoReg =
      context.RegisterService<SingletonTwoService>(m_SingletonTwo);
    // Framework service registry has shared ownership of the SingletonTwoService instance

    // Now the SingletonOneService constructor will get a valid
    // SingletonTwoService instance.
    m_SingletonOne = std::make_shared<SingletonOneService>();
    m_SingletonOneReg =
      context.RegisterService<SingletonOneService>(m_SingletonOne);
  }
  //![0]

  //![1]
  void Stop(BundleContext /*context*/)
  {
    // Services are automatically unregistered during unloading of
    // the shared library after the call to Stop(BundleContext*)
    // has returned.

    // Since SingletonOneService needs a non-null SingletonTwoService
    // instance in its destructor, we explicitly unregister and delete the
    // SingletonOneService instance here. This way, the SingletonOneService
    // destructor will still get a valid SingletonTwoService instance.
    m_SingletonOneReg.Unregister();
    m_SingletonOne.reset();
    // Deletion of the SingletonTwoService instance is handled by the smart pointer

    // For singletonTwoService, we could rely on the automatic unregistering
    // by the service registry and on automatic deletion of service
    // instances through smart pointers.
    m_SingletonTwoReg.Unregister();
    m_SingletonTwo.reset();
    // Deletion of the SingletonOneService instance is handled by the smart pointer
  }
  //![1]

private:
  std::shared_ptr<SingletonOneService> m_SingletonOne;
  std::shared_ptr<SingletonTwoService> m_SingletonTwo;

  ServiceRegistration<SingletonOneService> m_SingletonOneReg;
  ServiceRegistration<SingletonTwoService> m_SingletonTwoReg;
};

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(MyActivator)

int main()
{
  std::cout
    << "This snippet is not meant to be executed.\n"
       "It does not provide a complete working example.\n"
       "See "
       "http://docs.cppmicroservices.org/en/stable/doc/src/getting_started.html"
    << std::endl;
  return 0;
}

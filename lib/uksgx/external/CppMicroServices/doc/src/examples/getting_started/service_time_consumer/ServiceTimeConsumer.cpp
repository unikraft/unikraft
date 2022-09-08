#include <cppmicroservices/BundleActivator.h>
#include <cppmicroservices/BundleContext.h>
#include <cppmicroservices/GetBundleContext.h>

#include <ServiceTime.h>

#include <iostream>

using namespace cppmicroservices;

class ServiceTimeConsumerActivator : public BundleActivator
{
  typedef ServiceReference<ServiceTime> ServiceTimeRef;

  void Start(BundleContext ctx)
  {
    auto ref = ctx.GetServiceReference<ServiceTime>();

    PrintTime(ref);
  }

  void Stop(BundleContext)
  {
    // Nothing to do
  }

  void PrintTime(const ServiceTimeRef& ref) const
  {
    if (!ref) {
      std::cout << "ServiceTime reference invalid" << std::endl;
      return;
    }

    // We can also get the bundle context like this
    auto ctx = GetBundleContext();

    // Get the ServiceTime service
    auto svc_time = ctx.GetService(ref);
    if (!svc_time) {
      std::cout << "ServiceTime not available" << std::endl;
    } else {
      std::cout << "Elapsed: " << svc_time->elapsed().count() << "ms"
                << std::endl;
    }
  }
};

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(ServiceTimeConsumerActivator)

// [no-cmake]
// The code below is required if the CMake
// helper functions are not used.
#ifdef NO_CMAKE
CPPMICROSERVICES_INITIALIZE_BUNDLE(service_time_consumer)
#endif

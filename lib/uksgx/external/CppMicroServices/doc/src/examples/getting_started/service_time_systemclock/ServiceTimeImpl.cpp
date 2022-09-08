#include <ServiceTime.h>

#include <cppmicroservices/BundleActivator.h>

using namespace cppmicroservices;

class ServiceTimeSystemClock : public ServiceTime
{
  std::chrono::milliseconds elapsed() const
  {
    auto now = std::chrono::system_clock::now();

    // Relies on the de-facto standard of relying on
    // POSIX time in all known implementations so far.
    return std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  }
};

class ServiceTimeActivator : public BundleActivator
{
  void Start(BundleContext ctx)
  {
    auto service = std::make_shared<ServiceTimeSystemClock>();
    ctx.RegisterService<ServiceTime>(service);
  }

  void Stop(BundleContext)
  {
    // Nothing to do
  }
};

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(ServiceTimeActivator)

// [no-cmake]
// The code below is required if the CMake
// helper functions are not used.
#ifdef NO_CMAKE
CPPMICROSERVICES_INITIALIZE_BUNDLE(service_time_systemclock)
#endif

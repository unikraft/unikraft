#include <cppmicroservices/Bundle.h>
#include <cppmicroservices/BundleContext.h>
#include <cppmicroservices/BundleImport.h>
#include <cppmicroservices/Framework.h>
#include <cppmicroservices/FrameworkFactory.h>

using namespace cppmicroservices;

int main(int argc, char* argv[])
{
#ifdef US_BUILD_SHARED_LIBS
  if (argc < 2) {
    std::cout << "Pass shared libraries as command line arguments" << std::endl;
  }
#endif

  // Create a new framework with a default configuration.
  Framework fw = FrameworkFactory().NewFramework();

  try {
    // Initialize the framework, such that we can call
    // GetBundleContext() later.
    fw.Init();
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
    return 1;
  }

  // The framework inherits from the Bundle class; it is
  // itself a bundle.
  auto ctx = fw.GetBundleContext();
  if (!ctx) {
    std::cerr << "Invalid framework context" << std::endl;
    return 1;
  }

  // Install all bundles contained in the shared libraries
  // given as command line arguments.
  for (int i = 1; i < argc; ++i) {
    try {
      ctx.InstallBundles(argv[i]);
    } catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
    }
  }

  try {
    // Start the framwork itself.
    fw.Start();

    // Our bundles depend on each other in the sense that the consumer
    // bundle expects a ServiceTime service in its activator Start()
    // function. This is done here for simplicity, but is actually
    // bad practice.
    auto bundles = ctx.GetBundles();
    auto iter = std::find_if(bundles.begin(), bundles.end(), [](Bundle& b) {
      return b.GetSymbolicName() == "service_time_systemclock";
    });
    if (iter != bundles.end()) {
      iter->Start();
    }

    // Now start all bundles.
    for (auto& bundle : bundles) {
      bundle.Start();
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}

#if !defined(US_BUILD_SHARED_LIBS)
CPPMICROSERVICES_IMPORT_BUNDLE(service_time_systemclock)
CPPMICROSERVICES_IMPORT_BUNDLE(service_time_consumer)
#endif

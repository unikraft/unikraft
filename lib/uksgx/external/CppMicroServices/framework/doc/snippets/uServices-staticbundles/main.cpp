#include "cppmicroservices/GlobalConfig.h"

#include <iostream>

namespace cppmicroservices {
struct BundleActivator;
class BundleContext;
class BundleContextPrivate;
}

using namespace cppmicroservices;

// This is just for illustration purposes in code snippets
extern "C" BundleActivator* _us_create_activator_MyStaticBundle1()
{
  return nullptr;
}
extern "C" void _us_destroy_activator_MyStaticBundle1(BundleActivator*) {}
extern "C" BundleActivator* _us_create_activator_MyStaticBundle2()
{
  return nullptr;
}
extern "C" void _us_destroy_activator_MyStaticBundle2(BundleActivator*) {}
extern "C" void _us_import_bundle_initializer_MyStaticBundle1() {}
extern "C" void _us_import_bundle_initializer_MyStaticBundle2() {}
extern "C" BundleContext* _us_get_bundle_context_instance_MyStaticBundle1()
{
  return nullptr;
}
extern "C" void _us_set_bundle_context_instance_MyStaticBundle1(
  BundleContextPrivate*)
{}
extern "C" BundleContext* _us_get_bundle_context_instance_MyStaticBundle2()
{
  return nullptr;
}
extern "C" void _us_set_bundle_context_instance_MyStaticBundle2(
  BundleContextPrivate*)
{}
extern "C" BundleContext* _us_get_bundle_context_instance_main()
{
  return nullptr;
}
extern "C" void _us_set_bundle_context_instance_main(BundleContextPrivate*) {}

//! [ImportStaticBundleIntoMain]
#include "cppmicroservices/BundleImport.h"

CPPMICROSERVICES_IMPORT_BUNDLE(MyStaticBundle1)
//! [ImportStaticBundleIntoMain]

//! [ImportStaticBundleIntoMain2]
#include "cppmicroservices/BundleImport.h"

#ifndef US_BUILD_SHARED_LIBS
CPPMICROSERVICES_INITIALIZE_STATIC_BUNDLE(system_bundle)
CPPMICROSERVICES_IMPORT_BUNDLE(MyStaticBundle2)
CPPMICROSERVICES_INITIALIZE_STATIC_BUNDLE(main)
#endif
//! [ImportStaticBundleIntoMain2]

int main(int /*argc*/, char* /*argv*/ [])
{
  std::cout
    << "This snippet is not meant to be executed.\n"
       "It does not provide a complete working example.\n"
       "See "
       "http://docs.cppmicroservices.org/en/stable/doc/src/getting_started.html"
    << std::endl;
  return 0;
}

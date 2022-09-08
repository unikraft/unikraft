//! [GetBundleContext]
#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleActivator.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/GetBundleContext.h"

#include <iostream>

using namespace cppmicroservices;

void RetrieveBundleContext()
{
  auto context = GetBundleContext();
  auto bundle = context.GetBundle();
  std::cout << "Bundle name: " << bundle.GetSymbolicName()
            << " [id: " << bundle.GetBundleId() << "]\n";
}
//! [GetBundleContext]

//! [InitializeBundle]
#include "cppmicroservices/BundleInitialization.h"

CPPMICROSERVICES_INITIALIZE_BUNDLE
//! [InitializeBundle]

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

#include "IDictionaryService.h"

#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/GetBundleContext.h"
#include "cppmicroservices/ServiceReference.h"

#include <iostream>

using namespace cppmicroservices;

int main(int /*argc*/, char* /*argv*/ [])
{
  ServiceReference<IDictionaryService> dictionaryServiceRef =
    GetBundleContext().GetServiceReference<IDictionaryService>();
  if (dictionaryServiceRef) {
    auto dictionaryService =
      GetBundleContext().GetService(dictionaryServiceRef);
    if (dictionaryService) {
      std::cout << "Dictionary contains 'Tutorial': "
                << dictionaryService->CheckWord("Tutorial") << std::endl;
    }
  }
}

#include "cppmicroservices/BundleInitialization.h"

CPPMICROSERVICES_INITIALIZE_BUNDLE

#include "cppmicroservices/BundleActivator.h"

#include <iostream>

using namespace cppmicroservices;

//! [0]
class MyActivator : public BundleActivator
{

public:
  void Start(BundleContext /*context*/)
  { /* register stuff */
  }

  void Stop(BundleContext /*context*/)
  { /* cleanup */
  }
};

CPPMICROSERVICES_EXPORT_BUNDLE_ACTIVATOR(MyActivator)
//![0]

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

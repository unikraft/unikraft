#include "cppmicroservices/Bundle.h"
#include "cppmicroservices/BundleContext.h"
#include "cppmicroservices/BundleInitialization.h"
#include "cppmicroservices/BundleResource.h"
#include "cppmicroservices/BundleResourceStream.h"
#include "cppmicroservices/GetBundleContext.h"

#include <iostream>

CPPMICROSERVICES_INITIALIZE_BUNDLE

using namespace cppmicroservices;

void resourceExample()
{
  //! [1]
  // Get this bundle's Bundle object
  auto bundle = GetBundleContext().GetBundle();

  BundleResource resource = bundle.GetResource("config.properties");
  if (resource.IsValid()) {
    // Create a BundleResourceStream object
    BundleResourceStream resourceStream(resource);

    // Read the contents line by line
    std::string line;
    while (std::getline(resourceStream, line)) {
      // Process the content
      std::cout << line << std::endl;
    }
  } else {
    // Error handling
  }
  //! [1]
}

void parseComponentDefinition(std::istream&) {}

void extenderPattern(const BundleContext& bundleCtx)
{
  //! [2]
  // Check if a bundle defines a "service-component" property
  // and use its value to retrieve an embedded resource containing
  // a component description.
  for (auto const bundle : bundleCtx.GetBundles()) {
    if (bundle.GetState() == Bundle::STATE_UNINSTALLED)
      continue;
    auto headers = bundle.GetHeaders();
    auto iter = headers.find("service-component");
    std::string componentPath =
      (iter == headers.end()) ? std::string() : iter->second.ToString();
    if (!componentPath.empty()) {
      BundleResource componentResource = bundle.GetResource(componentPath);
      if (!componentResource.IsValid() || componentResource.IsDir())
        continue;

      // Create a std::istream compatible object and parse the
      // component description.
      BundleResourceStream resStream(componentResource);
      parseComponentDefinition(resStream);
    }
  }
  //! [2]
}

int main(int /*argc*/, char* /*argv*/ [])
{
  std::cout
    << "This snippet is not meant to be executed.\n"
       "It does not provide a complete working example.\n"
       "See "
       "http://docs.cppmicroservices.org/en/stable/doc/src/getting_started.html"
    << std::endl;
  return 0;

  //! [0]
  auto bundleContext = GetBundleContext();
  auto bundle = bundleContext.GetBundle();

  // List all XML files in the config directory
  std::vector<BundleResource> xmlFiles =
    bundle.FindResources("config", "*.xml", false);

  // Find the resource named vertex_shader.txt starting at the root directory
  std::vector<BundleResource> shaders =
    bundle.FindResources("", "vertex_shader.txt", true);
  //! [0]
}

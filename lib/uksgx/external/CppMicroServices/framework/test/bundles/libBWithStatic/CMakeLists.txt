
set(BUILD_SHARED_LIBS 0)
usFunctionCreateTestBundleWithResources(TestBundleImportedByB
  SOURCES TestBundleImportedByB.cpp
  RESOURCES static.txt res.txt manifest.json
  RESOURCES_ROOT resources_static
  SKIP_BUNDLE_LIST)

set(BUILD_SHARED_LIBS ${US_BUILD_SHARED_LIBS})
usFunctionCreateTestBundleWithResources(TestBundleB
  SOURCES TestBundleB.cpp
  RESOURCES dynamic.txt res.txt manifest.json
  LINK_LIBRARIES TestBundleImportedByB)

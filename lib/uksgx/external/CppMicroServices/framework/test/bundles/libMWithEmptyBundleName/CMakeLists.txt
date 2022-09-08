
set(resource_files
  manifest.json
)

usFunctionCreateTestBundleWithResources(TestBundleWithEmptyBundleName
  SOURCES TestBundleM.cpp
  RESOURCES ${resource_files})

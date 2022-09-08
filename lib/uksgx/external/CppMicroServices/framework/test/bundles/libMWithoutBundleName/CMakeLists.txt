
set(resource_files
  manifest.json
)

usFunctionCreateTestBundleWithResources(TestBundleWithoutBundleName
  SOURCES TestBundleM.cpp
  RESOURCES ${resource_files})

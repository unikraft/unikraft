
set(resource_files
  manifest.json
)

usFunctionCreateTestBundleWithResources(TestBundleWithInvalidVersion
  SOURCES TestBundleM.cpp
  RESOURCES ${resource_files})


set(resource_files
  manifest.json
)

usFunctionCreateTestBundleWithResources(TestBundleWithInvalidVersionType
  SOURCES TestBundleM.cpp
  RESOURCES ${resource_files})

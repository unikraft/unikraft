
set(resource_files
  manifest.json
)

usFunctionCreateTestBundleWithResources(TestBundleM
  SOURCES TestBundleM.cpp
  RESOURCES ${resource_files})


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libSL1)

usFunctionCreateTestBundleWithResources(TestBundleSL3 SOURCES ActivatorSL3.cpp RESOURCES manifest.json)

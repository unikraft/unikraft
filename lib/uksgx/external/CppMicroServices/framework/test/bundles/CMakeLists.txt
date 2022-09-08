#-----------------------------------------------------------------------------
# Create test bundles
#-----------------------------------------------------------------------------

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../driver
  ${CMAKE_CURRENT_SOURCE_DIR}/../util
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party
  )

include(usFunctionCreateTestBundle)

set(_us_test_bundle_libs "" CACHE INTERNAL "" FORCE)

add_subdirectory(libA)
if(US_BUILD_SHARED_LIBS)
  add_subdirectory(libADuplicate)
endif()
add_subdirectory(libA2)
add_subdirectory(libBWithStatic)
add_subdirectory(libH)
add_subdirectory(libLQ)
add_subdirectory(libM)
if(US_BUILD_SHARED_LIBS)
  add_subdirectory(libMWithEmptyBundleName)
  add_subdirectory(libMWithInvalidVersion)
  add_subdirectory(libMWithInvalidVersionType)
  add_subdirectory(libMWithoutBundleName)
  add_subdirectory(libStartFail)
  add_subdirectory(libStopFail)
endif()

add_subdirectory(libWaitOnBundleOperation)
  
add_subdirectory(libS)
add_subdirectory(libSL1)
add_subdirectory(libSL3)
add_subdirectory(libSL4)

add_subdirectory(libRWithResources)
add_subdirectory(libRWithAppendedResources)
add_subdirectory(libRWithLinkedResources)

add_subdirectory(libWithNonStandardExt)

if(US_ENABLE_THREADING_SUPPORT)
  add_subdirectory(libC1)
endif()

add_subdirectory(libBA_00)
add_subdirectory(libBA_01)
add_subdirectory(libBA_X1)
add_subdirectory(libBA_S1)
add_subdirectory(libBA_10)
add_subdirectory(libU)

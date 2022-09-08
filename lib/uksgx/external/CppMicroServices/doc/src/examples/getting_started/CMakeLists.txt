# [proj-begin]
project(CppMicroServicesGettingStarted)

find_package(CppMicroServices REQUIRED)
# [proj-end]

include(GenerateExportHeader)

# [interface-begin]
#=========================================================
# A library providing the ServiceTime interface
#---------------------------------------------------------

# Technically, this is not a bundle.

add_library(ServiceTime
  service_time/ServiceTime.h
  service_time/ServiceTime.cpp
  )

target_include_directories(ServiceTime PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/service_time>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

generate_export_header(ServiceTime)

if(BUILD_SHARED_LIBS)
  set_target_properties(ServiceTime PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
    )
endif()
# [interface-end]

# [publisher-begin]
#=========================================================
# A bundle implementing the ServiceTime interface
#---------------------------------------------------------

set(_srcs
  service_time_systemclock/ServiceTimeImpl.cpp
  )

# Set up dependencies to resources to track changes
usFunctionGetResourceSource(TARGET ServiceTime_SystemClock OUT _srcs)
# Generate bundle initialization code
usFunctionGenerateBundleInit(TARGET ServiceTime_SystemClock OUT _srcs)

add_library(ServiceTime_SystemClock ${_srcs})

target_link_libraries(ServiceTime_SystemClock CppMicroServices ServiceTime)

set(_bundle_name service_time_systemclock)

set_target_properties(ServiceTime_SystemClock PROPERTIES
  # This is required for every bundle
  COMPILE_DEFINITIONS US_BUNDLE_NAME=${_bundle_name}
  # This is for convenience, used by other CMake functions
  US_BUNDLE_NAME ${_bundle_name}
  )

if(BUILD_SHARED_LIBS)
  set_target_properties(ServiceTime_SystemClock PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
    )
endif()

# Embed meta-data from a manifest.json file
usFunctionEmbedResources(TARGET ServiceTime_SystemClock
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/service_time_systemclock
  FILES
    manifest.json
  )
# [publisher-end]

#=========================================================
# A bundle consuming a ServiceTime service
#---------------------------------------------------------

set(_srcs
  service_time_consumer/ServiceTimeConsumer.cpp
  )

usFunctionGetResourceSource(TARGET ServiceTime_Consumer OUT _srcs)
usFunctionGenerateBundleInit(TARGET ServiceTime_Consumer OUT _srcs)

add_library(ServiceTime_Consumer ${_srcs})

target_link_libraries(ServiceTime_Consumer CppMicroServices ServiceTime)

set(_bundle_name service_time_consumer)

set_target_properties(ServiceTime_Consumer PROPERTIES
  COMPILE_DEFINITIONS US_BUNDLE_NAME=${_bundle_name}
  US_BUNDLE_NAME ${_bundle_name}
  )

if(BUILD_SHARED_LIBS)
  set_target_properties(ServiceTime_Consumer PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
    )
endif()

usFunctionEmbedResources(TARGET ServiceTime_Consumer
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/service_time_consumer
  FILES
    manifest.json
  )


# [exec-begin]
#=========================================================
# The executable managing the CppMicroServices framework
#---------------------------------------------------------

set(_srcs
  main.cpp
  )

if(NOT US_BUILD_SHARED_LIBS)
  # Set up dependencies to resources from static bundles
  usFunctionGetResourceSource(TARGET GettingStarted OUT _srcs)
endif()

add_executable(GettingStarted ${_srcs})

target_link_libraries(GettingStarted CppMicroServices)

if(NOT US_BUILD_SHARED_LIBS)
  set(_static_bundles ServiceTime_SystemClock ServiceTime_Consumer)
  target_link_libraries(GettingStarted ${_static_bundles})
  usFunctionEmbedResources(TARGET GettingStarted
    # The executable is not a bundle itself but we still
    # need a bundle name for it here.
    BUNDLE_NAME getting_started
    # Merge in all resources from static bundles
    ZIP_ARCHIVES ${_static_bundles}
    )
endif()
# [exec-end]

aux_source_directory(. _srcs)

#Disable CET
if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8)
  add_compile_options(-fcf-protection=none)
endif()

if(NOT US_BUILD_SHARED_LIBS)
  # Set up dependencies to resources from static bundles
  usFunctionGetResourceSource(TARGET aesm_service OUT _srcs)
endif()
add_executable(aesm_service ${_srcs})

target_include_directories(aesm_service PRIVATE
  ${PROJECT_SOURCE_DIR}/common
  ${PROJECT_SOURCE_DIR}/../../inc
  ${PROJECT_SOURCE_DIR}/../../inc/internal
  ${SGX_HEADER_DIR}
  ${PROJECT_SOURCE_DIR}/../../../../common/inc/internal
  ${PROJECT_SOURCE_DIR}/../../../../external/epid-sdk
  ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/common/inc
  ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/quote/inc
  ${PROJECT_SOURCE_DIR}/interfaces
  ipc
)

add_subdirectory(ipc)
target_link_libraries(aesm_service
  CppMicroServices
  oal
  utils
  ipc
)

target_compile_definitions(aesm_service PRIVATE
    $<$<CONFIG:Debug>:DBG_LOG>
)

if(NOT US_BUILD_SHARED_LIBS)
  set(_static_bundles ${SUBDIRS})
  target_link_libraries(aesm_service ${_static_bundles})
  usFunctionEmbedResources(TARGET aesm_service
    # The executable is not a bundle itself but we still
    # need a bundle name for it here.
    BUNDLE_NAME aesm_service_bundle_name
    # Merge in all resources from static bundles
    ZIP_ARCHIVES ${_static_bundles}
    )
endif()

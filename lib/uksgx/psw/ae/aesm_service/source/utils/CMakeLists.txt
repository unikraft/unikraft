find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    message ("OpenSSL found")
else()
    message (FATAL_ERROR "Cannot find OPENSSL")
endif()
aux_source_directory(. _srcs)
add_library(utils SHARED ${_srcs}
    "../../../../../common/src/sgx_read_rand.cpp"
	"../../../../../common/src/sgx_memset_s.cpp")

target_include_directories(utils PRIVATE
    ${PROJECT_SOURCE_DIR}/common
    ${PROJECT_SOURCE_DIR}/common/oal
    ${PROJECT_SOURCE_DIR}/../../inc
    ${PROJECT_SOURCE_DIR}/../../inc/internal
    ${SGX_HEADER_DIR}
    ${PROJECT_SOURCE_DIR}/../../../../common/inc/internal
    ${PROJECT_SOURCE_DIR}/../../../../external/epid-sdk
    ${PROJECT_SOURCE_DIR}/../../../../external/rdrand
    ${PROJECT_SOURCE_DIR}/../../data/constants/linux
)

target_compile_definitions(utils PRIVATE
  $<$<CONFIG:Debug>:DBG_LOG>
)

set_property(TARGET utils APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs")

target_link_libraries(utils
    ${OPENSSL_LIBRARIES}
    oal
    ${CMAKE_SOURCE_DIR}/../../../../external/rdrand/src/librdrand.a
)


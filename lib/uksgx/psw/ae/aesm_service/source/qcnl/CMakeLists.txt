find_package(CURL REQUIRED)
if(CURL_FOUND)
    message ("CURL found")
else()
    message (FATAL_ERROR "Cannot find CURL")
endif()
aux_source_directory(${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/qcnl _srcs)
aux_source_directory(${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/qcnl/linux _srcs)
add_library(sgx_default_qcnl_wrapper SHARED ${_srcs})

target_include_directories(sgx_default_qcnl_wrapper PRIVATE
    ${SGX_HEADER_DIR}
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/qcnl/inc
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/pce_wrapper/inc
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/common/inc
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/common/inc/internal
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteVerification/QVL/Src/ThirdParty/rapidjson/include/rapidjson
)

set_property(TARGET sgx_default_qcnl_wrapper APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs")
set_property(TARGET sgx_default_qcnl_wrapper APPEND_STRING PROPERTY LINK_DEPENDS  
        ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/qcnl/linux/sgx_default_qcnl.lds
    )

target_link_libraries(sgx_default_qcnl_wrapper
    ${CURL_LIBRARIES}
)


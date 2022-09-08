aux_source_directory(${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/qpl _srcs)
aux_source_directory(${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/qpl/linux _srcs)

link_directories(
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/prebuilt/openssl/lib/linux64
)

add_library(dcap_quoteprov SHARED ${_srcs})

target_include_directories(dcap_quoteprov PRIVATE
    ${SGX_HEADER_DIR}
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/qcnl/inc
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/qpl/inc
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/pce_wrapper/inc
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/common/inc
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/common/inc/internal
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/prebuilt/openssl/inc
)

set_property(TARGET dcap_quoteprov APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs")
set_property(TARGET dcap_quoteprov APPEND_STRING PROPERTY LINK_DEPENDS
        ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/qpl/linux/sgx_default_quote_provider.lds
    )

target_link_libraries(dcap_quoteprov sgx_default_qcnl_wrapper crypto pthread dl)


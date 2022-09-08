get_filename_component(bundle ${CMAKE_CURRENT_SOURCE_DIR} NAME)
aux_source_directory(. _srcs)
CreateBundle(${bundle} ${_srcs})

target_include_directories(${bundle} PRIVATE
	${PROJECT_SOURCE_DIR}/common
	${SGX_HEADER_DIR}
	${PROJECT_SOURCE_DIR}/../../../../common/inc/internal
	${PROJECT_SOURCE_DIR}/../../inc
	${PROJECT_SOURCE_DIR}/../../inc/internal
	${PROJECT_SOURCE_DIR}/../../../../external/epid-sdk
	${PROJECT_SOURCE_DIR}/../../../../external/ippcp_internal/inc
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/quote/inc
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/common/inc
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/quote/enclave
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/pce_wrapper/inc
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/common/inc/internal
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/common/inc/internal/linux
)

target_link_libraries(${bundle} oal utils)
set_property(TARGET ${bundle} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs")

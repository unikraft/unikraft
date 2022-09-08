set(qe3_logic "${CMAKE_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/build/linux/libsgx_qe3_logic.so")
get_filename_component(bundle ${CMAKE_CURRENT_SOURCE_DIR} NAME)
aux_source_directory(. _src)
link_directories(${CMAKE_SOURCE_DIR}/../../../../build/linux
                 ${CMAKE_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/build/linux)
CreateBundle(${bundle} ${_src})

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

add_custom_command(OUTPUT ${qe3_logic}
    COMMAND make all
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/quote/linux"
)
add_custom_target(qe3_logic_lib DEPENDS ${qe3_logic})
target_link_libraries(${bundle} oal utils ${qe3_logic})
add_dependencies(${bundle} qe3_logic_lib)
set_property(TARGET ${bundle} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs")

set(Edger8rOutputPVE "${CMAKE_CURRENT_BINARY_DIR}/provision_enclave_u.c")
set(Edger8rOutputQE "${CMAKE_CURRENT_BINARY_DIR}/quoting_enclave_u.c")
get_filename_component(bundle ${CMAKE_CURRENT_SOURCE_DIR} NAME)
aux_source_directory(. _srcs)
set(all_src ${_srcs} ${CMAKE_SOURCE_DIR}/../../common/pek_pub_key.cpp ${CMAKE_SOURCE_DIR}/../../common/se_sig_rl.cpp)
link_directories(${CMAKE_SOURCE_DIR}/../../../../build/linux)
CreateBundle(${bundle} ${all_src} ${Edger8rOutputQE} ${Edger8rOutputPVE})

target_include_directories(${bundle} PRIVATE
	${PROJECT_SOURCE_DIR}/common
	${SGX_HEADER_DIR}
	${PROJECT_SOURCE_DIR}/../../../../common/inc/internal
	${PROJECT_SOURCE_DIR}/../../inc
	${PROJECT_SOURCE_DIR}/../../inc/internal
	${PROJECT_SOURCE_DIR}/../../pve
	${PROJECT_SOURCE_DIR}/../../../../external/epid-sdk
	${PROJECT_SOURCE_DIR}/../../../../external/ippcp_internal/inc
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/quote/inc
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/common/inc
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/quote/enclave
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/pce_wrapper/inc
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/common/inc/internal
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/common/inc/internal/linux
)

add_custom_command(
    OUTPUT ${Edger8rOutputPVE}
    COMMAND ${EDGER8R} --untrusted --untrusted-dir ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/../../pve/provision_enclave.edl --search-path ${SGX_HEADER_DIR}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_command(
    OUTPUT ${Edger8rOutputQE}
    COMMAND ${EDGER8R} --untrusted --untrusted-dir ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/../../qe/quoting_enclave.edl --search-path ${SGX_HEADER_DIR}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(${bundle} oal utils urts_internal)
set_property(TARGET ${bundle} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs")

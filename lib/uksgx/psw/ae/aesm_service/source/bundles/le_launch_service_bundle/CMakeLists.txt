if(REF_LE)
    set(Edger8rOutputLE "${CMAKE_CURRENT_BINARY_DIR}/ref_le_u.c")
    set(Edger8rEDLLE "${PROJECT_SOURCE_DIR}/../../ref_le/ref_le.edl")
else()
    set(Edger8rOutputLE "${CMAKE_CURRENT_BINARY_DIR}/launch_enclave_u.c")
    set(Edger8rEDLLE "${PROJECT_SOURCE_DIR}/../../le/launch_enclave.edl")
endif()

get_filename_component(bundle ${CMAKE_CURRENT_SOURCE_DIR} NAME)
aux_source_directory(. _srcs)
link_directories(${CMAKE_SOURCE_DIR}/../../../urts/linux)
CreateBundle(${bundle} ${_srcs} ${Edger8rOutputLE})

target_include_directories(${bundle} PRIVATE
	${PROJECT_SOURCE_DIR}/common
	${SGX_HEADER_DIR}
	${PROJECT_SOURCE_DIR}/../../../../common/inc/internal
	${PROJECT_SOURCE_DIR}/../../inc
	${PROJECT_SOURCE_DIR}/../../inc/internal
	${PROJECT_SOURCE_DIR}/../../../../external/epid-sdk
	${PROJECT_SOURCE_DIR}/../../../../external/ippcp_internal/inc
)

add_custom_command(
    OUTPUT ${Edger8rOutputLE}
    COMMAND ${EDGER8R} --untrusted --untrusted-dir ${CMAKE_CURRENT_BINARY_DIR} ${Edger8rEDLLE} --search-path ${SGX_HEADER_DIR}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(${bundle} oal utils urts_internal)
set_property(TARGET ${bundle} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs")

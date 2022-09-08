set(pce_logic "${CMAKE_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/build/linux/libsgx_pce_logic.so")
get_filename_component(bundle ${CMAKE_CURRENT_SOURCE_DIR} NAME)
aux_source_directory(. _srcs)
link_directories(${CMAKE_SOURCE_DIR}/../../../../build/linux
				 /../../../../external/dcap_source/QuoteGeneration/build/linux)
CreateBundle(${bundle} ${_srcs})

target_include_directories(${bundle} PRIVATE
	${PROJECT_SOURCE_DIR}/common
	${PROJECT_SOURCE_DIR}/../../../../common/inc/internal
	${SGX_HEADER_DIR}
	${PROJECT_SOURCE_DIR}/../../inc
	${PROJECT_SOURCE_DIR}/../../inc/internal
	${PROJECT_SOURCE_DIR}/../../pve
	${PROJECT_SOURCE_DIR}/../../../../external/epid-sdk
	${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/pce_wrapper/inc
)

add_custom_command(OUTPUT ${pce_logic}
    COMMAND make all
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/pce_wrapper/linux"
)
add_custom_target(pce_logic_lib DEPENDS ${pce_logic})
target_link_libraries(${bundle} oal ${pce_logic})
add_dependencies(${bundle} pce_logic_lib)
set_property(TARGET ${bundle} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs")

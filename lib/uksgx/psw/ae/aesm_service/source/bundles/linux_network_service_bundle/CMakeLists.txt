find_package(CURL REQUIRED)
if(CURL_FOUND)
    message ("CURL found")
else()
    message (FATAL_ERROR "Cannot find CURL")
endif()
get_filename_component(bundle ${CMAKE_CURRENT_SOURCE_DIR} NAME)
aux_source_directory(. _srcs)
CreateBundle(${bundle} ${_srcs})

target_include_directories(${bundle} PRIVATE
	${PROJECT_SOURCE_DIR}/common
	${PROJECT_SOURCE_DIR}/common/oal
	${SGX_HEADER_DIR}
	${PROJECT_SOURCE_DIR}/../../../../common/inc/internal
	${PROJECT_SOURCE_DIR}/../../inc
	${PROJECT_SOURCE_DIR}/../../inc/internal
	${PROJECT_SOURCE_DIR}/../../../../external/epid-sdk
)

target_link_libraries(${bundle} ${CURL_LIBRARIES} oal utils)
set_property(TARGET ${bundle} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs")

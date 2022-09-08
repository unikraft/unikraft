aux_source_directory(. IPC_LIB_SRCS)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS messages.proto)
# Print path to generated files
message ("PROTO_SRCS = ${PROTO_SRCS}")
message ("PROTO_HDRS = ${PROTO_HDRS}")
list(APPEND IPC_LIB_SRCS ${PROTO_SRCS})
add_library (ipc ${IPC_LIB_SRCS})
target_include_directories(ipc
    PUBLIC
    ${PROTOBUF_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/common
    ${PROJECT_SOURCE_DIR}/core/ipc
    ${PROJECT_SOURCE_DIR}/../../inc
    ${PROJECT_SOURCE_DIR}/../../inc/internal
    ${SGX_HEADER_DIR}
    ${PROJECT_SOURCE_DIR}/../../../../common/inc/internal
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/common/inc
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/quote_wrapper/quote/inc
    ${PROJECT_SOURCE_DIR}/../../../../external/dcap_source/QuoteGeneration/pce_wrapper/inc
)

# link the exe against the libraries
target_link_libraries(ipc
    PUBLIC
    ${PROTOBUF_LIBRARIES}
)


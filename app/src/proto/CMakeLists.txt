#include(FindProtobuf)
#PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER model.proto)
#add_library(proto ${PROTO_HEADER} ${PROTO_SRC})
FILE(GLOB proto_files ${CMAKE_CURRENT_SOURCE_DIR}/*.proto)
FOREACH(proto_file ${proto_files})
    STRING(REGEX REPLACE "[^/]proto" "" proto_file_name ${proto_file})
    LIST(APPEND PROTO_SRCS "${proto_file_name}.pb.cc")
    ADD_CUSTOM_COMMAND(
            OUTPUT "${proto_file_name}.pb.h" "${proto_file_name}.pb.cc"
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/protoc ${proto_file} --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
            --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}
            DEPENDS ${proto_file}
    )
ENDFOREACH(proto_file)
message(STATUS ${PROTO_SRCS})

message(STATUS ${CMAKE_CURRENT_SOURCE_DIR}/../external/protobuf/src/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")


add_library(pt STATIC ${PROTO_SRCS} model.cc)
target_compile_options(pt PRIVATE -fno-rtti)

target_include_directories(pt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/protobuf/src
        ../main/cpp
        ../main/include
        ${ANDROID_NDK}/sources/android/native_app_glue
        )

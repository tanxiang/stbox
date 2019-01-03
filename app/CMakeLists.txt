# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")

add_library( # Sets the name of the library.
             stbox

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
             src/main/cpp/util.cc
             src/main/cpp/main.cc
             src/main/cpp/stboxvk.cc
             src/main/cpp/Blob.cc
             src/main/cpp/Connecter.cc
             src/main/cpp/Net.cc
             src/main/cpp/Activation.cc
             )

target_compile_definitions(stbox PRIVATE VK_USE_PLATFORM_ANDROID_KHR)


target_include_directories(stbox PRIVATE
               src/main/include
               ${ANDROID_NDK}/sources/android/native_app_glue
               #${ANDROID_NDK}/sources/third_party/vulkan/src/include
               #${ANDROID_NDK}/sources/third_party/vulkan/src/libs
               #${ANDROID_NDK}/sources/third_party/shaderc/include
               )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        z-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        z )

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

find_library( # Sets the name of the path variable.
              vulkan-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              vulkan )

find_library( # Sets the name of the path variable.
              android-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              android )
# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

# Import shaderc lib in NDK
#get_filename_component(SHADERC_SRC
#                       ${ANDROID_NDK}/sources/third_party/shaderc
#                       ABSOLUTE)
# Build Util Lib
set(UTILS_NAME  vsamputils)
set(GLSLANG_SPIRV_INCLUDE_DIR ${SHADERC_SRC}/third_party/glslang)
get_filename_component(PROJECT_SOURCE_DIR
                       "${CMAKE_SOURCE_DIR}/../../.."
                       ABSOLUTE)
get_filename_component(GLMINC_PREFIX
                       "${CMAKE_SOURCE_DIR}/../utils"
                       ABSOLUTE)
#add_library(shaderc_lib STATIC IMPORTED)
#set_target_properties(shaderc_lib PROPERTIES IMPORTED_LOCATION
#                      ${SHADERC_SRC}/libs/${ANDROID_STL}/${ANDROID_ABI}/libshaderc.a)

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries( # Specifies the target library.
                       stbox
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       ${vulkan-lib}
                       ${android-lib}
                       #shaderc_lib
                       )


set(protobuf_source_dir  .)

set(libprotobuf_lite_files
        ${protobuf_source_dir}/src/google/protobuf/arena.cc
        ${protobuf_source_dir}/src/google/protobuf/arenastring.cc
        ${protobuf_source_dir}/src/google/protobuf/extension_set.cc
        ${protobuf_source_dir}/src/google/protobuf/generated_message_table_driven_lite.cc
        ${protobuf_source_dir}/src/google/protobuf/generated_message_util.cc
        ${protobuf_source_dir}/src/google/protobuf/implicit_weak_message.cc
        ${protobuf_source_dir}/src/google/protobuf/parse_context.cc
        ${protobuf_source_dir}/src/google/protobuf/io/coded_stream.cc
        ${protobuf_source_dir}/src/google/protobuf/io/strtod.cc
        ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream.cc
        ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl_lite.cc
        ${protobuf_source_dir}/src/google/protobuf/message_lite.cc
        ${protobuf_source_dir}/src/google/protobuf/repeated_field.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/bytestream.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/common.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/int128.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/io_win32.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/status.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/statusor.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/stringpiece.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/stringprintf.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/structurally_valid.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/strutil.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/time.cc
        ${protobuf_source_dir}/src/google/protobuf/wire_format_lite.cc
        )


set(libprotobuf_files
        ${protobuf_source_dir}/src/google/protobuf/any.cc
        ${protobuf_source_dir}/src/google/protobuf/any.pb.cc
        ${protobuf_source_dir}/src/google/protobuf/api.pb.cc
        ${protobuf_source_dir}/src/google/protobuf/compiler/importer.cc
        ${protobuf_source_dir}/src/google/protobuf/compiler/parser.cc
        ${protobuf_source_dir}/src/google/protobuf/descriptor.cc
        ${protobuf_source_dir}/src/google/protobuf/descriptor.pb.cc
        ${protobuf_source_dir}/src/google/protobuf/descriptor_database.cc
        ${protobuf_source_dir}/src/google/protobuf/duration.pb.cc
        ${protobuf_source_dir}/src/google/protobuf/dynamic_message.cc
        ${protobuf_source_dir}/src/google/protobuf/empty.pb.cc
        ${protobuf_source_dir}/src/google/protobuf/extension_set_heavy.cc
        ${protobuf_source_dir}/src/google/protobuf/field_mask.pb.cc
        ${protobuf_source_dir}/src/google/protobuf/generated_message_reflection.cc
        ${protobuf_source_dir}/src/google/protobuf/generated_message_table_driven.cc
        ${protobuf_source_dir}/src/google/protobuf/io/gzip_stream.cc
        ${protobuf_source_dir}/src/google/protobuf/io/printer.cc
        ${protobuf_source_dir}/src/google/protobuf/io/tokenizer.cc
        ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl.cc
        ${protobuf_source_dir}/src/google/protobuf/map_field.cc
        ${protobuf_source_dir}/src/google/protobuf/message.cc
        ${protobuf_source_dir}/src/google/protobuf/reflection_ops.cc
        ${protobuf_source_dir}/src/google/protobuf/service.cc
        ${protobuf_source_dir}/src/google/protobuf/source_context.pb.cc
        ${protobuf_source_dir}/src/google/protobuf/struct.pb.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/mathlimits.cc
        ${protobuf_source_dir}/src/google/protobuf/stubs/substitute.cc
        ${protobuf_source_dir}/src/google/protobuf/text_format.cc
        ${protobuf_source_dir}/src/google/protobuf/timestamp.pb.cc
        ${protobuf_source_dir}/src/google/protobuf/type.pb.cc
        ${protobuf_source_dir}/src/google/protobuf/unknown_field_set.cc
        ${protobuf_source_dir}/src/google/protobuf/util/delimited_message_util.cc
        ${protobuf_source_dir}/src/google/protobuf/util/field_comparator.cc
        ${protobuf_source_dir}/src/google/protobuf/util/field_mask_util.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/datapiece.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/default_value_objectwriter.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/error_listener.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/field_mask_utility.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/json_escaping.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/json_objectwriter.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/json_stream_parser.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/object_writer.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/proto_writer.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/protostream_objectsource.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/protostream_objectwriter.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/type_info.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/type_info_test_helper.cc
        ${protobuf_source_dir}/src/google/protobuf/util/internal/utility.cc
        ${protobuf_source_dir}/src/google/protobuf/util/json_util.cc
        ${protobuf_source_dir}/src/google/protobuf/util/message_differencer.cc
        ${protobuf_source_dir}/src/google/protobuf/util/time_util.cc
        ${protobuf_source_dir}/src/google/protobuf/util/type_resolver_util.cc
        ${protobuf_source_dir}/src/google/protobuf/wire_format.cc
        ${protobuf_source_dir}/src/google/protobuf/wrappers.pb.cc
        )

set(libprotobuf_includes
        ${protobuf_source_dir}/src/google/protobuf/any.h
        ${protobuf_source_dir}/src/google/protobuf/any.pb.h
        ${protobuf_source_dir}/src/google/protobuf/api.pb.h
        ${protobuf_source_dir}/src/google/protobuf/compiler/importer.h
        ${protobuf_source_dir}/src/google/protobuf/compiler/parser.h
        ${protobuf_source_dir}/src/google/protobuf/descriptor.h
        ${protobuf_source_dir}/src/google/protobuf/descriptor.pb.h
        ${protobuf_source_dir}/src/google/protobuf/descriptor_database.h
        ${protobuf_source_dir}/src/google/protobuf/duration.pb.h
        ${protobuf_source_dir}/src/google/protobuf/dynamic_message.h
        ${protobuf_source_dir}/src/google/protobuf/empty.pb.h
        ${protobuf_source_dir}/src/google/protobuf/field_mask.pb.h
        ${protobuf_source_dir}/src/google/protobuf/generated_message_reflection.h
        ${protobuf_source_dir}/src/google/protobuf/io/gzip_stream.h
        ${protobuf_source_dir}/src/google/protobuf/io/printer.h
        ${protobuf_source_dir}/src/google/protobuf/io/tokenizer.h
        ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl.h
        ${protobuf_source_dir}/src/google/protobuf/map_field.h
        ${protobuf_source_dir}/src/google/protobuf/message.h
        ${protobuf_source_dir}/src/google/protobuf/reflection_ops.h
        ${protobuf_source_dir}/src/google/protobuf/service.h
        ${protobuf_source_dir}/src/google/protobuf/source_context.pb.h
        ${protobuf_source_dir}/src/google/protobuf/struct.pb.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/mathlimits.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/substitute.h
        ${protobuf_source_dir}/src/google/protobuf/text_format.h
        ${protobuf_source_dir}/src/google/protobuf/timestamp.pb.h
        ${protobuf_source_dir}/src/google/protobuf/type.pb.h
        ${protobuf_source_dir}/src/google/protobuf/unknown_field_set.h
        ${protobuf_source_dir}/src/google/protobuf/util/delimited_message_util.h
        ${protobuf_source_dir}/src/google/protobuf/util/field_comparator.h
        ${protobuf_source_dir}/src/google/protobuf/util/field_mask_util.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/datapiece.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/default_value_objectwriter.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/error_listener.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/field_mask_utility.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/json_escaping.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/json_objectwriter.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/json_stream_parser.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/object_writer.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/proto_writer.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/protostream_objectsource.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/protostream_objectwriter.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/type_info.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/type_info_test_helper.h
        ${protobuf_source_dir}/src/google/protobuf/util/internal/utility.h
        ${protobuf_source_dir}/src/google/protobuf/util/json_util.h
        ${protobuf_source_dir}/src/google/protobuf/util/message_differencer.h
        ${protobuf_source_dir}/src/google/protobuf/util/time_util.h
        ${protobuf_source_dir}/src/google/protobuf/util/type_resolver_util.h
        ${protobuf_source_dir}/src/google/protobuf/wire_format.h
        ${protobuf_source_dir}/src/google/protobuf/wrappers.pb.h
        )


add_library(libprotobuf STATIC ${libprotobuf_lite_files}
        ${libprotobuf_files} ${libprotobuf_includes})
target_link_libraries(libprotobuf ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(libprotobuf PUBLIC ${protobuf_source_dir}/src)
target_compile_options(libprotobuf PRIVATE -frtti)
target_compile_definitions(libprotobuf PRIVATE HAVE_PTHREAD)

add_library(protobuf::libprotobuf ALIAS libprotobuf)

add_library( # Sets the name of the library.
        onnxpb
        # Sets the library as a shared library.
        SHARED
        src/main/cpp/onnx.cc
        src/main/cpp/onnx.pb.cc )

target_compile_options(onnxpb PRIVATE -frtti)

target_include_directories(onnxpb PRIVATE
        src/google/protobuf
        )

target_link_libraries(onnxpb
        libprotobuf
        ${log-lib}
        ${z-lib}
        )
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

set(libprotobuf_lite_includes
        ${protobuf_source_dir}/src/google/protobuf/arena.h
        ${protobuf_source_dir}/src/google/protobuf/arenastring.h
        ${protobuf_source_dir}/src/google/protobuf/extension_set.h
        ${protobuf_source_dir}/src/google/protobuf/generated_message_util.h
        ${protobuf_source_dir}/src/google/protobuf/implicit_weak_message.h
        ${protobuf_source_dir}/src/google/protobuf/parse_context.h
        ${protobuf_source_dir}/src/google/protobuf/io/coded_stream.h
        ${protobuf_source_dir}/src/google/protobuf/io/strtod.h
        ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream.h
        ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl_lite.h
        ${protobuf_source_dir}/src/google/protobuf/message_lite.h
        ${protobuf_source_dir}/src/google/protobuf/repeated_field.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/bytestream.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/common.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/int128.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/once.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/status.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/statusor.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/stringpiece.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/stringprintf.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/strutil.h
        ${protobuf_source_dir}/src/google/protobuf/stubs/time.h
        ${protobuf_source_dir}/src/google/protobuf/wire_format_lite.h
        )


add_library(libprotobuf_lite STATIC
        ${libprotobuf_lite_files} ${libprotobuf_lite_includes})
target_link_libraries(libprotobuf_lite ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(libprotobuf_lite PUBLIC ${protobuf_source_dir}/src)
target_compile_options(libprotobuf_lite PRIVATE -frtti)
target_compile_definitions(libprotobuf_lite PRIVATE HAVE_PTHREAD)

add_library(protobuf::libprotobuf_lite ALIAS libprotobuf_lite)

add_library( # Sets the name of the library.
        onnxpb
        # Sets the library as a shared library.
        SHARED
        src/main/cpp/onnx.pb.cc )

target_compile_options(onnxpb PRIVATE -frtti)

target_include_directories(onnxpb PRIVATE
        src/google/protobuf
        )

target_link_libraries(onnxpb
        libprotobuf_lite
        ${log-lib}
        ${z-lib}
        )
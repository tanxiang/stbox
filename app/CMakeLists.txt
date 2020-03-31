# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.6)

#message(FATAL_ERROR ${CMAKE_ANDROID_NDK})
# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_subdirectory(src/proto)

SET(protobuf_BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "Build libprotoc and protoc compiler" FORCE)
set(BUILD_SHARED_LIBS OFF FORCE)
add_subdirectory(src/external/protobuf/cmake)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")

add_library(app-glue
        STATIC
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

add_library( # Sets the name of the library.
        stbox
        # Sets the library as a shared library.
        SHARED
        # Provides a relative path to your source file(s).
        src/main/cpp/util.cc
        src/main/cpp/main.cc
        src/main/cpp/Device.cc
        src/main/cpp/Window.cc
        src/main/cpp/JobDraw.cc
        src/main/cpp/JobFont.cc
        src/main/cpp/JobDrawLine.cc
        src/main/cpp/JobSkyBox.cc
        src/main/cpp/Instance.cc
        src/main/cpp/stboxvk.cc
        #src/main/cpp/Blob.cc
        #src/main/cpp/Connecter.cc
        #src/main/cpp/Net.cc
        #src/main/cpp/Activation.cc
        )

target_compile_definitions(stbox PRIVATE VK_USE_PLATFORM_ANDROID_KHR)
target_compile_options(stbox PRIVATE -fno-rtti)


target_include_directories(stbox PRIVATE
        src/external/glm
        src/external/gli
        src/main/include
        ${ANDROID_NDK}/sources/android/native_app_glue
        )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

#find_library( # Sets the name of the path variable.
# z-lib
# Specifies the name of the NDK library that
# you want CMake to locate.
# z )

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

find_library( # Sets the name of the path variable.
        vulkan-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        vulkan)


find_library( # Sets the name of the path variable.
        android-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        android)
# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

# Import shaderc lib in NDK
#get_filename_component(SHADERC_SRC
#                       ${ANDROID_NDK}/sources/third_party/shaderc
#                       ABSOLUTE)
# Build Util Lib
#set(UTILS_NAME  vsamputils)
#set(GLSLANG_SPIRV_INCLUDE_DIR ${SHADERC_SRC}/third_party/glslang)
#get_filename_component(PROJECT_SOURCE_DIR
#                       "${CMAKE_SOURCE_DIR}/../../.."
#                       ABSOLUTE)
#get_filename_component(GLMINC_PREFIX
#                       "${CMAKE_SOURCE_DIR}/../utils"
#                       ABSOLUTE)
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
        #onnxpb
        app-glue
        )


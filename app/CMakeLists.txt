# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_ANDROID_KHR -Wno-narrowing")

add_library( # Sets the name of the library.
             stbox

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
             src/main/cpp/util.cc
             src/main/cpp/main.cc
             src/main/cpp/stboxvk.cc )

target_include_directories(stbox PRIVATE
               ${ANDROID_NDK}/sources/android/native_app_glue
               #${ANDROID_NDK}/sources/third_party/vulkan/src/include
               #${ANDROID_NDK}/sources/third_party/vulkan/src/libs
               ${ANDROID_NDK}/sources/third_party/shaderc/include )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

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
get_filename_component(SHADERC_SRC
                       ${ANDROID_NDK}/sources/third_party/shaderc
                       ABSOLUTE)
# Build Util Lib
set(UTILS_NAME  vsamputils)
set(GLSLANG_SPIRV_INCLUDE_DIR ${SHADERC_SRC}/third_party/glslang)
get_filename_component(PROJECT_SOURCE_DIR
                       "${CMAKE_SOURCE_DIR}/../../.."
                       ABSOLUTE)
get_filename_component(GLMINC_PREFIX
                       "${CMAKE_SOURCE_DIR}/../utils"
                       ABSOLUTE)
add_library(shaderc_lib STATIC IMPORTED)
set_target_properties(shaderc_lib PROPERTIES IMPORTED_LOCATION
                      ${SHADERC_SRC}/libs/${ANDROID_STL}/${ANDROID_ABI}/libshaderc.a)

set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries( # Specifies the target library.
                       stbox
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       ${vulkan-lib}
                       ${android-lib}
                       shaderc_lib )
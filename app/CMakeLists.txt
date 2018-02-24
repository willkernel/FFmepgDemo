# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)
#配置工程路径
set(path_project E:/willkernel/FFmpeg)
#配置头文件的包含路径
include_directories(${path_project}/app/src/main/cpp/include)
include_directories(${path_project}/app/src/main/cpp/include/yuv)
# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             ffmpeg-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/ffmpeg-lib.c )

add_library(avcodec
            SHARED
            IMPORTED
)
set_target_properties(avcodec
                      PROPERTIES IMPORTED_LOCATION
                      ${path_project}/app/libs/${ANDROID_ABI}/libavcodec-56.so
)

add_library(avdevice
            SHARED
            IMPORTED
)
set_target_properties(avdevice
                      PROPERTIES IMPORTED_LOCATION
                      ${path_project}/app/libs/${ANDROID_ABI}/libavdevice-56.so
)

add_library(avfilter
            SHARED
            IMPORTED
)
set_target_properties(avfilter
                      PROPERTIES IMPORTED_LOCATION
                      ${path_project}/app/libs/${ANDROID_ABI}/libavfilter-5.so
)

add_library(avformat
            SHARED
            IMPORTED
)
set_target_properties(avformat
                      PROPERTIES IMPORTED_LOCATION
                      ${path_project}/app/libs/${ANDROID_ABI}/libavformat-56.so
)

add_library(avutil
            SHARED
            IMPORTED
)
set_target_properties(avutil
                      PROPERTIES IMPORTED_LOCATION
                      ${path_project}/app/libs/${ANDROID_ABI}/libavutil-54.so
)

add_library(postproc
            SHARED
            IMPORTED
)
set_target_properties(postproc
                      PROPERTIES IMPORTED_LOCATION
                      ${path_project}/app/libs/${ANDROID_ABI}/libpostproc-53.so
)

add_library(swresample
            SHARED
            IMPORTED
)
set_target_properties(swresample
                      PROPERTIES IMPORTED_LOCATION
                      ${path_project}/app/libs/${ANDROID_ABI}/libswresample-1.so
)

add_library(swscale
            SHARED
            IMPORTED
)
set_target_properties(swscale
                      PROPERTIES IMPORTED_LOCATION
                      ${path_project}/app/libs/${ANDROID_ABI}/libswscale-3.so
)
add_library( yuv
             SHARED
             IMPORTED
            )

set_target_properties(
                       yuv
                       PROPERTIES IMPORTED_LOCATION
                       ${path_project}/app/libs/${ANDROID_ABI}/libyuv.so
                    )

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
#找到Android的Window绘制相关的库（这个库已经在Android平台中了）
find_library(android-lib
             android
             )
# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       ffmpeg-lib
                       avcodec
                       avdevice
                       avfilter
                       avformat
                       avutil
                       postproc
                       swresample
                       swscale
                       yuv
                       ${android-lib}
                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )
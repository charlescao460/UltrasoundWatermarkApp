
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

cmake_minimum_required(VERSION 3.22.1)
include(FetchContent)

project("ultrasound_watermark")

# Require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
# Require C 11
set(C_STANDARD 11)
set(C_STANDARD_REQUIRED ON)
set(C_EXTENSIONS ON)


add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        UltrasoundWatermarkJNI.cpp
        WatermarkCallee.cpp
        WatermarkCaller.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC .)


# Add DSP core
set(ENABLE_ASE_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_ASE_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(ENABLE_APP_HUMAN_ANGLE OFF CACHE BOOL "" FORCE)
set(ENABLE_APP_RESPIRATION OFF CACHE BOOL "" FORCE)
set(ENABLE_APP_SLEEP_STAGING OFF CACHE BOOL "" FORCE)
set(ENABLE_APP_ULTRASOUND_WATERMARK ON CACHE BOOL "" FORCE)
set(ENABLE_APP_ULTRASOUND_WATERMARK_MAIN OFF CACHE BOOL "" FORCE)
add_subdirectory(Acoustic-DSP-Core)
# Add oboe
FetchContent_Declare(
        oboe
        GIT_REPOSITORY https://github.com/google/oboe.git
        GIT_TAG 1.10.0
        GIT_SHALLOW 1)
FetchContent_MakeAvailable(oboe)

target_link_libraries(${CMAKE_PROJECT_NAME}
        ase_ultrasound_watermark
        acoustic-dsp-core
        oboe
        android
        log
)

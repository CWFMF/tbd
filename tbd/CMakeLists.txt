cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# HACK: architecture that works on azure batch and local machine
set(ARCH_PREFERRED "skylake-avx512")

if(WIN32)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")
endif()

project(tbd)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++23")
if (WIN32)
    # don't set -Wextra
    set(CMAKE_CXX_FLAGS "-Wall")
else()
    # HACK: if gcc supports ARCH_PREFERRED on this then use that else native
    execute_process(COMMAND bash -c "(gcc -march=native -Q --help=target | grep ${ARCH_PREFERRED} > /dev/null && echo -n ${ARCH_PREFERRED}) || (echo -n 'native')"
                    OUTPUT_VARIABLE USE_ARCH
                    ERROR_QUIET)
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    # # set(USE_ARCH native)
    if (USE_ARCH)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=\"${USE_ARCH}\"")
        # # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=\"${USE_ARCH}\"")
        message("USE_ARCH = ${USE_ARCH}")
    endif()
    # # # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx -mavx2")
    # # # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx -mavx2 -mavx512f")
    message("Disabling AVX512 because performance is worse and results are wildly different with it on")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-avx512f")
    # # # message("Using 16-bit floats")
    # # # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mf16c")
    # # # # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx10.1")
    # # # # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mprefer-vector-width=512")
    # # # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mprefer-vector-width=256")
    # # # # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mprefer-avx128")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopt-info-vec-all -Wfatal-errors")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopt-info-vec-missed -Wfatal-errors")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopt-info-all -Wfatal-errors")
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopt-info-missed -Wfatal-errors")
    # set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
    add_compile_options(
        "$<$<CONFIG:RELEASE>:-O3;>"
        "$<$<CONFIG:DEBUG>:-O3;-fno-omit-frame-pointer;-g;>"
    )
endif()
message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

add_definitions(-D_POSIX_C_SOURCE=200809L)

# add version to version.cpp from environment variable so C++ can compile it in
set(VERSION $ENV{VERSION})
if ("${VERSION}" STREQUAL "")
    message(WARNING "VERSION IS NOT SET")
    file(STRINGS ../.env CONFIG REGEX "^[ ]*[A-Za-z0-9_]+[ ]*=")
    list(TRANSFORM CONFIG STRIP)
    list(TRANSFORM CONFIG REPLACE "([^=]+)=[ ]*(.*)" "set(\\1 \"\\2\")\n")
    message(${CONFIG})
    cmake_language(EVAL CODE ${CONFIG})
    message("Parsed config")
endif()
message("VERSION = ${VERSION}")
string(TIMESTAMP COMPILE_DATE "%Y-%m-%dT%H:%MZ" UTC)
message("COMPILE_DATE = ${COMPILE_DATE}")
set(VERSION_CODE "const char* VERSION = \"${VERSION}\";\nconst char* COMPILE_DATE = \"${COMPILE_DATE}\";\n")
if(EXISTS src/cpp/version.cpp)
    file(READ src/cpp/version.cpp VERSION_CODE_OLD)
else()
    set(VERSION_CODE_OLD "")
endif()

if (NOT "${VERSION_CODE}" STREQUAL "${VERSION_CODE_OLD}")
    file(WRITE src/cpp/version.cpp "${VERSION_CODE}")
endif()

file(GLOB SOURCES src/cpp/*.cpp)
set_source_files_properties(src/cpp/unstable.cpp PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}")


add_executable(${PROJECT_NAME} ${SOURCES})
# ./usr/lib/x86_64-linux-gnu/cmake/TBB/TBBConfig.cmake

# list(APPEND CMAKE_MODULE_PATH "deps/tbb/cmake/")
list(APPEND CMAKE_MODULE_PATH "/usr/lib/x86_64-linux-gnu/cmake")

# find_package(Threads REQUIRED)
# target_link_libraries(${PROJECT_NAME} PUBLIC geotiff tiff Threads::Threads)

find_package(TBB REQUIRED)
# HACK: ubuntu hides headers in /usr/include/geotiff but other distros don't
if(EXISTS "/usr/include/geotiff")
    include_directories("/usr/include/geotiff")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC geotiff tiff TBB::tbb)


# find_package(TBB REQUIRED)
# # target_link_libraries(${PROJECT_NAME} PUBLIC geotiff tiff Threads::Threads TBB:tbb)
# target_link_libraries(${PROJECT_NAME} PUBLIC geotiff tiff Threads::Threads tbb)
# # target_link_libraries(${PROJECT_NAME} PUBLIC pthread tbb geotiff tiff)

add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ../)

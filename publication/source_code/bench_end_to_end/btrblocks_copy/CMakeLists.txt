# -------------------------------------------------------------------------------
# BtrBlocks
# -------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.12)
project(BtrBlocks VERSION 0.0.1)

# -------------------------------------------------------------------------------
# Define compile-time flags
# -------------------------------------------------------------------------------

option(NO_SIMD "NO_SIMD" OFF)
option(WITH_LOGGING "WITH_LOGGING" OFF)
option(SAMPLING_TEST_MODE "SAMPLING_TEST_MODE" OFF)
option(ENABLE_FOR_SCHEME "ENABLE_FOR_SCHEME" OFF)
option(BUILD_SHARED_LIBRARY "BUILD_SHARED_LIBRARY" ON)

# -------------------------------------------------------------------------------
# C++ build options
# -------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_DEBUG_RELEASE} -g -fPIC")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")
add_compile_options(-march=native -Wno-error)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("cmake/environment.cmake") # Set environment-specific compiler flags
include("cmake/clang-tidy.cmake") # Integrate clang-tidy

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

# BtrBlocks ------------------------------------------------------------------------------------------------------------
FetchContent_Declare(
        roaring
        GIT_REPOSITORY https://github.com/RoaringBitmap/CRoaring.git
        GIT_TAG v0.2.66
        GIT_SHALLOW TRUE)

set(ENABLE_ROARING_TESTS OFF CACHE INTERNAL "")

set(ROARING_BUILD_STATIC ON CACHE INTERNAL "")
FetchContent_MakeAvailable(roaring)

FetchContent_GetProperties(roaring)
SET(CPP_ROARING_HEADERS ${roaring_SOURCE_DIR}/cpp/roaring64map.hh ${roaring_SOURCE_DIR}/cpp/roaring.hh)
file(COPY ${CPP_ROARING_HEADERS} DESTINATION ${roaring_SOURCE_DIR}/include/roaring)



find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(CMAKE_VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# Compression library dependencies
include("${CMAKE_VENDOR_DIR}/fastpfor.cmake")
include("${CMAKE_VENDOR_DIR}/fsst.cmake")
#include("${CMAKE_VENDOR_DIR}/croaring.cmake")
#include("${CMAKE_VENDOR_DIR}/dynamic_bitset.cmake")
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# TODO(open-sourcing) dependencies I'd like to remove from the core library
#include("${CMAKE_VENDOR_DIR}/tbb.cmake")
#include("${CMAKE_VENDOR_DIR}/gflags.cmake")
#include("${CMAKE_VENDOR_DIR}/spdlog.cmake")
#include("${CMAKE_VENDOR_DIR}/yaml-cpp.cmake")
#include("${CMAKE_VENDOR_DIR}/csv-parser.cmake")

# Test dependencies
#include("${CMAKE_VENDOR_DIR}/googletest.cmake")

# Benchmark dependencies
#include("${CMAKE_VENDOR_DIR}/aws-sdk.cmake")
#include("${CMAKE_VENDOR_DIR}/benchmark.cmake")
include("${CMAKE_VENDOR_DIR}/gdouble.cmake")
include("${CMAKE_VENDOR_DIR}/turbo.cmake")
include("${CMAKE_VENDOR_DIR}/lz4.cmake")

# ---------------------------------------------------------------------------
# Components
# ---------------------------------------------------------------------------

include("${CMAKE_CURRENT_SOURCE_DIR}/btrblocks/local.cmake")  # btrblocks library
#include("${CMAKE_CURRENT_SOURCE_DIR}/btrfiles/local.cmake")   # btrblocks files library
#include("${CMAKE_CURRENT_SOURCE_DIR}/test/local.cmake")       # btrblocks tests
#include("${CMAKE_CURRENT_SOURCE_DIR}/tools/local.cmake")      # btrblocks tooling

# ---------------------------------------------------------------------------
# Linting
# ---------------------------------------------------------------------------

if (NOT "${lint_targets}" STREQUAL "")
    add_custom_target(lint)
    add_dependencies(lint ${lint_targets})
endif ()

# ---------------------------------------------------------------------------
# Print Configuration
# ---------------------------------------------------------------------------

message(STATUS "[compiler] settings")
message(STATUS "     CMAKE_CXX_FLAGS            = ${CMAKE_CXX_FLAGS}")
message(STATUS "     CMAKE_CXX_FLAGS_RELEASE    = ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "[btrblocks] settings")
message(STATUS "    NO_SIMD                     = ${NO_SIMD}")
message(STATUS "    WITH_LOGGING                = ${WITH_LOGGING}")
message(STATUS "    SAMPLING_TEST_MODE          = ${SAMPLING_TEST_MODE}")
message(STATUS "    ENABLE_FOR_SCHEME           = ${ENABLE_FOR_SCHEME}")
message(STATUS "    BUILD_SHARED_LIBRARY        = ${BUILD_SHARED_LIBRARY}")
message(STATUS "[test] settings")
message(STATUS "    GTEST_INCLUDE_DIR           = ${GTEST_INCLUDE_DIR}")
message(STATUS "    GTEST_LIBRARY_PATH          = ${GTEST_LIBRARY_PATH}")


# ----------------------------------------------------------------------------------------------------------------------
message("================================= ALP  ===============================")
cmake_print_variables(BUILD_SHARED_LIBRARY)


message("================================= ALP  ===============================")
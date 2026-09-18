cmake_minimum_required(VERSION 3.16)

project(
    Parseval
    VERSION 0.0.1
    DESCRIPTION "A Numpy-like N-dimensional array library for C++17"
    LANGUAGES CXX
)

# -------------------------------------------------------------------
# Project-wide settings
# -------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDEbInfo" "MinSizeRel")
endif()

# Emit compile_commands_json for tooling (clangd, IDEs, etc.)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Is this project the top-level project, or has it been pulled in with
# add_subdirector()/FetcContent by someone else? Several defaults below
# (tests, examples, install rules) only make sense when we're the top level.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(PARSEVAL_IS_TOP_LEVEL ON)
else()
    set(PARSEVAL_IS_TOP_LEVEL OFF)
endif()

include(GNUInstallDirs)
include(CMakeDependentOption)
include(CMakePackageConfigHelpers)
include(FetchContent)


# -------------------------------------------------------------------
# User-facing options
# -------------------------------------------------------------------
#
# The let a consum trim the library down to what they actually need,
# and let and end-user building this for personal use (no system package
# manager access, no `sudo`, dependencies not preinstalled) opt in to
# having CMake fetch and build everything itself instead of requiring
# find_package() to succeed.

option(PARSEVAL_BUILD_EXAMPLES   "Build the example programs"           ${PARSEVAL_IS_TOP_LEVEL})
option(PARSEVAL_BUILD_TESTS      "Build the GoogleTest unit tests"      ${PARSEVAL_IS_TOP_LEVEL})
option(PARSEVAL_ENABLE_INSTALL   "Generate install rules/packge config" ${PARSEVAL_IS_TOP_LEVEL})

option(PARSEVAL_ENABLE_EIGEN     "Enable Eigen-backed linear algebra"   ON)
option(PARSEVAL_ENABLE_SQLITE3   "Enable SQLite3 array I/O"             ON)
option(PARSEVAL_ENABLE_NETCDF    "Enable NetCDF array I/O"              ON)
option(PARSEVAL_ENABLE_SIMD      "Enable SSE/SSE4.1 vectorized kernels" OFF)

# The important switch for "personal use / no dependencies preinstalled": 
# when ON, any dependency that find_package() can't locate on the system is
# transparently fetched and build from source instead of failling configure.
option(PARSEVAL_FETCH_MISSING_DEPS
    "If dependency isn't found on the system, download and build it with FetchContent instead of failing"
    ON
)

# Pinned versions used for the FetchContent fallback paths.
set(PARSEVAL_EIGEN_VERSION      "3.4.0" CACHE STRING "Eigen version to fetch if not found on the system")
set(PARSEVAL_SQLITE3_VERSION    "3.46.1" CACHE STRING "SQLite3 amalgamation version to fetch if not found on the system")
set(PARSEVAL_NETCDF_CXX4_TAG    "v4.3.1" CACHE STRING "netcdf-cxx4 git tag to fetch if not found on the system")
set(PARSEVAL_GOOGLETEST_VERSION "v1.15.2" CACHE STRING "GoogleTest version to fetch for tests")


message(STATUS "Parseval ${PROJECT_VERSION}")
message(STATUS "  PARSEVAL_BUILD_EXAMPLES     = ${PARSEVAL_BUILD_EXAMPLES}")
message(STATUS "  PARSEVAL_BUILD_TESTS        = ${PARSEVAL_BUILD_TESTS}")
message(STATUS "  PARSEVAL_ENABLE_EIGEN       = ${PARSEVAL_ENABLE_EIGEN}")
message(STATUS "  PARSEVAL_ENABLE_SQLITE3     = ${PARSEVAL_ENABLE_SQLITE3}")
message(STATUS "  PARSEVAL_ENABLE_NETCDF      = ${PARSEVAL_ENABLE_NETCDF}")
message(STATUS "  PARSEVAL_ENABLE_SIMD        = ${PARSEVAL_ENABLE_SIMD}")
message(STATUS "  PARSEVAL_FETCH_MISSING_DEPS = ${PARSEVAL_FETCH_MISSING_DEPS}")

# -------------------------------------------------------------------
# Core library target 
# -------------------------------------------------------------------
add_subdirectory(src)

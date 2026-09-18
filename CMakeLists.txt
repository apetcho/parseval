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

# A place to collect variables that need to be re-derived by anyone
# consuming an installed copy of this package (see parseval-config.cmake below).
set(PARSEVAL_INSTALLED_DEPENDENCIES "")


message(STATUS "Parseval ${PROJECT_VERSION}")
message(STATUS "  PARSEVAL_BUILD_EXAMPLES     = ${PARSEVAL_BUILD_EXAMPLES}")
message(STATUS "  PARSEVAL_BUILD_TESTS        = ${PARSEVAL_BUILD_TESTS}")
message(STATUS "  PARSEVAL_ENABLE_EIGEN       = ${PARSEVAL_ENABLE_EIGEN}")
message(STATUS "  PARSEVAL_ENABLE_SQLITE3     = ${PARSEVAL_ENABLE_SQLITE3}")
message(STATUS "  PARSEVAL_ENABLE_NETCDF      = ${PARSEVAL_ENABLE_NETCDF}")
message(STATUS "  PARSEVAL_ENABLE_SIMD        = ${PARSEVAL_ENABLE_SIMD}")
message(STATUS "  PARSEVAL_FETCH_MISSING_DEPS = ${PARSEVAL_FETCH_MISSING_DEPS}")


# -------------------------------------------------------------------
# Helper: find a dependency, or fetch+build it if missing and permitted
# -------------------------------------------------------------------
#
# parseval_required_dependency(<display name> FOUND_VAR <var> FETCH <cmake code>)
#
# <cmake code> is only evaluated if the dependency was not found on the system
# AND PARSEVAL_FETCH_MISSING_DEPS is ON. It is expected to defined the same
# targets that find_package() would have (e.g. Eigen3::Eigen).
macro(parseval_warn_or_fail _pkg_name)
    if(PARSEVAL_FETCH_MISSING_DEPS)
        message(STATUS "  -> ${_pkg_name} not found on the system; fetching and building it locally instead")
    else()
        message(FATAL_ERROR
            "${pkg_name} was not found on the system.\n"
            "Either install it (e.g. via your OS package manager) or reconfigure with\n"
            "   -DPARSEVAL_FETCH_MISSING_DEPS=ON\n"
            "to let CMake download and build it automatically. This is recommended for\n"
            "personal/local installs where you don't have (or don't want) system-wide\n"
            "packages."
        )
    endif()
endmacro()

# -------------------------------------------------------------------
# Core library target 
# -------------------------------------------------------------------
add_subdirectory(src)


# -------------------------------------------------------------------
# Examples
# -------------------------------------------------------------------
if(PARSEVAL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()


# -------------------------------------------------------------------
# Tests (GoogleTest, fetched on demand)
# -------------------------------------------------------------------
if(PARSEVAL_BUILD_TESTS)
    add_subdirectory(tests)
endif()


# -------------------------------------------------------------------
# Install rules
# -------------------------------------------------------------------
#
# Supports two common personal-use install layouts out of the box:
#
#   1) System-wide (needs privileges):
#       cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#       cmake --build build
#       sudo cmake --install build
#
#   2) Personal / user-local (no privileges needed, no dependencies
#      preinstalled anywhere else on the machine):
#       cmake -S . -B build \
#           -DCMAKE_INSTALL_PREFIX="$HOME/.local" \
#           -DPARSEVAL_FETCH_MISSING_DEPS=ON
#       cmake --build build
#       cmake --install build
#   Then, in a downstream project:
#       find_package(parseval REQUIRED PATHS "$HOME/.local")
#   or add "$HOME/.local" to CMAKE_PREFIX_PATH.

if(PARSEVAL_ENABLE_INSTALL)
    set(PARSEVAL_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/parseval")

    install(TARGETS parseval
        EXPORT parseval-targets
    )

    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.hpp"
    )

    install(
        EXPORT parseval-targets
        FILE parseval-tagets.cmake
        NAMESPACE parseval::
        DESTINATION "${PARSEVAL_INSTALL_CMAKEDIR}"
    )

    # Package version file, so consumers can request a specific version.
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/parseval-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    # Render the list of find_dependency(...) calls a downstream consumer
    # needs to re-run for each backend this build had enabled.
    string(
        REPLACE ";" "\n"
        PARSEVAL_INSTALLED_DEPENDECIES_BLOCK
        "${PARSEVAL_INSTALLED_DEPENDECIES}"
    )
    set(PARSEVAL_CONFIG_IN_CONTENT "\
include(CMakeFindDependencyMacro)
${PARSEVAL_INSTALLED_DEPENDECIES_BLOCK}
include(\"\${CMAKE_CURRENT_LIST_DIR}/parseval-targets.cmake\")
")

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/parseval-config.cmake.in" "@PACKAGE_INIT@\n${PARSEVAL_CONFIG_IN_CONTENT}")
    configure_package_config_file(
        "${CMAKE_CURRENT_BINARY_DIR}/parseval-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/parseval-config.cmake"
        INSTALL_DESTINATION "${PARSEVAL_INSTALL_CMAKEDIR}"
    )

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/parseval-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/parseval-config-version.cmake"
        DESTINATION "${PARSEVAL_INSTALL_CMAKEDIR}"
    )

    # Convenience: print where things landed after `cmake --install`.
    install(CODE "message(STATUS \"parseval installed under: \${CMAKE_INSTALL_PREFIX}\")")
endif()
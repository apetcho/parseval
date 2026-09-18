add_library(parseval array.cpp thread_pool.cpp)

target_include_directories(
    parseval PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(parseval PUBLIC cxx_std_17)

#[[
target_include_directories(mylib PUBLIC
   $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/mylib>
   $<INSTALL_INTERFACE:include/mylib>  # <prefix>/include/mylib
 )
#]]

# 
# Threads are required by parseval/thread_pool.hpp on every platform that isn't
# Windows-with-MSVC-builtin-threads; find_package(Threads) handles thats
# distinction for use and never needs a fallback.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(parseval PRIVATE Threads::Threads)

# A place to collect variables that need to be re-derived by anyone
# consuming an installed copy of this package (see parseval-config.cmake below).
set(PARSEVAL_INSTALLED_DEPENDENCIES "")

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
# Eigen3 (required: linear algebra backend) used for matrix decomposition
# -------------------------------------------------------------------
if(PARSEVAL_ENABLE_EIGEN)
    find_package(Eigen3 3.3 QUIET NO_MODULE)

    if(NOT Eigen3_FOUND)
        parseval_warn_or_fail("Eigen3")
        if(PARSEVAL_FETCH_MISSING_DEPS)
            FetchContent_Declare(
                eigen
                GIT_REPOSITORY "https://gitlab.com/libeigen/eigen.git"
                GIT_TAG        ${PARSEVAL_EIGEN_VERSION}
                GIT_SHALLOW     TRUE
            )
            # We only need Eigen's headers; skip building its tests/docs/demos.
            set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
            set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
            set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
            set(EIGEN_BUILD_PKGCONFIG OFF CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(eigen)

            if(NOT TARGET Eigen3::Eigen)
                add_library(Eigen3::Eigen ALIAS eigen)
            endif()
        endif()
    endif()
    
    target_link_libraries(parseval PRIVATE Eigen3::Eigen)
    target_compile_definitions(parseval PRIVATE PARSEVAL_WITH_EIGEN)
    list(APPEND PARSEVAL_INSTALLED_DEPENDENCIES "find_package(Eigen3 3.3 NO_MODULE)")
endif()

# -------------------------------------------------------------------
# SQLite3 (required: array I/O backend)
# -------------------------------------------------------------------
if(PARSEVAL_ENABLE_SQLITE3)
    find_package(SQLite3 QUIET)

    if(NOT SQLite3_FOUND AND NOT TARGET SQLite::SQLite3)
        parseval_warn_or_fail("SQLite3")

        if(PARSEVAL_FETCH_MISSING_DEPS)
            # Fetch the official amalgamation (a sing .c/.h pair) and build
            # it ourselves - this avoids depending on autotools/the sqlite build
            # system just get a personal-use copy of the library.
            # NOTE: pin a URL_HASH (e.g. SHA256=...) here once you've verified
            # the amalgamation download for extra supply-chain safety.
            FetchContent_Declare(
                sqlite3
                URL     "https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip"
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
            FetchContent_GetProperties(sqlite3)
            if(NOT sqlite3_POPULATED)
                FetchContent_Populate(sqlite3)
            endif()

            add_library(sqlite3_fetched STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
            target_include_directories(sqlite3_fetched PUBLIC ${sqlite3_SOURCE_DIR})
            target_link_libraries(sqlite3_fetched PUBLIC ${CMAKE_DL_LIBS} Threads::Threads)
            add_library(SQLite::SQLite3 ALIAS sqlite3_fetched)            
        endif()
    endif()

    target_link_libraries(parseval PRIVATE SQLite::SQLite3)
    target_compile_definitions(parseval PRIVATE PARSEVAL_WITH_SQLITE3)
    list(APPEND PARSEVAL_INSTALLED_DEPENDENCIES "find_dependency(SQLite3)")
endif()

# -------------------------------------------------------------------
# NetCDF-C++4 (required: array I/O backend)
# -------------------------------------------------------------------
if(PARSEVAL_ENABLE_NETCDF)
    find_package(netCDFCxx QUIET)
    find_package(NetCDFCxx QUIET)

    if(NOT TARGET NetCDF::netcdf-cxx4)
        parseval_warn_or_fail("netcdf-cxx4")

        if(PARSEVAL_FETCH_MISSING_DEPS)
            # netcdf-cxx4 itself needs the C library (libnetcdf) available one
            # the system; that one is intentionally left to the platform package
            # manage (apt/brew/vcpkg) rather than FetchContent, since it has heavy,
            # platform-specific dependency (HDF5, zlib).
            find_package(netCDF QUIET)
            if(NOT netCDF_FOUND)
                message(FATAL_ERROR
                    "NetCDF-C (libnetcdf) was not found and cannot be fetched "
                    "automatically because of its own native/HDF5 dependencies. "
                    "Install it via your package manager (e.g. 'apt install libnetcdf-dev',"
                    " 'brew install netcdf') and reconfigure, or run with "
                    "-DPARSEVAL_ENBALE_NETCDF=OFF."
                )
            endif()

            FetchContent_Declare(
                netcdf-cxx4
                GIT_REPOSITORY  "https://github.com/Unidata/netcdf-cxx4.git"
                GIT_TAG         ${PARSEVAL_NETCDF_CXX4_TAG}
                GIT_SHALLOW     TRUE
            )
            set(NCXX_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(netcdf-cxx4)

            if(NOT TARGET netCDF::netcdf-cxx4 AND TARGET netcdf-cxx4)
                add_library(netCDF::netcdf-cxx ALIAS netcdf-cxx4)
            endif()
        endif()
    endif()

    target_link_libraries(parseval PRIVATE netCDF::netcdf-cxx4)
    target_compile_definitions(parseval PRIVATE PARSEVAL_WITH_NETCDF)
    list(APPEND PARSEVAL_INSTALLED_DEPENDENCIES "find_dependency(netCDFCxx)")
endif()

# -------------------------------------------------------------------
# SIMD kernels (SSE2 /SSE4.1)
# -------------------------------------------------------------------

if(PARSEVAL_ENABLE_SIMD)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        include(CheckCXXCompilerFlag)
        check_cxx_compiler_flag("-msse4.1" PARSEVAL_HAS_MSSE41)
        if(PARSEVAL_HAS_MSSE41)
            target_compile_options(parseval PRIVATE -msse2 -msse4.1)
        else()
            target_compile_options(parseval PRIVATE -msse2)
        endif()
    elseif(MSVC)
        # x86_64 MSVC already target SSE2 by default; /arch: only applies to x86
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            target_compile_options(parseval PRIVATE /arch:SSE2)
        endif()
    endif()
    target_compile_definitions(parseval PRIVATE PARSEVAL_WITH_SIMD)
endif()
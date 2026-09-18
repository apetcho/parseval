enable_testing()

find_package(GTest QUIET)

if(NOT GTest_FOUND)
    parseval_warn_or_fail("GoogleTest")

    if(PARSEVAL_FETCH_MISSING_DEPS)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(INSTAL_GTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
            googletest
            GIT_REPOSITORY  "https://github.com/google/googletest.git"
            GIT_TAG         ${PARSEVAL_GOOGLETEST_VERSION}
            GIT_SHALLOW     TRUE
        )
        FetchContent_MakeAvailable(googletest)
    endif()
endif()

add_executable(parseval_tests test_parseval.cpp)
target_link_libraries(parseval_tests
    PRIVATE
        parseval::parseval
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(parseval_tests)
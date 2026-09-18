set(PARSEVAL_EXAMPLES
    basic
    use_cases
    io_and_broadcast
    netcdf_multi_var
)

foreach(_example IN LISTS PARSEVAL_EXAMPLES)
    set(_example_src "${_example}.cpp")
    if(EXISTS "${_example_src}")
        add_executable(parseval_${_example} "${_example_src}")
        target_link_libraries(parseval_${_example} PRIVATE parseval::parseval)
    endif()
endforeach()

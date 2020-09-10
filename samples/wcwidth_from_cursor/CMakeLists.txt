get_filename_component(TARGET ${CMAKE_CURRENT_LIST_DIR} NAME)
add_executable(${TARGET}
    main.cpp
)
target_link_libraries(${TARGET}
PRIVATE
    termgrid
    asio
    span
    fmt
    char8
)

project(termgrid)
cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 20)

# asio
add_library(asio INTERFACE)
target_include_directories(asio 
INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}/_external/asio/asio/include
)
target_link_libraries(asio
INTERFACE
    pthread
)

# char8
add_library(char8 INTERFACE)
target_include_directories(char8
INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/_external/char8/include
)

subdirs(
    termgrid
    samples/cursor_move
    samples/unicode_view
)

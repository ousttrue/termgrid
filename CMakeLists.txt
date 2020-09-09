project(termgrid)
cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 20)

set(EXTERNAL_DIR ${CMAKE_CURRENT_LIST_DIR}/_external)

# asio
add_library(asio INTERFACE)
target_include_directories(asio 
INTERFACE 
    ${EXTERNAL_DIR}/asio/asio/include
)
target_link_libraries(asio
INTERFACE
    pthread
)

# span
add_library(span INTERFACE)
target_include_directories(span
INTERFACE
    ${EXTERNAL_DIR}/span/include
)

# char8
add_library(char8 INTERFACE)
target_include_directories(char8
INTERFACE
    ${EXTERNAL_DIR}/char8/include
)

subdirs(
    termgrid
    fmt
    samples/cursor_move
    samples/unicode_view
)

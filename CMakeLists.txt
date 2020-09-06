project(termgrid)
cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 20)

add_library(asio INTERFACE)
target_include_directories(asio 
INTERFACE 
    ${CMAKE_CURRENT_LIST_DIR}/_external/asio/asio/include
)
target_link_libraries(asio
INTERFACE
    pthread
)

add_executable(termgrid)
target_sources(termgrid
PRIVATE
    main.cpp
)
target_link_libraries(termgrid
PRIVATE
    ncurses
    asio
)

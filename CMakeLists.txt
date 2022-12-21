cmake_minimum_required(VERSION 3.22)
project(sqlite)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

find_package(SQLite3 REQUIRED)
find_package(Boost REQUIRED system )
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_subdirectory(test)
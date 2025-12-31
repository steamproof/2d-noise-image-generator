cmake_minimum_required(VERSION 3.16)
project(noise LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(noise
    src/main.cpp
    src/args.cpp
    src/colormap.cpp
    src/stb_impl.cpp
)

target_include_directories(noise PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

if (MSVC)
    target_compile_options(noise PRIVATE /W4)
else()
    target_compile_options(noise PRIVATE -Wall -Wextra -Wpedantic)
endif()
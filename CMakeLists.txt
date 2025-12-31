cmake_minimum_required(VERSION 3.16)
project(2d-noise-image-generator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(2d-noise-image-generator
    src/main.cpp
    src/args.cpp
    src/colormap.cpp
    src/stb_impl.cpp
)

target_include_directories(2d-noise-image-generator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

if (MSVC)
    target_compile_options(2d-noise-image-generator PRIVATE /W4)
else()
    target_compile_options(2d-noise-image-generator PRIVATE -Wall -Wextra -Wpedantic)
endif()
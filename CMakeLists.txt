cmake_minimum_required(VERSION 3.20)

project(ColorTestSDL2)

set(CMAKE_CXX_STANDARD 20)

add_executable(
    ${PROJECT_NAME}
    src/main.cpp
        src/main.hpp
)

find_package(SDL2 REQUIRED)

target_include_directories(
    ${PROJECT_NAME} PRIVATE
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
)

set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)


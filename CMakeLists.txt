cmake_minimum_required(VERSION 3.15)
project(yal_archiver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(archiver ${SOURCES} src/main.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZSTD REQUIRED libzstd)
include_directories(${ZSTD_INCLUDE_DIRS})
target_link_libraries(archiver ${ZSTD_LIBRARIES})

enable_testing()
add_subdirectory(test) 
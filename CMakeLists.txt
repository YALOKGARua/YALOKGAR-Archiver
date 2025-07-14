cmake_minimum_required(VERSION 3.15)
project(yal_archiver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(archiver ${SOURCES} src/main.cpp)

find_package(ZSTD REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(archiver ZSTD::libzstd_static OpenSSL::SSL OpenSSL::Crypto)

enable_testing()
add_subdirectory(test) 
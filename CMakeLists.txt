cmake_minimum_required(VERSION 3.15)
project(yal_archiver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

add_executable(archiver ${SOURCES} src/main.cpp)

find_package(OpenSSL REQUIRED)
find_package(ZSTD CONFIG)
if (ZSTD_FOUND)
  target_link_libraries(archiver zstd::libzstd_static)
else()
  find_path(ZSTD_INCLUDE_DIR zstd.h)
  find_library(ZSTD_LIBRARY NAMES zstd)
  if (ZSTD_INCLUDE_DIR AND ZSTD_LIBRARY)
    target_include_directories(archiver PRIVATE ${ZSTD_INCLUDE_DIR})
    target_link_libraries(archiver ${ZSTD_LIBRARY})
  else()
    message(FATAL_ERROR "ZSTD not found")
  endif()
endif()
target_link_libraries(archiver OpenSSL::SSL OpenSSL::Crypto)

enable_testing() 
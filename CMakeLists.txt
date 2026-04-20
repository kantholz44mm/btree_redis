cmake_minimum_required(VERSION 3.15)

project(BTreeRedis 
    VERSION 1.0 
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# dependencies
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME Boost
    VERSION 1.86.0
    URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.xz
    URL_HASH SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57
    OPTIONS
        "BOOST_ENABLE_CMAKE ON"
        "BOOST_SKIP_INSTALL_RULES ON"
        "BUILD_SHARED_LIBS OFF"
        "BOOST_INCLUDE_LIBRARIES asio\\\;endian\\\;test" # Note the escapes!
)

enable_testing()

# Works for both GCC and Clang
add_compile_options(-Wall -Wextra -Wpedantic)

# Main btree library
add_subdirectory(libbtree)

# Redis server protocol
add_subdirectory(redis_server)

# 4. Create your main executable
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE btree)
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(redis_client VERSION 0.1 LANGUAGES CXX)

CPMAddPackage("gh:redis/hiredis#v1.3.0")

file(GLOB REDIS_CLIENT_SOURCES "src/*.cpp" "src/**/*.cpp")
add_library(redis_client ${REDIS_CLIENT_SOURCES})

target_link_libraries(redis_client PUBLIC hiredis)
target_include_directories(redis_client PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

add_subdirectory(example)

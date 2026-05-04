cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(client_example VERSION 0.1 LANGUAGES CXX)

CPMAddPackage("gh:redis/hiredis#v1.3.0")

file(GLOB CLIENT_EXAMPLE_SOURCES "src/*.cpp" "src/**/*.cpp")
add_executable(client_example ${CLIENT_EXAMPLE_SOURCES})

target_link_libraries(client_example PRIVATE hiredis)

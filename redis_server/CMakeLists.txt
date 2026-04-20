cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(redis_server VERSION 0.1 LANGUAGES CXX)

file(GLOB REDIS_SERVER_SOURCES "src/*.cpp" "src/**/*.cpp")
add_executable(redis_server ${REDIS_SERVER_SOURCES})

if(WIN32)
    target_link_libraries(redis_server PRIVATE wsock32 ws2_32)
endif()

target_link_libraries(redis_server PRIVATE Boost::asio)

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(redis_server VERSION 0.1 LANGUAGES CXX)

file(GLOB REDIS_SERVER_SOURCES "src/*.cpp" "src/**/*.cpp")
add_executable(redis_server ${REDIS_SERVER_SOURCES})

set_target_properties(redis_server PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
)

# add dependencies
include(../cmake/CPM.cmake)

if(WIN32)
    target_link_libraries(redis_server PRIVATE wsock32 ws2_32)
endif()

CPMAddPackage(
        NAME Boost
        VERSION 1.86.0
        URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.xz
        URL_HASH SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57
        OPTIONS
            "BOOST_ENABLE_CMAKE ON"
            "BOOST_SKIP_INSTALL_RULES ON"
            "BUILD_SHARED_LIBS OFF"
            "BOOST_INCLUDE_LIBRARIES container\\\;asio\\\;circular_buffer" # Note the escapes!
)

target_link_libraries(redis_server PRIVATE Boost::asio Boost::container Boost::circular_buffer)

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(ycsb2 VERSION 0.1 LANGUAGES CXX)

file(GLOB YCSB2_SOURCES "src/*.cpp" "src/**/*.cpp")
add_executable(ycsb2 ${YCSB2_SOURCES})

if(WIN32)
    target_link_libraries(ycsb2 PRIVATE wsock32 ws2_32)
endif()

target_link_libraries(ycsb2 PRIVATE redis_client btree zipfc)
target_link_directories(ycsb2 PRIVATE ${ZIPFC_LIB_PATH})

cmake_minimum_required(VERSION 3.15)

project(BTreeRedis 
    VERSION 1.0 
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Works for both GCC and Clang
add_compile_options(-Wall -Wextra -Wpedantic)

# Main btree library
add_subdirectory(libbtree)

# Redis server protocol
add_subdirectory(redis_server)

# 4. Create your main executable
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE btree)
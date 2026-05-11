cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(zipfc VERSION 0.1 LANGUAGES CXX)

add_custom_target(
    build_rust_zipfc ALL
    COMMAND cargo build --release
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Rust library zipfc"
)

set(RUST_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/target/release)

add_library(zipfc STATIC IMPORTED)
set_target_properties(zipfc PROPERTIES
    IMPORTED_LOCATION ${RUST_LIB_PATH}/libzipfc.a
)

add_dependencies(zipfc build_rust_zipfc)

set(ZIPFC_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/target/release CACHE STRING "Path to the Rust zipfc library")

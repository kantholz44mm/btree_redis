cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(zipfc VERSION 0.1 LANGUAGES CXX)

if(NOT CARGO_PATH)
    find_program(_CARGO_PATH cargo PATHS "~/.cargo/bin")
    if(_CARGO_PATH)
        set(CARGO_PATH ${_CARGO_PATH} CACHE FILEPATH "Path to cargo executable")
    else()
        message(STATUS "Note: 'cargo' not found at configure time. The 'build_rust_zipfc' target will fail during build if Cargo is not available or CARGO_PATH isn't set.")
        set(ZIPFC_CARGO_EXECUTABLE "cargo")
    endif()
endif()

if(NOT DEFINED ZIPFC_CARGO_EXECUTABLE)
    if(CARGO_PATH)
        set(ZIPFC_CARGO_EXECUTABLE ${CARGO_PATH})
    else()
        set(ZIPFC_CARGO_EXECUTABLE cargo)
    endif()
endif()

add_custom_target(
    build_rust_zipfc ALL
    COMMAND ${ZIPFC_CARGO_EXECUTABLE} build --release
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Rust library zipfc"
)

set(RUST_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/target/release)

add_library(zipfc STATIC IMPORTED)
set_target_properties(zipfc PROPERTIES
    IMPORTED_LOCATION ${RUST_LIB_PATH}/libzipfc.a
)

add_dependencies(zipfc build_rust_zipfc)

set(ZIPFC_TARGET_PATH ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "Path to the Rust zipfc target")
set(ZIPFC_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/target/release CACHE STRING "Path to the Rust zipfc library")

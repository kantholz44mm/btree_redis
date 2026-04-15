
# Let's try and find all .cpp files ...
file(GLOB BTREE_SOURCES "src/*.cpp")

# ... and add them to a library
add_library(btree STATIC ${BTREE_SOURCES})

# we need AVX2 for fast hashing.
target_compile_options(btree PRIVATE -mavx2 -fno-exceptions)

# anything that links to this lib will also need the headers.
target_include_directories(btree PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

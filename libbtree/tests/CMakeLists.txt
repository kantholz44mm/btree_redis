# tests/CMakeLists.txt

# Create the test runner
add_executable(libbtree_unit_tests 
    test_main.cpp 
    test_insert_lookup.cpp
    test_range_scans.cpp
    test_removal.cpp
    test_stress.cpp
    test_update.cpp
)

# Link against your library and the Boost framework provided by the root
target_link_libraries(libbtree_unit_tests 
    PRIVATE 
        btree 
        Boost::unit_test_framework
)

# Register with CTest
add_test(NAME BTree.UnitTests COMMAND libbtree_unit_tests --log_level=test_suite)
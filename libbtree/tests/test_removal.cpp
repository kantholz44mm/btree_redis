#include <boost/test/unit_test.hpp>
#include "btree2020.hpp"

BOOST_AUTO_TEST_SUITE(RemovalSuite)

BOOST_AUTO_TEST_CASE(BasicDelete) {
    DataStructureWrapper t(true);
    std::string key = "doomed_key";
    uint64_t val = 666;

    t.insert((uint8_t*)key.data(), key.size(), (uint8_t*)&val, 8);
    
    // Ensure it's there
    unsigned pSize;
    BOOST_REQUIRE(t.lookup((uint8_t*)key.data(), key.size(), pSize) != nullptr);

    // Remove it
    bool removed = t.remove((uint8_t*)key.data(), key.size());
    BOOST_CHECK_EQUAL(removed, true);

    // Ensure it's gone
    BOOST_CHECK(t.lookup((uint8_t*)key.data(), key.size(), pSize) == nullptr);
    
    // Try removing it again (should return false)
    BOOST_CHECK_EQUAL(t.remove((uint8_t*)key.data(), key.size()), false);
}

BOOST_AUTO_TEST_SUITE_END()
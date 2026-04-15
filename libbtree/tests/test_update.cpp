#include <boost/test/unit_test.hpp>
#include "btree2020.hpp"

BOOST_AUTO_TEST_SUITE(UpdateSuite)

BOOST_AUTO_TEST_CASE(UpdateViaDeleteInsert) {
    DataStructureWrapper t(true);
    std::string key = "stable_key";
    uint64_t val1 = 100;
    uint64_t val2 = 200;

    // Initial insert
    t.insert((uint8_t*)key.data(), key.size(), (uint8_t*)&val1, 8);
    
    // To update: must remove first
    bool removed = t.remove((uint8_t*)key.data(), key.size());
    BOOST_REQUIRE(removed); 

    // Re-insert with new value
    t.insert((uint8_t*)key.data(), key.size(), (uint8_t*)&val2, 8);

    unsigned pSize;
    uint8_t* payload = t.lookup((uint8_t*)key.data(), key.size(), pSize);
    BOOST_CHECK_EQUAL(*reinterpret_cast<uint64_t*>(payload), 200);
}

BOOST_AUTO_TEST_SUITE_END()
#include <boost/test/unit_test.hpp>
#include <boost/endian/conversion.hpp>
#include "btree2020.hpp"

BOOST_AUTO_TEST_SUITE(RangeSuite)

BOOST_AUTO_TEST_CASE(OrderedRangeScan) {
    DataStructureWrapper t(true);
    
    // Insert keys: 10, 20, 30, 40, 50
    for (uint32_t i = 10; i <= 50; i += 10) {
        uint32_t key = boost::endian::native_to_big(i);
        t.insert(reinterpret_cast<uint8_t*>(&key), 4, reinterpret_cast<uint8_t*>(&i), 8);
    }

    // Range lookup starting from 25
    uint32_t start_val = 25;
    uint32_t start_key = boost::endian::native_to_big(start_val);
    uint8_t keyBuffer[2048]; // BTreeNode::maxKVSize
    int found_count = 0;
    uint32_t expected_vals[] = {30, 40, 50};

    t.range_lookup(reinterpret_cast<uint8_t*>(&start_key), 4, keyBuffer, 
        [&](unsigned keyLen, uint8_t* payload, unsigned payloadLen) {
            uint32_t val = *reinterpret_cast<uint32_t*>(payload);
            BOOST_CHECK_EQUAL(val, expected_vals[found_count]);
            found_count++;
            return true; // Keep scanning
        });

    BOOST_CHECK_EQUAL(found_count, 3);
}

BOOST_AUTO_TEST_SUITE_END()
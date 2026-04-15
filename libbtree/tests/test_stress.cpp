#include <boost/test/unit_test.hpp>
#include <boost/endian/conversion.hpp>
#include "btree2020.hpp"

BOOST_AUTO_TEST_SUITE(StressSuite)

BOOST_AUTO_TEST_CASE(LargeVolumeInsert) {
    DataStructureWrapper t(true);
    const uint64_t total_keys = 16000; // this is enough to cause multiple splits.

    // Insert many keys
    for (uint64_t i = 0; i < total_keys; i++) {
        uint64_t key_raw = i;
        uint64_t key_be = boost::endian::native_to_big(key_raw);
        t.insert(reinterpret_cast<uint8_t*>(&key_be), 8, reinterpret_cast<uint8_t*>(&i), 8);
    }

    // Verify a random sample to ensure structural integrity
    uint64_t samples[] = {0, 99, 1500, 4999};
    for (uint64_t s : samples) {
        unsigned pSize;
        uint64_t key_be = boost::endian::native_to_big(s);
        uint8_t* payload = t.lookup(reinterpret_cast<uint8_t*>(&key_be), 8, pSize);
        
        BOOST_REQUIRE_MESSAGE(payload != nullptr, "Sample key " << s << " lost after splits");
        BOOST_CHECK_EQUAL(*reinterpret_cast<uint64_t*>(payload), s);
    }
}

BOOST_AUTO_TEST_SUITE_END()
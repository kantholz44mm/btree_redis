#include <boost/test/unit_test.hpp>
#include <boost/endian/conversion.hpp>
#include "btree2020.hpp"
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_map>

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

BOOST_AUTO_TEST_CASE(DifferentialDeleteTest) {
    DataStructureWrapper t(true);
    std::unordered_map<uint64_t, uint64_t> reference_map;
    std::vector<uint64_t> keys;
    
    const uint64_t total_entries = 100000;

    // 1. Insert 100,000 entries into both
    for (uint64_t i = 0; i < total_entries; i++) {
        uint64_t key_be = boost::endian::native_to_big(i);
        
        t.insert(reinterpret_cast<uint8_t*>(&key_be), 8, reinterpret_cast<uint8_t*>(&i), 8);
        reference_map[i] = i;
        keys.push_back(i);
    }

    // 2. Randomly shuffle and delete half
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(keys.begin(), keys.end(), g);

    size_t delete_count = total_entries / 2;
    for (size_t i = 0; i < delete_count; i++) {
        uint64_t key_to_remove = keys[i];
        uint64_t key_be = boost::endian::native_to_big(key_to_remove);
        
        t.remove(reinterpret_cast<uint8_t*>(&key_be), 8);
        reference_map.erase(key_to_remove);
    }

    // 3. Loop through the reference map and verify every key exists in the BTree
    for (auto const& [key, val] : reference_map) {
        unsigned pSize;
        uint64_t key_be = boost::endian::native_to_big(key);
        uint8_t* payload = t.lookup(reinterpret_cast<uint8_t*>(&key_be), 8, pSize);
        
        BOOST_REQUIRE_MESSAGE(payload != nullptr, "Key " << key << " present in map but missing in BTree");
        BOOST_CHECK_EQUAL(*reinterpret_cast<uint64_t*>(payload), val);
    }
    
    // 4. Final check: Ensure deleted keys are actually gone
    for (size_t i = 0; i < delete_count; i++) {
        unsigned pSize;
        uint64_t key_be = boost::endian::native_to_big(keys[i]);
        uint8_t* payload = t.lookup(reinterpret_cast<uint8_t*>(&key_be), 8, pSize);
        
        BOOST_CHECK_MESSAGE(payload == nullptr, "Key " << keys[i] << " was deleted but still found in BTree");
    }
}

BOOST_AUTO_TEST_SUITE_END()
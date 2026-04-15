#include <boost/test/unit_test.hpp>
#include <boost/endian/conversion.hpp>
#include "btree2020.hpp"
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(BasicInsertionAndLookup)

BOOST_AUTO_TEST_CASE(IntegerKeys) {
    DataStructureWrapper t(true);
    
    // We test 1000 keys to ensure page splits are handled
    for (uint64_t i = 0; i < 1000; i++) {
        // Use boost::endian for portability instead of __builtin_bswap32
        uint32_t key_portable = boost::endian::native_to_big(static_cast<uint32_t>(i));
        
        t.insert(reinterpret_cast<uint8_t*>(&key_portable), sizeof(uint32_t), 
                 reinterpret_cast<uint8_t*>(&i), sizeof(uint64_t));
    }

    for (uint64_t i = 0; i < 1000; i++) {
        unsigned pSize;
        uint32_t key_portable = boost::endian::native_to_big(static_cast<uint32_t>(i));
        uint8_t* payload = t.lookup(reinterpret_cast<uint8_t*>(&key_portable), sizeof(uint32_t), pSize);
        
        BOOST_REQUIRE_MESSAGE(payload != nullptr, "Key " << i << " not found");
        BOOST_CHECK_EQUAL(pSize, sizeof(uint64_t));
        BOOST_CHECK_EQUAL(*reinterpret_cast<uint64_t*>(payload), i);
    }
}

BOOST_AUTO_TEST_CASE(VaryingLengthKeys) {
    DataStructureWrapper t(true);
    std::string key = "foundation_test";
    uint64_t val = 42;
    
    t.insert(reinterpret_cast<uint8_t*>(key.data()), key.size(), 
             reinterpret_cast<uint8_t*>(&val), sizeof(val));
    
    unsigned pSize;
    uint8_t* res = t.lookup(reinterpret_cast<uint8_t*>(key.data()), key.size(), pSize);
    
    BOOST_REQUIRE_MESSAGE(res != nullptr, "String key lookup failed");
    BOOST_CHECK_EQUAL(pSize, sizeof(uint64_t));
    BOOST_CHECK_EQUAL(*reinterpret_cast<uint64_t*>(res), val);
    
    // Fixed your previous line: BOOST_CHECK(nullptr != nullptr) would always fail 
    // or be useless. Checking actual pointer validity instead.
    BOOST_CHECK_NE(res, static_cast<uint8_t*>(nullptr));
}

BOOST_AUTO_TEST_SUITE_END()
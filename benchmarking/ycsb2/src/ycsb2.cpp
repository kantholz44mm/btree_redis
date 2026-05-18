#include <algorithm>
#include <csignal>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <chrono>
#include "btree2020.hpp"
#include "RedisClient.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

extern "C" {
void zipf_generate(uint32_t, double, uint32_t*, uint32_t, bool);
void generate_rng4(uint64_t seed, uint32_t count, uint32_t* out);
void generate_rng8(uint64_t seed, uint32_t count, uint64_t* out);
}

static const std::string PAYLOAD = "PAYLOAD";

// zipfParameter is assumed to not change between invocations.
unsigned zipf_next(unsigned num_keys, double zipfParameter, bool shuffle, bool overGenerate) {
    constexpr unsigned GEN_SIZE = 1 << 18;
    static unsigned ARRAY[GEN_SIZE];
    static unsigned index = GEN_SIZE - 1;
    static unsigned generatedNumKeys = 0;

    while (true) {
        // COUNTER(zipf_fail_rate, num_keys > generatedNumKeys, 1 << 10);
        if (num_keys > generatedNumKeys) {
            index = GEN_SIZE - 1;
        }
        index += 1;
        if (index == GEN_SIZE)
            index = 0;
        if (index == 0) {
            generatedNumKeys = num_keys + (overGenerate ? GEN_SIZE / 10 : 0);
            zipf_generate(generatedNumKeys, zipfParameter, ARRAY, GEN_SIZE, shuffle);
        }
        // COUNTER(zipf_reject_rate, ARRAY[index] >= num_keys, 1 << 10);
        if (ARRAY[index] < num_keys) {
            return ARRAY[index];
        }
    }
}

bool op_next() {
    constexpr unsigned GEN_SIZE = 5 * (1 << 18);
    static bool ARRAY[GEN_SIZE];
    static unsigned index = GEN_SIZE - 1;
    index += 1;
    if (index == GEN_SIZE)
        index = 0;
    if (index == 0) {
        unsigned trueCount = GEN_SIZE / 20;
        for (unsigned i = 0; i < GEN_SIZE; ++i)
            ARRAY[i] = i < trueCount;
        std::random_shuffle(ARRAY, ARRAY + GEN_SIZE);
    }
    return ARRAY[index];
}

uint64_t envu64(const char* env) {
    if (getenv(env))
        return strtod(getenv(env), nullptr);
    std::cerr << "missing env " << env << std::endl;
    abort();
}

double envf64(const char* env) {
    if (getenv(env))
        return strtod(getenv(env), nullptr);
    std::cerr << "missing env " << env << std::endl;
    abort();
}

uint8_t* makePayload(unsigned len) {
    // add one to support zero length payload
    uint8_t* payload = new uint8_t[len + 1];
    memset(payload, 42, len);
    return payload;
}

bool keySizeAcceptable(unsigned maxPayload, std::vector<std::string>& data) {
    for (auto& k : data) {
        if (k.size() + maxPayload > BTreeNode::maxKVSize)
            return false;
    }
    return true;
}

void runMulti(
              RedisClient& client,
              std::vector<std::string>& data,
              unsigned keyCount,
              unsigned payloadSize,
              unsigned opCount,
              double zipfParameter,
              unsigned maxScanLength,
              bool dryRun) {
    if (keyCount <= data.size() && keySizeAcceptable(payloadSize, data)) {
        if (!dryRun)
            std::random_shuffle(data.begin(), data.end());
        data.resize(keyCount);
    } else {
        std::cerr << "UNACCEPTABLE" << std::endl;
        keyCount = 0;
        opCount = 0;
    }

    client.run("FLUSHALL").orThrow();
#ifdef USE_STRUCTURE_LITS
    t.impl.bulkInsert(data);
#endif
    unsigned preInsertCount = keyCount - keyCount / 10;
    if (!dryRun)
        for (uint64_t i = 0; i < preInsertCount; i++) {
            const auto key = data[i];
            client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
        }

    {
        // insert
        for (uint64_t i = preInsertCount; i < keyCount; i++) {
            const auto key = data[i];
            client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
        }
    }

    {
        if (!dryRun)
            for (uint64_t i = 0; i < opCount; i++) {
                unsigned keyIndex = zipf_next(keyCount, zipfParameter, false, false);
                assert(keyIndex < data.size());
                const auto key = data[keyIndex];
                const auto payload = client.run("GET %s", key.c_str()).orThrow().getString();
                if (payload != PAYLOAD)
                    throw;
            }
    }

    // TODO range lookup not supported
    /*
    std::minstd_rand generator(std::rand());
    std::uniform_int_distribution<unsigned> scanLengthDistribution{1, maxScanLength};

    {
        uint8_t keyBuffer[BTreeNode::maxKVSize];
        e.setParam("op", "scan");
        BTreeCppPerfEventBlock b(e, t, opCount);
        if (!dryRun)
            for (uint64_t i = 0; i < opCount; i++) {
                unsigned scanLength = scanLengthDistribution(generator);
                unsigned keyIndex = zipf_next(e, keyCount, zipfParameter, false, false);
                assert(keyIndex < data.size());
                uint8_t* key = (uint8_t*)data[keyIndex].data();
                unsigned long keyLen = data[keyIndex].size();
                unsigned foundIndex = 0;
                auto callback = [&](unsigned keyLen, uint8_t* payload, unsigned loadedPayloadLen) {
                    if (payloadSize != loadedPayloadLen) {
                        throw;
                    }
                    foundIndex += 1;
                    return foundIndex < scanLength;
                };
                t.range_lookup(key, keyLen, keyBuffer, callback);
            }
    }
    */

    data.clear();
}

void runYcsbC(RedisClient& client, std::vector<std::string>& data, unsigned keyCount, unsigned payloadSize,
              unsigned opCount, double zipfParameter, bool dryRun) {
    if (keyCount <= data.size() && keySizeAcceptable(payloadSize, data)) {
        if (!dryRun)
            std::random_shuffle(data.begin(), data.end());
        data.resize(keyCount);
    } else {
        std::cerr << "UNACCEPTABLE" << std::endl;
        keyCount = 0;
        opCount = 0;
    }

    client.run("FLUSHALL").orThrow();

#ifdef USE_STRUCTURE_LITS
    t.impl.bulkInsert(data);
#endif
    {
        // insert
        if (!dryRun)
            for (uint64_t i = 0; i < keyCount; i++) {
                const auto& key = data[i];
                client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
            }
    }

    {
        if (!dryRun)
            for (uint64_t i = 0; i < opCount; i++) {
                unsigned keyIndex = zipf_next(keyCount, zipfParameter, false, false);
                assert(keyIndex < data.size());
                const auto& key = data[keyIndex];
                const auto payload = client.run("GET %s", key.c_str()).orThrow().getString();
                if (payload != PAYLOAD)
                    throw;
            }
    }

    data.clear();
}

void runSortedInsert(RedisClient& client, std::vector<std::string>& data, unsigned keyCount, unsigned payloadSize,
                     bool dryRun, bool doSort = true) {
    if (keyCount <= data.size() && keySizeAcceptable(payloadSize, data)) {
        data.resize(keyCount);
        if (!dryRun && doSort) {
            std::sort(data.begin(), data.end());
        }
    } else {
        std::cerr << "UNACCEPTABLE" << std::endl;
        keyCount = 0;
    }


    client.run("FLUSHALL").orThrow();
    {
        // insert
        if (!dryRun)
            for (uint64_t i = 0; i < keyCount; i++) {
                const auto key = data[i];
                client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
            }
    }
    data.clear();
}

bool computeInitialKeyCount(unsigned avgKeyCount,
                            unsigned availableKeyCount,
                            unsigned opCount,
                            unsigned& initialKeyCount,
                            unsigned& reasonableMaxKeys) {
    bool configValid = false;
    initialKeyCount = 0;
    if (avgKeyCount > opCount / 40) {
        initialKeyCount = avgKeyCount - opCount / 40;
        unsigned expectedInsertions = opCount / 20;
        reasonableMaxKeys = initialKeyCount + expectedInsertions * 2;
        if (reasonableMaxKeys <= availableKeyCount) {
            configValid = true;
        } else {
            std::cerr << "not enough keys" << std::endl;
        }
    } else {
        std::cerr << "too many ops for data size" << std::endl;
    }
    return configValid;
}

void runYcsbD(
              RedisClient& client,
              std::vector<std::string>& data,
              unsigned avgKeyCount,
              unsigned payloadSize,
              unsigned opCount,
              double zipfParameter,
              bool dryRun) {
    unsigned initialKeyCount = 0;
    unsigned reasonableMaxKeys = 0;
    if (!computeInitialKeyCount(avgKeyCount, data.size(), opCount, initialKeyCount, reasonableMaxKeys) || !
        keySizeAcceptable(payloadSize, data)) {
        opCount = 0;
        initialKeyCount = 0;
        data.resize(0);
    }

    if (!dryRun)
        std::random_shuffle(data.begin(), data.end());

    client.run("FLUSHALL").orThrow();
    {
        // insert
        if (!dryRun)
            for (uint64_t i = 0; i < initialKeyCount; i++) {
                const auto key = data[i];
                client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
            }
    }

    unsigned insertedCount = initialKeyCount;
    {
        if (!dryRun)
            for (uint64_t completedOps = 0; completedOps < opCount; ++completedOps) {
                if (op_next()) {
                    if (insertedCount == data.size()) {
                        std::cerr << "exhausted keys for insertion" << std::endl;
                        abort();
                    }
                    const auto key = data[insertedCount];
                    client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
                    ++insertedCount;
                } else {
                    unsigned zipfSample = zipf_next(insertedCount, zipfParameter, false, true);
                    unsigned keyIndex = insertedCount - 1 - zipfSample;
                    const auto& key = data[keyIndex];
                    const auto payload = client.run("GET %s", key.c_str()).orThrow().getString();
                    if (payload != PAYLOAD)
                        throw;
                }
            }
    }
}

void runYcsbE(
              RedisClient& client,
              std::vector<std::string>& data,
              unsigned avgKeyCount,
              unsigned payloadSize,
              unsigned opCount,
              unsigned maxScanLength,
              double zipfParameter,
              bool dryRun) {
    unsigned initialKeyCount = 0;
    unsigned reasonableMaxKeys = 0;
    if (!computeInitialKeyCount(avgKeyCount, data.size(), opCount, initialKeyCount, reasonableMaxKeys) || !
        keySizeAcceptable(payloadSize, data)) {
        opCount = 0;
        initialKeyCount = 0;
        data.resize(0);
    }

    if (!dryRun)
        std::random_shuffle(data.begin(), data.end());
    uint8_t* payload = makePayload(payloadSize);
    if (data.size() > 0) {
        // permute zipf indices
        unsigned* permutation = new unsigned[data.size()];
        for (unsigned i = 0; i < data.size(); ++i) {
            permutation[i] = i;
        }
        std::random_shuffle(permutation, permutation + data.size());
        delete[] permutation;
    }
    client.run("FLUSHALL").orThrow();
    {
        // insert
        if (!dryRun)
            for (uint64_t i = 0; i < initialKeyCount; i++) {
                const auto key = data[i];
                client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
            }
    }

    std::minstd_rand generator(std::rand());
    std::uniform_int_distribution<unsigned> scanLengthDistribution{1, maxScanLength};

    unsigned insertedCount = initialKeyCount;
    unsigned sampleIndex = 0;
    {
        if (!dryRun)
            for (uint64_t completedOps = 0; completedOps < opCount; ++completedOps, ++sampleIndex) {
                if (op_next()) {
                    // printf("insert :%lu\n",completedOps);
                    if (insertedCount == reasonableMaxKeys) {
                        std::cerr << "exhausted keys for insertion" << std::endl;
                        abort();
                    }
                    const auto key = data[insertedCount];
                    client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
                    ++insertedCount;
                } else {
                    // printf("range :%lu\n",completedOps);
                    unsigned scanLength = scanLengthDistribution(generator);
                    while (true) {
                        // num_keys for zipf distribution must remain constant to not mess with shuffling permutation.
                        unsigned keyIndex = zipf_next(reasonableMaxKeys, zipfParameter, true, true);
                        // COUNTER(zipf_reject_rate_E, keyIndex >= insertedCount, 1 << 10);
                        if (keyIndex < insertedCount) {
                            // TODO range lookups are not part of the redis protocol for standard key value stores
                            /*
                            uint8_t keyBuffer[BTreeNode::maxKVSize];
                            unsigned foundIndex = 0;
                            const auto key = data[keyIndex];
                            unsigned int keyLen = data[keyIndex].size();
                            auto callback = [&](unsigned keyLen, uint8_t* payload, unsigned loadedPayloadLen) {
                                if (payloadSize != loadedPayloadLen) {
                                    throw;
                                }
                                foundIndex += 1;
                                return foundIndex < scanLength;
                            };
                            t.range_lookup(key, keyLen, keyBuffer, callback);
                            break;
                            */
                        } else {
                            ++sampleIndex;
                        }
                    }
                }
            }
    }
}

void runSortedScan(
    RedisClient& client,
    std::vector<std::string>& data,
    unsigned keyCount,
    unsigned payloadSize,
    unsigned opCount,
    unsigned maxScanLength,
    double zipfParameter,
    bool dryRun) {
    if (keyCount <= data.size()) {
        if (!dryRun)
            std::random_shuffle(data.begin(), data.end());
        data.resize(keyCount);
    } else {
        std::cerr << "not enough keys" << std::endl;
        keyCount = 0;
        opCount = 0;
    }

    uint8_t* payload = makePayload(payloadSize);

    client.run("FLUSHALL").orThrow();
    {
        // insert
        if (!dryRun)
            for (uint64_t i = 0; i < keyCount; i++) {
                const auto key = data[i];
                client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
            }
    }
    uint8_t keyBuffer[BTreeNode::maxKVSize];
    std::minstd_rand generator(std::rand());
    std::uniform_int_distribution<unsigned> scanLengthDistribution{1, maxScanLength};

    // TODO range scan not supported
    /*
    t.range_lookup(payload, 0, keyBuffer, [&](unsigned keyLen, uint8_t* payload, unsigned loadedPayloadLen) {
        return true;
    });

    {
        e.setParam("op", "sorted_scan");
        BTreeCppPerfEventBlock b(e, t, opCount);
        if (!dryRun)
            for (uint64_t i = 0; i < opCount; i++) {
                unsigned keyIndex = zipf_next(e, keyCount, zipfParameter, false, false);
                assert(keyIndex < data.size());
                unsigned scanLength = scanLengthDistribution(generator);

                unsigned foundIndex = 0;
                uint8_t* key = (uint8_t*)data[keyIndex].data();
                unsigned int keyLen = data[keyIndex].size();
                auto callback = [&](unsigned keyLen, uint8_t* payload, unsigned loadedPayloadLen) {
                    if (payloadSize != loadedPayloadLen) {
                        throw;
                    }
                    foundIndex += 1;
                    return foundIndex < scanLength;
                };
                t.range_lookup(key, keyLen, keyBuffer, callback);
            }
    }*/
}

unsigned workloadGenCount(unsigned keyCount, unsigned opCount, unsigned ycsbVariant) {
    switch (envu64("YCSB_VARIANT")) {
    case 3:
        {
            return keyCount;
        }
    case 4:
        {
            return keyCount;
        }
    case 401:
        {
            return keyCount;
        }
    case 402:
        {
            return keyCount;
        }
    case 5:
        {
            return keyCount + opCount;
        }
    case 501:
        {
            return keyCount;
        }
    case 6:
        {
            return keyCount;
        }
    default:
        {
            std::cerr << "bad ycsb variant" << std::endl;
            abort();
        }
    }
}

std::string int_to_key(uint32_t x) {
    std::string s;
    s.resize(4);
    x = __builtin_bswap32(x);
    memcpy(s.data(), &x, 4);
    return s;
}

void lits_escape_strings(std::vector<std::string>& data) {
    unsigned discard_count = 0;
    for (std::string& s : data) {
        for (int i = 0; i < s.size(); ++i) {
            uint8_t c = s[i];
            if (c < 2) {
                s[i] = 1;
                s.insert(i + 1, 1, (char)(c + 1));
                i += 1;
            } else if (c >= 126) {
                c -= 126;
                if (c < 64) {
                    s[i] = 126;
                } else {
                    s[i] = 127;
                    c -= 64;
                }
                s.insert(i + 1, 1, (char)(c + 1));
                i += 1;
            }
        }
        if (s.size() > 255) {
            discard_count += 1;
            s.resize(255);
        }
    }
    if (discard_count > 0) {
        std::cerr << "truncated " << discard_count << " overlong keys for lits escaping" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    auto client = RedisClient::connect(getenv("REDIS_HOST"), envu64("REDIS_PORT"));

    bool dryRun = getenv("DRYRUN");

    std::vector<std::string> data;

    unsigned rand_seed = getenv("SEED") ? envu64("SEED") : time(NULL);
    srand(rand_seed);
    unsigned keyCount = envu64("KEY_COUNT");
    if (!getenv("DATA")) {
        std::cerr << "no keyset" << std::endl;
        abort();
    }
    const char* run_id = getenv("RUN_ID");
    if (!run_id) {
        std::cerr << "WARN: no run_id" << std::endl;
        char* timestmap = new char[64];
        sprintf(timestmap, "%lu",
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now().time_since_epoch()).count());
        run_id = timestmap;
    }
    std::string keySet = getenv("DATA");
    unsigned payloadSize = envu64("PAYLOAD_SIZE");
    unsigned opCount = envu64("OP_COUNT");
    double zipfParameter = envf64("ZIPF");
    double intDensity = envf64("DENSITY");
    unsigned maxScanLength = envu64("SCAN_LENGTH");
    if (maxScanLength == 0) {
        throw;
    }

    if (keySet == "int") {
        std::random_device rd;
        std::mt19937 gen(rd());
        // Create a bernoulli_distribution with the given probability
        std::bernoulli_distribution dist(intDensity);

        // Generate a random boolean value
        bool result = dist(gen);
        unsigned genCount = workloadGenCount(keyCount, opCount, envu64("YCSB_VARIANT")) / intDensity;
        std::vector<uint32_t> v;
        if (dryRun) {
            data.resize(genCount);
        } else {
            data.reserve(genCount);
            for (uint32_t i = 0; data.size() < genCount; i++)
                data.push_back(int_to_key(i));
        }
    } else if (keySet == "rng4") {
        unsigned genCount = workloadGenCount(keyCount, opCount, envu64("YCSB_VARIANT"));
        if (dryRun) {
            data.resize(genCount);
        } else {
            std::vector<uint32_t> v;
            v.resize(genCount);
            generate_rng4(std::rand(), genCount, v.data());
            data.reserve(genCount);
            for (auto x : v) {
                data.push_back(int_to_key(x));
            }
        }
    } else if (keySet == "rng8") {
        unsigned genCount = workloadGenCount(keyCount, opCount, envu64("YCSB_VARIANT"));
        if (dryRun) {
            data.resize(genCount);
        } else {
            std::vector<uint64_t> v;
            v.resize(genCount);
            generate_rng8(std::rand(), genCount, v.data());
            std::string s;
            s.resize(8);
            data.reserve(genCount);
            for (auto x : v) {
                *(uint64_t*)(s.data()) = __builtin_bswap64(x);
                data.push_back(s);
            }
        }
    } else if (keySet == "long1") {
        for (unsigned i = 0; i < keyCount; i++) {
            std::string s;
            for (unsigned j = 0; j < i; j++)
                s.push_back('A');
            data.push_back(s);
        }
    } else if (keySet == "long2") {
        for (unsigned i = 0; i < keyCount; i++) {
            std::string s;
            for (unsigned j = 0; j < i; j++)
                s.push_back('A' + random() % 60);
            data.push_back(s);
        }
    } else if (keySet == "partitioned_id") {
        unsigned partitionCount = maxScanLength;
        std::vector<uint32_t> next_id;
        for (unsigned i = 0; i < partitionCount; ++i)
            next_id.push_back(0);

        std::mt19937 gen(rand_seed);
        std::uniform_int_distribution dist(uint32_t(0), uint32_t(partitionCount - 1));

        data.reserve(keyCount);
        for (uint32_t i = 0; i < keyCount; i++) {
            uint64_t partition = dist(gen);
            uint64_t id = next_id[partition]++;
            union {
                uint64_t key;
                uint8_t keyBytes[8];
            };
            key = __builtin_bswap64(partition << 32 | id);
            data.emplace_back(keyBytes, keyBytes + 8);
        }
    } else {
        std::ifstream in(keySet);
        keySet = "file:" + keySet;
        if (dryRun && keySet == "file:data/access")
            data.resize(6625815);
        else if (dryRun && keySet == "file:data/genome")
            data.resize(262084);
        else if (dryRun && keySet == "file:data/urls")
            data.resize(6393703);
        else if (dryRun && keySet == "file:data/urls-short")
            data.resize(6391379);
        else if (dryRun && keySet == "file:data/wiki")
            data.resize(15772029);
        else if (dryRun) {
            std::cerr << "key count unknown for [" << keySet << "]" << std::endl;
            abort();
        } else {
            std::string line;
            while (getline(in, line)) {
                if (dryRun) {
                    data.emplace_back();
                } else {
                    if (configName == std::string{"art"})
                        line.push_back(0);
                    data.push_back(line);
                }
            }
        }
    }

#ifdef USE_STRUCTURE_LITS
    lits_escape_strings(data);
#endif

    for (unsigned i = 0; i < data.size(); ++i) {
        if (data[i].size() + payloadSize > BTreeNode::maxKVSize) {
            std::cerr << "key too long for page size" << std::endl;
            // this forces the key count check to fail and emits nan values.
            data.clear();
            keyCount = 1;
            break;
        }
    }

    switch (envu64("YCSB_VARIANT")) {
    case 3:
        {
            runYcsbC(client, data, keyCount, payloadSize, opCount, zipfParameter, dryRun);
            break;
        }
    case 4:
        {
            runYcsbD(client, data, keyCount, payloadSize, opCount, zipfParameter, dryRun);
            break;
        }
    case 401:
        {
            runSortedInsert(client, data, keyCount, payloadSize, dryRun);
            break;
        }
    case 402:
        {
            runSortedInsert(client, data, keyCount, payloadSize, dryRun, false);
            break;
        }
    case 5:
        {
            runYcsbE(client, data, keyCount, payloadSize, opCount, maxScanLength, zipfParameter, dryRun);
            break;
        }
    case 501:
        {
            runSortedScan(client, data, keyCount, payloadSize, opCount, maxScanLength, zipfParameter, dryRun);
            break;
        }
    case 6:
        {
            runMulti(client, data, keyCount, payloadSize, opCount, zipfParameter, maxScanLength, dryRun);
            break;
        }
    default:
        {
            std::cerr << "bad ycsb variant" << std::endl;
            abort();
        }
    }

    return 0;
}

#pragma GCC diagnostic pop

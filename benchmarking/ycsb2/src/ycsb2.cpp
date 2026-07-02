#include <algorithm>
#include <csignal>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <chrono>
#include <regex>
#include "btree2020.hpp"
#include "PerfTImer.h"
#include "RedisClient.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

extern "C" {
void zipf_generate(uint32_t, double, uint32_t*, uint32_t, bool);
void generate_rng4(uint64_t seed, uint32_t count, uint32_t* out);
void generate_rng8(uint64_t seed, uint32_t count, uint64_t* out);
}



struct Options {
    bool dryRun;
    uint64_t keyCount;
    uint64_t keyBatchCount;
    uint64_t payloadSize;
    uint64_t opCount;
    uint64_t opBatchCount;
    double zipfParameter;
    uint64_t maxScanLength;
};

std::string int_to_key(uint32_t x) {
    std::string s;
    s.resize(4);
    x = __builtin_bswap32(x);
    memcpy(s.data(), &x, 4);
    return s;
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

bool keySizeAcceptable(unsigned maxPayload, std::vector<std::string>& data) {
    for (auto& k : data) {
        if (k.size() + maxPayload > BTreeNode::maxKVSize)
            return false;
    }
    return true;
}

void runMulti(
                PerfTimer& timer,
              RedisClient& client,
              std::vector<std::string>& data,
              Options& options) {
    if (options.keyCount <= data.size() && keySizeAcceptable(options.payloadSize, data)) {
        if (!options.dryRun)
            std::random_shuffle(data.begin(), data.end());
        data.resize(options.keyCount);
    } else {
        std::cerr << "UNACCEPTABLE" << std::endl;
        options.keyCount = 0;
        options.opCount = 0;
    }

    client.run("FLUSHALL").orThrow();
#ifdef USE_STRUCTURE_LITS
    t.impl.bulkInsert(data);
#endif
    unsigned preInsertCount = options.keyCount - options.keyCount / 10;
    if (!options.dryRun)
        for (uint64_t i = 0; i < preInsertCount; i++) {
            const auto key = data[i];
            client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
        }

    {
        // insert
        timer.setParam("op", "insert90");
        PerfTimerBlock b(timer);
        for (uint64_t i = preInsertCount; i < options.keyCount; i++) {
            const auto key = data[i];
            client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
        }
    }

    {
        timer.setParam("op", "ycsb_c");
        PerfTimerBlock b(timer);
        if (!options.dryRun)
            for (uint64_t i = 0; i < options.opCount; i++) {
                unsigned keyIndex = zipf_next(options.keyCount, options.zipfParameter, false, false);
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

void runYcsbC(
                PerfTimer& timer, RedisClient& client, std::vector<std::string>& data, Options& options) {
    if (options.keyCount <= data.size() && keySizeAcceptable(options.payloadSize, data)) {
        if (!options.dryRun)
            std::random_shuffle(data.begin(), data.end());
        data.resize(options.keyCount);
    } else {
        std::cerr << "UNACCEPTABLE" << std::endl;
        options.keyCount = 0;
        options.opCount = 0;
    }

    client.run("FLUSHALL").orThrow();

#ifdef USE_STRUCTURE_LITS
    t.impl.bulkInsert(data);
#endif
    {
        timer.setParam("op", "ycsb_c_init");
        PerfTimerBlock b(timer);
        // insert
        if (!options.dryRun) {
            auto msetCommand = std::string("MSET");
            for (uint64_t keyIndex = 0; keyIndex < options.keyCount; keyIndex += options.keyBatchCount) {
                auto batch = std::span(data).subspan(keyIndex, std::min(keyIndex + options.keyBatchCount, options.keyCount) - keyIndex);
                auto args = std::vector<std::reference_wrapper<const std::string>>();
                args.reserve(batch.size() * 2 + 1);
                args.emplace_back(msetCommand);
                for (auto& key : batch) {
                    args.emplace_back(key);
                    args.emplace_back(PAYLOAD);
                }
                client.run(args).orThrow();
            }
        }
    }

    {
        timer.setParam("op", "ycsb_c_rehash");
        PerfTimerBlock b(timer);
        // retrieve every key once to trigger potential rehashes to not affect actual benchmark
        if (!options.dryRun) {
            auto msetCommand = std::string("MGET");
            for (uint64_t keyIndex = 0; keyIndex < options.keyCount; keyIndex += options.keyBatchCount) {
                auto batch = std::span(data).subspan(keyIndex, std::min(keyIndex + options.keyBatchCount, options.keyCount) - keyIndex);
                auto args = std::vector<std::reference_wrapper<const std::string>>();
                args.reserve(batch.size() + 1);
                args.emplace_back(msetCommand);
                for (auto& key : batch) {
                    args.emplace_back(key);
                }
                client.run(args).orThrow();
            }
        }
    }

    {
        timer.setParam("op", "ycsb_c");
        PerfTimerBlock b(timer);
        if (!options.dryRun) {
            auto mgetCommand = std::string("MGET");
            for (int64_t remainingOps = options.opCount; remainingOps > 0; remainingOps -= options.opBatchCount) {
                const auto batchSize = std::min(remainingOps, static_cast<int64_t>(options.opBatchCount));
                auto args = std::vector<std::reference_wrapper<const std::string>>();
                args.reserve(batchSize + 1);
                args.emplace_back(mgetCommand);
                for (int64_t i = 0; i < batchSize; i++) {
                    const unsigned keyIndex = zipf_next(options.keyCount, options.zipfParameter, false, false);
                    assert(keyIndex < data.size());
                    const auto& key = data[keyIndex];
                    args.emplace_back(key);
                }
                const auto reply = client.run(args).orThrow();
                for (auto val : reply.getArray()) {
                    if (val.getString() != PAYLOAD)
                        throw;
                }
            }
        }
    }

    data.clear();
}

void runSortedInsert(
                PerfTimer& timer,RedisClient& client, std::vector<std::string>& data, Options& options, bool doSort = true) {
    if (options.keyCount <= data.size() && keySizeAcceptable(options.payloadSize, data)) {
        data.resize(options.keyCount);
        if (!options.dryRun && doSort) {
            std::sort(data.begin(), data.end());
        }
    } else {
        std::cerr << "UNACCEPTABLE" << std::endl;
        options.keyCount = 0;
    }


    client.run("FLUSHALL").orThrow();
    {
        // insert
        timer.setParam("op", "sorted_insert");
        PerfTimerBlock b(timer);
        if (!options.dryRun)
            for (uint64_t i = 0; i < options.keyCount; i++) {
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
                PerfTimer& timer,
              RedisClient& client,
              std::vector<std::string>& data,
              Options& options) {
    unsigned initialKeyCount = 0;
    unsigned reasonableMaxKeys = 0;
    if (!computeInitialKeyCount(options.keyCount, data.size(), options.opCount, initialKeyCount, reasonableMaxKeys) || !
        keySizeAcceptable(options.payloadSize, data)) {
        options.opCount = 0;
        initialKeyCount = 0;
        data.resize(0);
    }

    if (!options.dryRun)
        std::random_shuffle(data.begin(), data.end());

    client.run("FLUSHALL").orThrow();
    {
        timer.setParam("op", "ycsb_d_init");
        PerfTimerBlock b(timer);
        // insert
        if (!options.dryRun)
            for (uint64_t i = 0; i < initialKeyCount; i++) {
                const auto key = data[i];
                client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
            }
    }

    unsigned insertedCount = initialKeyCount;
    {
        timer.setParam("op", "ycsb_d");
        PerfTimerBlock b(timer);
        if (!options.dryRun)
            for (uint64_t completedOps = 0; completedOps < options.opCount; ++completedOps) {
                if (op_next()) {
                    if (insertedCount == data.size()) {
                        std::cerr << "exhausted keys for insertion" << std::endl;
                        abort();
                    }
                    const auto key = data[insertedCount];
                    client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
                    ++insertedCount;
                } else {
                    unsigned zipfSample = zipf_next(insertedCount, options.zipfParameter, false, true);
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
                PerfTimer& timer,
              RedisClient& client,
              std::vector<std::string>& data,
              Options& options) {
    unsigned initialKeyCount = 0;
    unsigned reasonableMaxKeys = 0;
    if (!computeInitialKeyCount(options.keyCount, data.size(), options.opCount, initialKeyCount, reasonableMaxKeys) || !
        keySizeAcceptable(options.payloadSize, data)) {
        options.opCount = 0;
        initialKeyCount = 0;
        data.resize(0);
    }

    if (!options.dryRun)
        std::random_shuffle(data.begin(), data.end());
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
        timer.setParam("op", "ycsb_e_init");
        PerfTimerBlock b(timer);
        // insert
        if (!options.dryRun)
            for (uint64_t i = 0; i < initialKeyCount; i++) {
                const auto key = data[i];
                client.run("SET %s %s", key.c_str(), PAYLOAD.c_str()).orThrow();
            }
    }

    std::minstd_rand generator(std::rand());
    std::uniform_int_distribution<unsigned> scanLengthDistribution{1, options.maxScanLength};

    unsigned insertedCount = initialKeyCount;
    unsigned sampleIndex = 0;
    {
        timer.setParam("op", "ycsb_e");
        PerfTimerBlock b(timer);
        if (!options.dryRun)
            for (uint64_t completedOps = 0; completedOps < options.opCount; ++completedOps, ++sampleIndex) {
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
                        unsigned keyIndex = zipf_next(reasonableMaxKeys, options.zipfParameter, true, true);
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
                PerfTimer& timer,
    RedisClient& client,
    std::vector<std::string>& data,
    Options& options) {
    if (options.keyCount <= data.size()) {
        if (!options.dryRun)
            std::random_shuffle(data.begin(), data.end());
        data.resize(options.keyCount);
    } else {
        std::cerr << "not enough keys" << std::endl;
        options.keyCount = 0;
        options.opCount = 0;
    }

    client.run("FLUSHALL").orThrow();
    {
        timer.setParam("op", "sorted_scan_init");
        PerfTimerBlock b(timer);
        // insert
        if (!options.dryRun) {
            auto msetCommand = std::string("ZADD");
            auto sortedSetKey = std::string("$ROOT$");
            auto dummyScore = std::string("0");
            for (uint64_t keyIndex = 0; keyIndex < options.keyCount; keyIndex += options.keyBatchCount) {
                auto batch = std::span(data).subspan(keyIndex, std::min(keyIndex + options.keyBatchCount, options.keyCount) - keyIndex);
                auto args = std::vector<std::reference_wrapper<const std::string>>();
                args.reserve(batch.size() * 2 + 1);
                args.emplace_back(msetCommand);
                args.emplace_back(sortedSetKey);
                for (auto& key : batch) {
                    args.emplace_back(dummyScore);
                    args.emplace_back(key);
                }
                client.run(args).orThrow();
            }
        }
    }
    std::minstd_rand generator(std::rand());
    std::uniform_int_distribution<unsigned> scanLengthDistribution{1, static_cast<unsigned>(options.maxScanLength)};

    client.run("ZRANGE $ROOT$ %s + BYLEX LIMIT 0 %ld", "[", 99999999999999).orThrow();

    {
        timer.setParam("op", "sorted_scan");
        PerfTimerBlock b(timer);
        if (!options.dryRun) {
            for (int64_t remainingOps = options.opCount; remainingOps > 0; remainingOps -= options.opBatchCount) {
                const auto batchSize = std::min(remainingOps, static_cast<int64_t>(options.opBatchCount));
                for (long i = 0; i < batchSize; ++i) {
                    const unsigned keyIndex = zipf_next(options.keyCount, options.zipfParameter, false, false);
                    assert(keyIndex < data.size());
                    const unsigned scanLength = scanLengthDistribution(generator);
                    const auto key = data[keyIndex];
                    client.appendRun("ZRANGE $ROOT$ [%s + BYLEX LIMIT 0 %ld", key.c_str(), scanLength);
                }
                for (long i = 0; i < batchSize; ++i) {
                    auto res = client.getReply().orThrow();
                    std::cout << "Reply got items: =======================================================" << std::endl;
                    for (const auto& item : res.getArray()) {
                        for (const char c : item.getString()) {
                            std::cout << std::format("{:x} ", c);
                        }
                        std::cout << std::dec << std::endl;
                    }
                }
            }
        }
    }
}

void runMemory(
                PerfTimer& timer, RedisClient& client, std::vector<std::string>& data, Options& options) {
    if (options.keyCount <= data.size() && keySizeAcceptable(options.payloadSize, data)) {
        if (!options.dryRun)
            std::random_shuffle(data.begin(), data.end());
        data.resize(options.keyCount);
    } else {
        std::cerr << "UNACCEPTABLE" << std::endl;
        options.keyCount = 0;
        options.opCount = 0;
    }

    client.run("FLUSHALL").orThrow();
    {
        timer.setParam("op", "ycsb_memory_init");
        timer.setParam("mem", "0");
        PerfTimerBlock b(timer);
        // insert
        if (!options.dryRun) {
            auto msetCommand = std::string("MSET");
            for (uint64_t keyIndex = 0; keyIndex < options.keyCount; keyIndex += options.keyBatchCount) {
                auto batch = std::span(data).subspan(keyIndex, std::min(keyIndex + options.keyBatchCount, options.keyCount) - keyIndex);
                auto args = std::vector<std::reference_wrapper<const std::string>>();
                args.reserve(batch.size() * 2 + 1);
                args.emplace_back(msetCommand);
                for (auto& key : batch) {
                    args.emplace_back(key);
                    args.emplace_back(PAYLOAD);
                }
                client.run(args).orThrow();
            }
        }
    }

    {
        timer.setParam("op", "ycsb_memory_measure");
        PerfTimerBlock b(timer);
        if (!options.dryRun) {
            const auto res = client.run("INFO MEMORY");
            const auto str = res.getString();
            const std::regex pattern("^used_memory_dataset:(\\d+)$", std::regex_constants::multiline);
            std::smatch match;
            if (!std::regex_search(str, match, pattern)) {
                throw std::runtime_error("Could not find used_memory_dataset");
            }
            if (match.size() < 2) {
                throw std::runtime_error("Not enough groups");
            }
            const auto usedMemory = match[1].str();
            timer.setParam("mem", usedMemory);
        }
    }

    data.clear();
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
    case 1001:
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
    client.run("FLUSHALL").orThrow();


    Options options = {
        .dryRun = getenv("DRYRUN") != nullptr,
        .keyCount = envu64("KEY_COUNT"),
        .keyBatchCount = envu64("KEY_BATCH_COUNT"),
        .payloadSize = envu64("PAYLOAD_SIZE"),
        .opCount = envu64("OP_COUNT"),
        .opBatchCount = envu64("OP_BATCH_COUNT"),
        .zipfParameter = envf64("ZIPF"),
        .maxScanLength = envu64("SCAN_LENGTH")
    };

    unsigned rand_seed = getenv("SEED") ? envu64("SEED") : time(NULL);
    srand(rand_seed);
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
    double intDensity = envf64("DENSITY");



    PerfTimer timer = makePerfTimer(keySet, false, options.keyCount);
    timer.setParam("payload_size", options.payloadSize);
    timer.setParam("run_id", run_id);
    timer.setParam("ycsb_zipf", options.zipfParameter);
    timer.setParam("bin_name", std::string{argv[0]});
    timer.setParam("density", intDensity);
    timer.setParam("rand_seed", rand_seed);
    if (options.maxScanLength == 0) {
        throw;
    }
    timer.setParam("ycsb_range_len", options.maxScanLength);

    std::vector<std::string> data;

    if (keySet == "int") {
        std::random_device rd;
        std::mt19937 gen(rd());
        // Create a bernoulli_distribution with the given probability
        std::bernoulli_distribution dist(intDensity);

        // Generate a random boolean value
        bool result = dist(gen);
        unsigned genCount = workloadGenCount(options.keyCount, options.opCount, envu64("YCSB_VARIANT")) / intDensity;
        std::vector<uint32_t> v;
        if (options.dryRun) {
            data.resize(genCount);
        } else {
            data.reserve(genCount);
            for (uint32_t i = 0; data.size() < genCount; i++)
                data.push_back(int_to_key(i));
        }
    } else if (keySet == "rng4") {
        unsigned genCount = workloadGenCount(options.keyCount, options.opCount, envu64("YCSB_VARIANT"));
        if (options.dryRun) {
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
        unsigned genCount = workloadGenCount(options.keyCount, options.opCount, envu64("YCSB_VARIANT"));
        if (options.dryRun) {
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
        for (unsigned i = 0; i < options.keyCount; i++) {
            std::string s;
            for (unsigned j = 0; j < i; j++)
                s.push_back('A');
            data.push_back(s);
        }
    } else if (keySet == "long2") {
        for (unsigned i = 0; i < options.keyCount; i++) {
            std::string s;
            for (unsigned j = 0; j < i; j++)
                s.push_back('A' + random() % 60);
            data.push_back(s);
        }
    } else if (keySet == "partitioned_id") {
        unsigned partitionCount = options.maxScanLength;
        std::vector<uint32_t> next_id;
        for (unsigned i = 0; i < partitionCount; ++i)
            next_id.push_back(0);

        std::mt19937 gen(rand_seed);
        std::uniform_int_distribution dist(uint32_t(0), uint32_t(partitionCount - 1));

        data.reserve(options.keyCount);
        for (uint32_t i = 0; i < options.keyCount; i++) {
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
        if (options.dryRun && keySet == "file:data/access")
            data.resize(6625815);
        else if (options.dryRun && keySet == "file:data/genome")
            data.resize(262084);
        else if (options.dryRun && keySet == "file:data/urls")
            data.resize(6393703);
        else if (options.dryRun && keySet == "file:data/urls-short")
            data.resize(6391379);
        else if (options.dryRun && keySet == "file:data/wiki")
            data.resize(15772029);
        else if (options.dryRun) {
            std::cerr << "key count unknown for [" << keySet << "]" << std::endl;
            abort();
        } else {
            std::string line;
            while (getline(in, line)) {
                if (options.dryRun) {
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
        if (data[i].size() + options.payloadSize > BTreeNode::maxKVSize) {
            std::cerr << "key too long for page size" << std::endl;
            // this forces the key count check to fail and emits nan values.
            data.clear();
            options.keyCount = 1;
            break;
        }
    }

    const auto variant = envu64("YCSB_VARIANT");
    switch (variant) {
    case 3:
        {
            runYcsbC(timer, client, data, options);
            break;
        }
    case 1001:
        {
            runMemory(timer, client, data, options);
            break;
        }
    case 4:
        {
            runYcsbD(timer, client, data, options);
            break;
        }
    case 401:
        {
            runSortedInsert(timer, client, data, options);
            break;
        }
    case 402:
        {
            runSortedInsert(timer, client, data, options, false);
            break;
        }
    case 5:
        {
            runYcsbE(timer, client, data, options);
            break;
        }
    case 501:
        {
            runSortedScan(timer, client, data, options);
            break;
        }
    case 6:
        {
            runMulti(timer, client, data, options);
            break;
        }
    default:
        {
            std::cerr << "bad ycsb variant: " << variant << std::endl;
            abort();
        }
    }

    return 0;
}

#pragma GCC diagnostic pop

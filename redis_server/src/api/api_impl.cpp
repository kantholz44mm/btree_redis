#include "api_impl.h"

#include <optional>
#include <algorithm>

#include "arg_parsing.h"
#include "../resp/resp_types.h"

api_impl::api_impl(BTree& btree): btree(btree) {
}

std::shared_ptr<std::string> api_impl::get(std::string& key) const {
    unsigned payloadSize;
    uint8_t* res = btree.lookupImpl(reinterpret_cast<uint8_t*>(key.data()), key.length(), payloadSize);
    if (res == nullptr) {
        return nullptr;
    }
    return std::make_shared<std::string>(reinterpret_cast<char*>(res), payloadSize);
}

void api_impl::set(std::string& key, std::string& val) const {
    btree.removeImpl(reinterpret_cast<uint8_t*>(key.data()), key.length());
    btree.insertImpl(
        reinterpret_cast<uint8_t*>(key.data()), key.length(),
        reinterpret_cast<uint8_t*>(val.data()), val.length()
    );
}

bool api_impl::del(std::string& key) const {
    return btree.removeImpl(reinterpret_cast<uint8_t*>(key.data()), key.length());
}

bool api_impl::exists(std::string& key) const {
    return btree.removeImpl(reinterpret_cast<uint8_t*>(key.data()), key.length());
}

std::optional<int64_t> api_impl::increment(std::string& key, const int64_t amount) const {
    unsigned payloadSize;
    uint8_t* res = btree.lookupImpl(reinterpret_cast<uint8_t*>(key.data()), key.length(), payloadSize);
    int64_t intVal;
    if (res == nullptr) {
        intVal = 0;
    } else {
        const char* val = reinterpret_cast<char*>(res);
        const auto optInt = arg_parsing::parseIntStrict(val, payloadSize);
        if (!optInt) return {};
        intVal = *optInt;
    }
    intVal += amount;
    // TODO performance can be improved here, allocate global buffer for this, don't use std::string
    auto incrementedVal = std::to_string(intVal);
    btree.removeImpl(reinterpret_cast<uint8_t*>(key.data()), key.length());
    btree.insertImpl(
        reinterpret_cast<uint8_t*>(key.data()), key.length(),
        reinterpret_cast<uint8_t*>(incrementedVal.data()), incrementedVal.length()
    );
    return {intVal};
}

std::vector<std::shared_ptr<std::string>> api_impl::mget(std::span<const resp_value> keys) const {
    auto result = std::vector<std::shared_ptr<std::string>>();
    result.reserve(keys.size());
    std::ranges::transform(keys, std::back_inserter(result), [&](const resp_value& key) {
        return get(*key.getAsString());
    });
    return result;
}

void api_impl::mset(std::span<const resp_value> kvPairs) const {
    auto iter = kvPairs.begin();
    while (iter != kvPairs.end()) {
        auto& key = *iter->getAsString();
        ++iter;
        if (iter == kvPairs.end()) break;
        auto& val = *iter->getAsString();
        set(key, val);
        ++iter;
    }
}

void api_impl::flushAll() const {
    btree.clearImpl();
}

size_t api_impl::getMemory() const {
    return btree.approximateMemoryUsage();
}

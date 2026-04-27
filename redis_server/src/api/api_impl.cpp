#include "api_impl.h"

#include <optional>

api_impl::api_impl(DataStructureWrapper& btree): btree(btree) {
}

std::shared_ptr<std::string> api_impl::get(std::string& key) const {
    unsigned payloadSize;
    uint8_t* res = btree.lookup(reinterpret_cast<uint8_t*>(key.data()), key.length(), payloadSize);
    if (res == nullptr) {
        return nullptr;
    }
    return std::make_shared<std::string>(reinterpret_cast<char*>(res), payloadSize);
}

void api_impl::set(std::string& key, std::string& val) const {
    btree.remove(reinterpret_cast<uint8_t*>(key.data()), key.length());
    btree.insert(
        reinterpret_cast<uint8_t*>(key.data()), key.length(),
        reinterpret_cast<uint8_t*>(val.data()), val.length()
    );
}

bool api_impl::del(std::string& key) const {
    return btree.remove(reinterpret_cast<uint8_t*>(key.data()), key.length());
}

bool api_impl::exists(std::string& key) const {
    return btree.lookup(reinterpret_cast<uint8_t*>(key.data()), key.length());
}

std::optional<int64_t> api_impl::increment(std::string& key, const int64_t amount) const {
    unsigned payloadSize;
    uint8_t* res = btree.lookup(reinterpret_cast<uint8_t*>(key.data()), key.length(), payloadSize);
    int64_t intVal;
    if (res == nullptr) {
        intVal = 0;
    } else {
        const char* val = reinterpret_cast<char*>(res);
        const auto optInt = parseIntStrict(val, payloadSize);
        if (!optInt) return {};
        intVal = *optInt;
    }
    intVal += amount;
    // TODO performance can be improved here, allocate global buffer for this, don't use std::string
    auto incrementedVal = std::to_string(intVal);
    btree.remove(reinterpret_cast<uint8_t*>(key.data()), key.length());
    btree.insert(
        reinterpret_cast<uint8_t*>(key.data()), key.length(),
        reinterpret_cast<uint8_t*>(incrementedVal.data()), incrementedVal.length()
    );
    return {intVal};
}

std::optional<int64_t> api_impl::parseIntStrict(const std::string& str) {
    return parseIntStrict(str.data(), str.data() + str.length());
}

std::optional<int64_t> api_impl::parseIntStrict(const char* buf, const size_t size) {
    return parseIntStrict(buf, buf + size);
}

std::optional<int64_t> api_impl::parseIntStrict(const char* buf, const char* end) {
    if (buf == end) return {};
    int64_t val = 0;
    for (auto i = buf; i != end; ++i) {
        if (*i >= '0' && *i <= '9') {
            val = val * 10 + (*i - '0');
        } else if (*i == '-' && i == buf) {
        } else {
            return {};
        }
    }
    if (buf[0] == '-') val = -val;
    return {val};
}

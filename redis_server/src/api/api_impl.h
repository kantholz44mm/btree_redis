#pragma once

#include <memory>
#include <optional>
#include <span>

#include "../resp/resp_types.h"
#include "btree2020.hpp"

class api_impl {
public:
    explicit api_impl(BTree& btree);

    std::shared_ptr<std::string> get(std::string& key) const;
    void set(std::string& key, std::string& val) const;
    bool del(std::string& key) const;
    bool exists(std::string& key) const;
    std::optional<int64_t> increment(std::string& key, int64_t amount = 1) const;
    std::vector<std::shared_ptr<std::string>> mget(std::span<const resp_value> keys) const;
    void mset(std::span<const resp_value> kvPairs) const;
    void flushAll() const;
    size_t getMemory() const;

    static std::optional<int64_t> parseIntStrict(const std::string& str);
    static std::optional<int64_t> parseIntStrict(const char* buf, size_t size);
    static std::optional<int64_t> parseIntStrict(const char* buf, const char* end);

private:
    BTree& btree;
};

#pragma once

#include <memory>
#include <optional>

#include "btree2020.hpp"

class api_impl {
public:
    explicit api_impl(DataStructureWrapper& btree);

    std::shared_ptr<std::string> get(std::string& key) const;
    void set(std::string& key, std::string& val) const;
    bool del(std::string& key) const;
    bool exists(std::string& key) const;
    std::optional<int64_t> increment(std::string& key, int64_t amount = 1) const;

    static std::optional<int64_t> parseIntStrict(const std::string& str);
    static std::optional<int64_t> parseIntStrict(const char* buf, size_t size);
    static std::optional<int64_t> parseIntStrict(const char* buf, const char* end);

private:
    DataStructureWrapper& btree;
};

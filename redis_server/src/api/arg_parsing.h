#pragma once

#include <optional>
#include <string>
#include <cstdint>

class arg_parsing {
public:
    static std::optional<int64_t> parseIntStrict(const std::string& str);
    static std::optional<int64_t> parseIntStrict(const char* buf, size_t size);
    static std::optional<int64_t> parseIntStrict(const char* buf, const char* end);
};

#include "arg_parsing.h"

std::optional<int64_t> arg_parsing::parseIntStrict(const std::string& str) {
    return parseIntStrict(str.data(), str.data() + str.length());
}

std::optional<int64_t> arg_parsing::parseIntStrict(const char* buf, const size_t size) {
    return parseIntStrict(buf, buf + size);
}

std::optional<int64_t> arg_parsing::parseIntStrict(const char* buf, const char* end) {
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

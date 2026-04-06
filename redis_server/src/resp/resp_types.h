#pragma once

#include <variant>
#include <vector>
#include <string>

class resp_value;

using resp_int = int;
using resp_bulk_string = std::string;
using resp_array = std::vector<resp_value>;

class resp_value {
public:
    using variant_type = std::variant<resp_int, resp_bulk_string, resp_array>;

    resp_value(resp_int val) : value(val) {}
    resp_value(resp_array val) : value(val) {}
    resp_value(resp_bulk_string val) : value(val) {}

    template<typename T>
    bool is() const;

    template<typename T>
    T getAs() const;

private:
    variant_type value;
};

template <typename T>
bool resp_value::is() const {
    return std::holds_alternative<T>(value);
}

template <typename T>
T resp_value::getAs() const {
    return std::get<T>(value);
}

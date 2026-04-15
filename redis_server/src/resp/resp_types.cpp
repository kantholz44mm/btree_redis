#include "resp_types.h"

resp_value resp_value::integer(const int64_t value) {
    return resp_value{resp_integer_value{value}};
}

resp_value resp_value::simple_string(const std::shared_ptr<std::string>& value) {
    return resp_value{resp_simple_string_value{value}};
}

resp_value resp_value::simple_string(const std::string& value) {
    return simple_string(std::make_shared<std::string>(value));
}

resp_value resp_value::bulk_string(const std::shared_ptr<std::string>& value) {
    return resp_value{resp_bulk_string_value{value}};
}

resp_value resp_value::bulk_string(const std::string& value) {
    return bulk_string(std::make_shared<std::string>(value));
}

resp_value resp_value::array(const std::shared_ptr<std::vector<resp_value>>& value) {
    return resp_value{resp_array_value{value}};
}

resp_value resp_value::array(const std::vector<resp_value>& value) {
    return array(std::make_shared<std::vector<resp_value>>(value));
}

std::optional<int64_t> resp_value::getAsInteger() const {
    if (const auto intVal = std::get_if<resp_integer_value>(&value)) {
        return {intVal->value};
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<std::string>> resp_value::getAsString() const {
    if (const auto simpleStringValue = std::get_if<resp_simple_string_value>(&value)) {
        return {simpleStringValue->value};
    }
    if (const auto bulkStringValue = std::get_if<resp_bulk_string_value>(&value)) {
        return {bulkStringValue->value};
    }
    return std::nullopt;
}

std::optional<std::shared_ptr<std::vector<resp_value>>> resp_value::getAsArray() const {
    if (const auto arrayValue = std::get_if<resp_array_value>(&value)) {
        return {arrayValue->value};
    }
    return std::nullopt;
}

bool resp_value::isInteger() const {
    return std::holds_alternative<resp_integer_value>(value);
}

bool resp_value::isSimpleString() const {
    return std::holds_alternative<resp_simple_string_value>(value);
}

bool resp_value::isBulkString() const {
    return std::holds_alternative<resp_bulk_string_value>(value);
}

bool resp_value::isArray() const {
    return std::holds_alternative<resp_array_value>(value);
}

resp_value::resp_value(resp_type_variant value): value(std::move(value)) {}

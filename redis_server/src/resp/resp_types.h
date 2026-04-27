#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>


// https://redis.io/docs/latest/develop/reference/protocol-spec/#resp-protocol-description
namespace resp_type {
    constexpr char STRING = '+';
    constexpr char ERR = '-';
    constexpr char INTEGER = ':';
    constexpr char BULK_STRING = '$';
    constexpr char ARRAY = '*';
    constexpr char NIL = '_';
    constexpr char BOOLEAN = '#';
    constexpr char DOUBLE = ',';
    constexpr char BIG_NUMBER = '(';
    constexpr char BULK_EROR = '!';
    constexpr char VERBATIM_STRING = '=';
    constexpr char MAP = '%';
    constexpr char ATTRIBUTE = '|';
    constexpr char SET = '~';
    constexpr char PUSH = '>';
};

class resp_value;

struct resp_integer_value {
    int64_t value;
};
struct resp_simple_string_value {
    std::shared_ptr<std::string> value;
};
struct resp_error_value {
    std::shared_ptr<std::string> message;
};
struct resp_bulk_string_value {
    std::shared_ptr<std::string> value;
};
struct resp_null_bulk_string_value {
};
struct resp_array_value {
    std::shared_ptr<std::vector<resp_value>> value;
};

using resp_type_variant = std::variant<resp_integer_value, resp_simple_string_value, resp_error_value, resp_bulk_string_value, resp_null_bulk_string_value, resp_array_value>;

class resp_value {
public:
    static resp_value integer(int64_t value);
    static resp_value simple_string(const std::shared_ptr<std::string>& value);
    static resp_value simple_string(const std::string& value);
    static resp_value error(const std::shared_ptr<std::string>& value);
    static resp_value error(const std::string& value);
    static resp_value bulk_string(const std::shared_ptr<std::string>& value);
    static resp_value bulk_string(const std::string& value);
    static resp_value null_bulk_string();
    static resp_value array(const std::shared_ptr<std::vector<resp_value>>& value);
    static resp_value array(const std::vector<resp_value>& value);

    [[nodiscard]] std::optional<int64_t> getAsInteger() const;
    [[nodiscard]] std::shared_ptr<std::string> getAsString() const;
    [[nodiscard]] std::shared_ptr<std::vector<resp_value>> getAsArray() const;

    [[nodiscard]] bool isInteger() const;
    [[nodiscard]] bool isSimpleString() const;
    [[nodiscard]] bool isError() const;
    [[nodiscard]] bool isBulkString() const;
    [[nodiscard]] bool isNullBulkString() const;
    [[nodiscard]] bool isArray() const;
private:
    explicit resp_value(resp_type_variant value);
    resp_type_variant value;
};
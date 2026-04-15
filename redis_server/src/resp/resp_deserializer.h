#pragma once
#include <string>
#include <vector>
#include <istream>
#include <memory>
#include "resp_types.h"

class resp_connection;

class resp_deserializer {
public:
    resp_deserializer(std::istream& input, resp_connection& connection);

    [[nodiscard]] resp_value readValue() const;
    [[nodiscard]] char readByte() const;
    [[nodiscard]] int64_t readInteger() const;
    [[nodiscard]] std::shared_ptr<std::vector<resp_value>> readArray() const;
    [[nodiscard]] std::shared_ptr<std::string> readSimpleString() const;
    [[nodiscard]] std::shared_ptr<std::string> readBulkString() const;
    void expect(char byte) const;
private:
    std::istream& input;
    resp_connection& connection;
};

#include "resp_deserializer.h"

#include <iostream>
#include "resp_connection.h"


resp_deserializer::resp_deserializer(std::istream& input, resp_connection& connection): input(input),
    connection(connection) {
}

resp_value resp_deserializer::readValue() const {
    // https://redis.io/docs/latest/develop/reference/protocol-spec/#resp-protocol-description
    switch (const char valueType = readByte()) {
    case resp_type::INTEGER:
        return resp_value::integer(readInteger());
    case resp_type::STRING:
        return resp_value::simple_string(readSimpleString());
    case resp_type::BULK_STRING:
        return resp_value::bulk_string(readBulkString());
    case resp_type::ARRAY:
        return resp_value::array(readArray());
    default:
        std::cout << "Unknown value type: " << valueType << "(" << std::hex << static_cast<int>(valueType) << std::dec << ")" << std::endl;
        throw std::runtime_error("Unknown value type");
    }
}

char resp_deserializer::readByte() const {
    connection.requireData(1);
    return static_cast<char>(input.get());
}


int64_t resp_deserializer::readInteger() const {
    connection.requireDataUntil('\r');
    long value = 0;
    bool negative = false;
    while (true) {
        const char byte = static_cast<char>(input.get());
        if (std::isdigit(byte)) {
            value *= 10;
            value += byte - '0';
            continue;
        }
        if (byte == '\r') {
            expect('\n');
            break;
        }
        if (byte == '+') {
            continue;
        }
        if (byte == '-') {
            negative = true;
            continue;
        }
        std::ostringstream str;
        str << "Unexpected byte while parsing integer: " << byte;
        throw std::runtime_error(str.str());
    }
    return negative ? -value : value;
}

std::shared_ptr<std::vector<resp_value>> resp_deserializer::readArray() const {
    const int64_t length = readInteger();
    auto arr = std::make_shared<std::vector<resp_value>>();
    arr->reserve(length);
    for (long i = 0; i < length; i++) {
        arr->push_back(readValue());
    }
    return arr;
}

std::shared_ptr<std::string> resp_deserializer::readSimpleString() const {
    connection.requireDataUntil('\r');
    const auto str = std::make_shared<std::string>();
    std::getline(input, *str, '\r');
    expect('\n');
    return str;
}

std::shared_ptr<std::string> resp_deserializer::readBulkString() const {
    const int64_t length = readInteger();
    connection.requireData(length + 2);
    auto str = std::make_shared<std::string>();
    str->reserve(length);
    std::copy_n(std::istreambuf_iterator(input), length, std::back_inserter(*str));
    input.get(); // For some reason, the above doesn't read the last byte
    expect('\r');
    expect('\n');
    return str;
}

void resp_deserializer::expect(const char byte) const {
    const char b = readByte();
    if (b != byte) {
        std::stringstream str;
        str << "Error while reading expected byte: Expected " << byte << ", got " << b << ".";
        throw std::runtime_error(str.str());
    }
}

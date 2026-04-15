#include "resp_serializer.h"

#include <stdexcept>


resp_serializer::resp_serializer(std::ostream& output): output(output) {}

void resp_serializer::writeValue(const resp_value& value) const {
    if (value.isInteger()) {
        writeInteger(value.getAsInteger().value());
    } else if (value.isSimpleString()) {
        writeSimpleString(*value.getAsString().value());
    } else if (value.isBulkString()) {
        writeBulkString(*value.getAsString().value());
    } else if (value.isArray()) {
        writeArray(*value.getAsArray().value());
    } else {
        throw std::runtime_error("Cannot write value, unknown type");
    }
}

void resp_serializer::writeSimpleString(const std::string& str) const {
    output << resp_type::STRING << str << '\r' << '\n';
}

void resp_serializer::writeBulkString(const std::string& str) const {
    output << resp_type::BULK_STRING << str.size() << '\r' << '\n' << str << '\r' << '\n';
}

void resp_serializer::writeInteger(const int64_t value) const {
    output << resp_type::INTEGER << value << '\r' << '\n';
}

void resp_serializer::writeArray(const std::vector<resp_value>& vector) const {
    output << resp_type::ARRAY << vector.size() << '\r' << '\n';
    for (const auto& item : vector) {
        writeValue(item);
    }
}
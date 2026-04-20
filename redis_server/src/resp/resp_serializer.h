#pragma once
#include <string>
#include <vector>
#include <ostream>
#include "resp_types.h"

class resp_serializer {
public:
    explicit resp_serializer(std::ostream& output);

    void writeValue(const resp_value& value) const;
    void writeSimpleString(const std::string& str) const;
    void writeError(const std::string& str) const;
    void writeBulkString(const std::string& str) const;
    void writeInteger(int64_t value) const;
    void writeArray(const std::vector<resp_value>& vector) const;
private:
    std::ostream& output;
};
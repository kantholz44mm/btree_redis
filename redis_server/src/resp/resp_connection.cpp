#include "resp_connection.h"

#include <iomanip>
#include <iostream>

#include "resp_types.h"

class resp_value;

resp_connection::resp_connection(tcp::socket& socket, const command_handler& handler): socket(socket), handler(handler) {
}

void resp_connection::run() {
    sendHello();
    while (true) {
        std::cout << "Waiting for a command" << std::endl;
        const auto command = readCommand();
        handler(command, *this);
    }
}

void resp_connection::send(const resp_value& value) {
    writeValue(value);
    flush();
}

// special method, since there is no difference in C++ types between simple and bulk strings
void resp_connection::sendSimpleString(const std::string& str) {
    writeSimpleString(str);
    flush();
}

void resp_connection::writeValue(const resp_value& value) {
    if (value.is<resp_array>()) {
        writeArray(value.getAs<resp_array>());
    } else if (value.is<resp_bulk_string>()) {
        writeBulkString(value.getAs<resp_bulk_string>());
    } else {
        throw std::runtime_error("Cannot write value");
    }
}

void resp_connection::writeSimpleString(const std::string& str) {
    writeByte(resp_type::STRING);
    writeRawString(std::string(str));
    writeByte('\r');
    writeByte('\n');
}

void resp_connection::writeBulkString(const std::string& str) {
    writeByte(resp_type::BULK_STRING);
    writeIntegerValue(str.size());
    writeRawString(str);
    writeByte('\r');
    writeByte('\n');
}

void resp_connection::writeRawString(const std::string& str) {
    for (const char str1 : str) writeByte(str1);
}

void resp_connection::writeInteger(long value) {
    writeByte(resp_type::INTEGER);
    writeIntegerValue(value);
}

void resp_connection::writeIntegerValue(long value) {
    std::ostringstream str;
    str << value << "\r\n";
    writeRawString(str.str());
}

void resp_connection::writeByte(const char val) {
    writeData.push_back(val);
    if (writeData.size() >= writeData.capacity()) flush();
}

void resp_connection::writeArray(const std::vector<resp_value>& vector) {
    writeByte(resp_type::ARRAY);
    writeIntegerValue(vector.size());
    for (auto item : vector) {
        send(item);
    }
}

void resp_connection::flush() {
    if (writeData.empty()) return;
    boost::system::error_code error;
    const auto elemCount = std::min(writeBuf.size(), writeData.size());
    std::move(writeData.begin(), writeData.begin() + elemCount, writeBuf.begin());
    const auto len = boost::asio::write(socket, boost::asio::buffer(writeBuf, elemCount), error);
    if (error == boost::asio::error::eof) {
        std::cout << "EOF error found" << std::endl;
        throw std::runtime_error("EOF");
    }
    if (error) {
        std::cerr << "Read error: " << error.message() << std::endl;
        return;
    }
    writeData.erase_begin(len);
}

resp_value resp_connection::readValue() {
    char valueType = readByte();
    // https://redis.io/docs/latest/develop/reference/protocol-spec/#resp-protocol-description
    switch (valueType) {
    case resp_type::INTEGER:
        return resp_value(readInteger());
    case resp_type::BULK_STRING:
        return resp_value(readBulkString());
    case resp_type::ARRAY:
        return resp_value(readArray());
    default:
        std::cout << "Unknown value type: " << valueType << "(" << std::hex << static_cast<int>(valueType) << std::dec << ")" << std::endl;
        throw std::runtime_error("Unknown value type");
    }
}

std::vector<std::string> resp_connection::readCommand() {
    const auto val = readValue();
    if (!val.is<resp_array>()) {
        throw std::runtime_error("Expected an array as the command");
    }
    const auto arr = val.getAs<resp_array>();

    std::vector<std::string> command;
    command.reserve(arr.size());
    for (auto resp_value : arr) {
        if (resp_value.is<resp_bulk_string>()) {
            command.push_back(resp_value.getAs<resp_bulk_string>());
        } else {
            throw std::runtime_error("Expected only strings in command");
        }
    }
    return command;
}

void resp_connection::sendHello() {

}

char resp_connection::readByte() {
    requireData(1);
    const char data = *readData.begin();
    readData.pop_front();
    return data;
}


resp_int resp_connection::readInteger() {
    long value = 0;
    bool negative = false;
    while (true) {
        uint8_t byte = readByte();
        if (byte >= '0' && byte <= '9') {
            value *= 10;
            value += byte - '0';
            continue;
        }
        if (byte == '\r') {
            expect(readByte(), '\n');
            break;
        }
        if (byte == '+') {
            continue;
        }
        if (byte == '-') {
            negative = true;
            continue;
        }
        std::ostringstream stream;
        stream << "Unexpected byte: " << byte;
        throw std::runtime_error(stream.str());

    }
    return negative ? -value : value;
}

resp_array resp_connection::readArray() {
    const long length = readInteger();
    resp_array arr{};
    arr.reserve(length);
    for (long i = 0; i < length; i++) {
        arr.push_back(readValue());
    }
    return arr;
}

std::string resp_connection::readBulkString() {
    long length = readInteger();
    std::string str;
    str.reserve(length);
    for (long i = 0; i < length; i++) {
        str.push_back(readByte());
    }
    expect('\r', readByte());
    expect('\n', readByte());
    return str;
}

void resp_connection::requireData(const long bytes) {
    if (static_cast<long>(readData.size()) >= bytes) return;
    if (bytes > static_cast<long>(readData.capacity() - readData.size())) {
        throw std::runtime_error("Cannot require more bytes than readData has space");
    }
    if (bytes > static_cast<long>(readBuf.size())) {
        throw std::runtime_error("Cannot require more bytes than buffer is in size");
    }
    while (static_cast<long>(readData.size()) < bytes) {
        waitData();
    }
}

void resp_connection::waitData() {
    boost::system::error_code error;
    const auto buffer = boost::asio::buffer(readBuf);
    const std::size_t len = socket.read_some(buffer, error);
    if (error == boost::asio::error::eof) {
        std::cout << "EOF error found" << std::endl;
        throw std::runtime_error("EOF");
    }
    if (error) {
        std::cerr << "Read error: " << error.message() << std::endl;
        return;
    }
    std::copy_n(readBuf.begin(), len, std::back_inserter(readData));
}

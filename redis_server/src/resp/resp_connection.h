#pragma once

#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>

#include "resp_types.h"

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

class resp_connection;

using boost::asio::ip::tcp;

using command_handler = std::function<void(const std::vector<std::string>&, resp_connection&)>;

class resp_connection {
public:
    resp_connection(tcp::socket& socket, const command_handler& handler);
    void run();


    // Server to client
    void sendHello();
    void send(const resp_value& value);
    void sendSimpleString(const std::string& str);

private:
    // Writing
    void writeValue(const resp_value& value);
    void writeSimpleString(const std::string& str);
    void writeBulkString(const std::string& str);
    void writeRawString(const std::string& str);
    void writeInteger(long value);
    void writeIntegerValue(long value);
    void writeByte(char val);
    void writeArray(const std::vector<resp_value>& vector);
    void flush();

    // Reading
    resp_value readValue();
    std::vector<std::string> readCommand();
    char readByte();
    resp_int readInteger();
    resp_array readArray();
    resp_bulk_string readBulkString();
    void requireData(long bytes);
    void waitData();

    std::array<char, 256> readBuf{};
    boost::circular_buffer<char> readData{1024};
    std::array<char, 256000> writeBuf{};
    boost::circular_buffer<char> writeData{1024000};

    tcp::socket& socket;
    command_handler handler;

    template<typename T>
    static void expect(T val, T expected);
};

template <typename T>
void resp_connection::expect(T val, T expected) {
    if (val != expected) {
        throw std::runtime_error("Expected");
    }
}

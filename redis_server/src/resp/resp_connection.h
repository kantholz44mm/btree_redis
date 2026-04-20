#pragma once

#include <boost/asio.hpp>

#include "resp_deserializer.h"
#include "resp_serializer.h"

using boost::asio::ip::tcp;

class resp_connection;
class resp_command_context;

using command_handler = std::function<void(resp_command_context&)>;

class connection_closed : public std::runtime_error {
public:
    connection_closed();
    explicit connection_closed(const std::string& message);
    explicit connection_closed(const char* message);
    explicit connection_closed(runtime_error&& e);
    explicit connection_closed(const runtime_error& e);
};

class resp_connection {
public:
    resp_connection(tcp::socket& socket, command_handler  handler);
    void run();

    // Server to client
    void send(const resp_value& value);
    void requireData(std::size_t bytes);
    void requireDataUntil(char byte);
    void flush();

private:
    std::shared_ptr<std::vector<resp_value>> readCommand() const;


    boost::asio::streambuf readBuf;
    std::istream input{&readBuf};
    resp_deserializer deserializer{input, *this};

    boost::asio::streambuf writeBuf;
    std::ostream output{&writeBuf};
    resp_serializer serializer{output};


    tcp::socket& socket;
    command_handler handler;
};

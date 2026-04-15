#pragma once

#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>

#include "resp_deserializer.h"
#include "resp_serializer.h"
#include "resp_types.h"

using boost::asio::ip::tcp;

class resp_connection;

/**
 * Simply a wrapper for a reference to a connection, exposing only the send method.
 */
class resp_sender {
public:
    explicit resp_sender(resp_connection& connection) : connection(connection) {}
    void send(const resp_value& value) const;
private:
    resp_connection& connection;
};

using command_handler = std::function<void(const std::vector<std::shared_ptr<std::string>>&, resp_sender)>;

class connection_closed : public std::runtime_error {
public:
    connection_closed();
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
    std::unique_ptr<std::vector<std::shared_ptr<std::string>>> readCommand() const;


    boost::asio::streambuf readBuf;
    std::istream input{&readBuf};
    resp_deserializer deserializer{input, *this};

    boost::asio::streambuf writeBuf;
    std::ostream output{&writeBuf};
    resp_serializer serializer{output};


    tcp::socket& socket;
    command_handler handler;
};

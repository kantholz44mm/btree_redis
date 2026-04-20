#include "resp_connection.h"

#include <iostream>
#include <utility>
#include <stdexcept>

#include "resp_types.h"
#include "resp_command_context.h"

class resp_value;

connection_closed::connection_closed() : connection_closed("Connection closesd") {}
connection_closed::connection_closed(const std::string& message): runtime_error(message) {}
connection_closed::connection_closed(const char* message): runtime_error(message) {}
connection_closed::connection_closed(runtime_error&& e): runtime_error(e) {}
connection_closed::connection_closed(const runtime_error& e): runtime_error(e) {}

resp_connection::resp_connection(tcp::socket& socket, command_handler  handler): socket(socket), handler(std::move(handler)) {}

void resp_connection::run() {
    try {
        while (true) {
            const auto command = readCommand();
            resp_command_context context{*command, *this};
            handler(context);
        }
    } catch (connection_closed& e) {
        std::cout << "Disconnected: " << e.what() << std::endl;
    }
}

void resp_connection::send(const resp_value& value) {
    serializer.writeValue(value);
    flush();
}


void resp_connection::requireData(const std::size_t bytes) {
    if(readBuf.size() < bytes) {
        boost::system::error_code error;
        boost::asio::read(socket, readBuf, boost::asio::transfer_at_least(bytes - readBuf.size()), error);
        if (error == boost::asio::error::eof) throw connection_closed();
        if (error) {
            std::stringstream str;
            str << "Error reading data (" << bytes << " bytes): " << error.message();
            throw connection_closed(str.str());
        }
    }
}

void resp_connection::requireDataUntil(const char byte) {
    boost::system::error_code error;
    boost::asio::read_until(socket, readBuf, byte, error);
    if (error == boost::asio::error::eof) throw connection_closed();
    if (error) {
        std::stringstream str;
        str << "Error reading data until " << byte << ": " << error.message();
        throw connection_closed(str.str());
    }
}

void resp_connection::flush() {
    if (writeBuf.size() == 0) return;
    boost::system::error_code error;
    boost::asio::write(socket, writeBuf, error);
    if (error == boost::asio::error::eof) throw connection_closed();
    if (error) {
        std::stringstream str;
        str << "Error flushing buffer of size " << writeBuf.size() << ": " << error.message();
        throw connection_closed(str.str());
    }
}

std::shared_ptr<std::vector<resp_value>> resp_connection::readCommand() const {
    const auto val = deserializer.readValue();
    if (!val.isArray()) {
        throw std::runtime_error("Expected an array as the command");
    }
    return *val.getAsArray();
}

#include "resp_connection.h"

#include <iomanip>
#include <iostream>
#include <utility>

#include "resp_types.h"

class resp_value;

void resp_sender::send(const resp_value& value) const {
    connection.send(value);
}

connection_closed::connection_closed(): std::runtime_error("Connection closed") {}

resp_connection::resp_connection(tcp::socket& socket, command_handler  handler): socket(socket), handler(std::move(handler)) {
}

void resp_connection::run() {
    try {
        while (true) {
            std::cout << "Waiting for a command..." << std::endl;
            const auto command = readCommand();
            handler(*command, resp_sender{*this});
        }
    } catch (connection_closed& e) {
        std::cout << e.what() << std::endl;
    }
}

void resp_connection::send(const resp_value& value) {
    serializer.writeValue(value);
    flush();
}


void resp_connection::flush() {
    if (writeBuf.size() == 0) return;
    boost::system::error_code error;
    boost::asio::write(socket, writeBuf, error);
    if (error == boost::asio::error::eof) throw connection_closed();
    if (error) {
        std::stringstream str;
        str << "Error flushing buffer of size " << writeBuf.size() << ": " << error.message();
        throw std::runtime_error(str.str());
    }
}

std::unique_ptr<std::vector<std::shared_ptr<std::string>>> resp_connection::readCommand() const {
    const auto val = deserializer.readValue();
    if (!val.isArray()) {
        throw std::runtime_error("Expected an array as the command");
    }
    const auto arr = val.getAsArray().value();

    auto command = std::make_unique<std::vector<std::shared_ptr<std::string>>>();
    command->reserve(arr->size());
    for (const auto& resp_value : *arr) {
        if (resp_value.isBulkString()) {
            command->push_back(resp_value.getAsString().value());
        } else {
            throw std::runtime_error("Expected only strings in command");
        }
    }
    return command;
}

void resp_connection::requireData(const std::size_t bytes) {
    if(readBuf.size() < bytes) {
        boost::system::error_code error;
        boost::asio::read(socket, readBuf, boost::asio::transfer_at_least(bytes - readBuf.size()), error);
        if (error == boost::asio::error::eof) throw connection_closed();
        if (error) {
            std::stringstream str;
            str << "Error reading data (" << bytes << " bytes): " << error.message();
            throw std::runtime_error(str.str());
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
        throw std::runtime_error(str.str());
    }
}

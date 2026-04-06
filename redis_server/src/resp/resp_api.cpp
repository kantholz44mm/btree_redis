#include "resp_api.h"

#include <iostream>

void resp_api::processCommand(const std::vector<std::string>& command, resp_connection& connection) {
    std::cout << "Received command: ";
    for (auto str : command) {
        std::cout << '"' << str << "\"  ";
    }
    std::cout << std::endl;

    if (command.size() >= 1) {
        const auto& first = command[0];
        if (first == "COMMAND") {
            if (command.at(1) == "DOCS") {
                connection.send(std::vector<resp_value>());
            }
        } else if (first == "info") {
            onInfo(connection);
        } else if (first == "client") {
            if (command.at(1) == "setname") {
                connection.sendSimpleString("OK");
            }
        }
    }
}

void resp_api::onInfo(resp_connection& connection) {
    connection.send(std::string(
        "# Server\r\n"
        "redis_version:8.6.2\r\n"
        "redis_mode:standalone\r\n"
    ));
}
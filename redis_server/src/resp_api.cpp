#include "resp_api.h"

#include <iostream>

void resp_api::processCommand(const std::vector<std::shared_ptr<std::string>>& command, const resp_sender connection) {
    // TODO implement a command parsing utility
    std::cout << "Received command: ";
    for (const auto& str : command) {
        std::cout << '"' << *str << "\"  ";
    }
    std::cout << std::endl;

    if (command.empty()) return;
    const auto& first = *command[0];
    if (first == "COMMAND") {
        if (command.size() >= 2 && *command[1] == "DOCS") {
            connection.send(resp_value::array(std::vector<resp_value>{}));
        }
    } else if (first == "info") {
        onInfo(connection);
    } else if (first == "client") {
        if (*command.at(1) == "setname") {
            connection.send(resp_value::simple_string("OK"));
        }
    } else {
        connection.send(resp_value::simple_string("Unknown command"));
    }
}

/** https://redis.io/docs/latest/commands/info/ */
void resp_api::onInfo(const resp_sender connection) {
    connection.send(resp_value::bulk_string(
        "# Server\r\n"
        "redis_version:8.6.2\r\n"
        "redis_mode:standalone\r\n"
    ));
}
#include "resp_api.h"

#include <iostream>

// ReSharper disable CppMemberFunctionMayBeStatic

void resp_api::processCommand(const resp_command_context& command) {
    std::cout << "Received command: ";
    for (const auto& str : command.getCommand()) {
        std::cout << '"' << *str << "\"  ";
    }
    std::cout << std::endl;


    if (command.isAt(0, "ping")) {
        onPing(command);
        return;
    }
    if (command.isAt(0, "info")) {
        onInfo(command);
        return;
    }

    if (command.isAt(0, "command")) {
        if (command.isAt(1, "docs")) {
            onDocs(command);
            return;
        }
    }
    if (command.isAt(0, "client")) {
        if (command.isAt(1, "setname")) {
            command.respond(resp_value::simple_string("OK"));
            return;
        }
    }
    command.respond(resp_value::error("ERR unknown command"));
}

/** https://redis.io/docs/latest/commands/info/ */
void resp_api::onInfo(const resp_command_context& command) {
    command.respond(resp_value::bulk_string(
        "# Server\r\n"
        "redis_version:8.6.2\r\n"
        "redis_mode:standalone\r\n"
    ));
}

/** https://redis.io/docs/latest/commands/ping/ */
void resp_api::onPing(const resp_command_context& command) {
    command.respond(resp_value::simple_string("PONG"));
}

/** https://redis.io/docs/latest/commands/command-docs/ */
void resp_api::onDocs(const resp_command_context& command) {
    command.respond(resp_value::array(std::vector<resp_value>{}));
}

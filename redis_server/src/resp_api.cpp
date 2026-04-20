#include "resp_api.h"

#include <iostream>
#include "resp/resp_command_context.h"

// #define COMMAND_LOGGING

// ReSharper disable CppMemberFunctionMayBeStatic

void resp_api::processCommand(const resp_command_context& command) {
#ifdef COMMAND_LOGGING
    std::cout << "Received command: ";
    for (const auto& val : command.getCommand()) {
        if (val.isBulkString()) {
            std::cout << '"' << **val.getAsString() << "\"  ";
        }
    }
    std::cout << std::endl;
#endif


    if (command.argIs(0, "ping")) {
        onPing(command);
        return;
    }
    if (command.argIs(0, "info")) {
        onInfo(command);
        return;
    }

    if (command.argIs(0, "command")) {
        if (command.argIs(1, "docs")) {
            onDocs(command);
            return;
        }
    }
    if (command.argIs(0, "client")) {
        if (command.argIs(1, "setname")) {
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

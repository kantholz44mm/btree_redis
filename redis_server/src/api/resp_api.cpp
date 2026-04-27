#include "resp_api.h"

#include <iostream>
#include "../resp/resp_command_context.h"
#include "../resp/resp_connection.h"

// #define COMMAND_LOGGING

// ReSharper disable CppMemberFunctionMayBeStatic

resp_api::resp_api(const api_impl& api) : api(api) {
}

void resp_api::processCommand(const resp_command_context& command) const {
#ifdef COMMAND_LOGGING
    std::cout << "Received command: ";
    for (const auto& val : command.getCommand()) {
        if (val.isBulkString()) {
            std::cout << '"' << **val.getAsString() << "\"  ";
        }
    }
    std::cout << std::endl;
#endif

    // TODO add a proper command tree data structure for better lookup

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
            command.respondOk();
            return;
        }
    }


    if (command.argIs(0, "get")) {
        onGet(command);
        return;
    }
    if (command.argIs(0, "set")) {
        onSet(command);
        return;
    }
    if (command.argIs(0, "del")) {
        onDel(command);
        return;
    }
    if (command.argIs(0, "exists")) {
        onExists(command);
        return;
    }
    if (command.argIs(0, "incr")) {
        onIncr(command);
        return;
    }
    if (command.argIs(0, "incrby")) {
        onIncrBy(command);
        return;
    }
    if (command.argIs(0, "decr")) {
        onDecr(command);
        return;
    }
    if (command.argIs(0, "decrby")) {
        onDecrBy(command);
        return;
    }
    command.respond(resp_value::error("ERR unknown command"));
}

/** https://redis.io/docs/latest/commands/info/ */
void resp_api::onInfo(const resp_command_context& command) const {
    command.respond(resp_value::bulk_string(
        "# Server\r\n"
        "redis_version:8.6.2\r\n"
        "redis_mode:standalone\r\n"
    ));
}

/** https://redis.io/docs/latest/commands/ping/ */
void resp_api::onPing(const resp_command_context& command) const {
    command.respond(resp_value::simple_string("PONG"));
}

/** https://redis.io/docs/latest/commands/command-docs/ */
void resp_api::onDocs(const resp_command_context& command) const {
    command.respond(resp_value::array(std::vector<resp_value>{}));
}

/** https://redis.io/docs/latest/commands/get/ */
void resp_api::onGet(const resp_command_context& command) const {
    const auto key = command.getArgOrNull(1);
    if (!key) {
        command.respondErrorWrongArguments();
        return;
    }
    const auto val = api.get(*key);
    if (!val) {
        command.respond(resp_value::null_bulk_string());
        return;
    }
    command.respond(resp_value::bulk_string(val));
}

/** https://redis.io/docs/latest/commands/set/ */
void resp_api::onSet(const resp_command_context& command) const {
    const auto key = command.getArgOrNull(1);
    if (!key) {
        command.respondErrorWrongArguments();
        return;
    }
    const auto val = command.getArgOrNull(2);
    if (!val) {
        command.respondErrorWrongArguments();
        return;
    }
    api.set(*key, *val);
    command.respondOk();
}

/** https://redis.io/docs/latest/commands/del/ */
void resp_api::onDel(const resp_command_context& command) const {
    if (command.getCommand().size() < 2) {
        command.respondErrorWrongArguments();
        return;
    }
    int64_t removeCount = 0;
    for (const auto& arg : command.varArgs(1)) {
        const auto key = arg.getAsString();
        if (!key) {
            command.respondErrorWrongArguments();
            return;
        }
        const bool removed = api.del(*key);
        if (removed) removeCount++;
    }
    command.respond(resp_value::integer(removeCount));
}

/** https://redis.io/docs/latest/commands/exists/ */
void resp_api::onExists(const resp_command_context& command) const {
    if (command.getCommand().size() < 2) {
        command.respondErrorWrongArguments();
        return;
    }
    int64_t existCount = 0;
    for (const auto& arg : command.varArgs(1)) {
        const auto key = arg.getAsString();
        if (!key) {
            command.respondErrorWrongArguments();
            return;
        }
        const bool exists = api.exists(*key);
        if (exists) existCount++;
    }
    command.respond(resp_value::integer(existCount));
}

/** https://redis.io/docs/latest/commands/incr/ */
void resp_api::onIncr(const resp_command_context& command) const {
    const auto key = command.getArgOrNull(1);
    if (!key) {
        command.respondErrorWrongArguments();
        return;
    }
    const auto result = api.increment(*key);
    if (!result) {
        command.respond(resp_value::error("value is not an integer or out of range"));
        return;
    }
    command.respond(resp_value::integer(*result));
}

/** https://redis.io/docs/latest/commands/incrby/ */
void resp_api::onIncrBy(const resp_command_context& command) const {
    const auto key = command.getArgOrNull(1);
    if (!key) {
        command.respondErrorWrongArguments();
        return;
    }
    const auto amountStr = command.getArgOrNull(2);
    if (!amountStr) {
        command.respondErrorWrongArguments();
        return;
    }
    const auto amount = api_impl::parseIntStrict(*amountStr);
    if (!amount) {
        command.respondErrorNoInteger();
        return;
    }
    const auto result = api.increment(*key, *amount);
    if (!result) {
        command.respondErrorNoInteger();
        return;
    }
    command.respond(resp_value::integer(*result));
}

/** https://redis.io/docs/latest/commands/decr/ */
void resp_api::onDecr(const resp_command_context& command) const {
    const auto key = command.getArgOrNull(1);
    if (!key) {
        command.respondErrorWrongArguments();
        return;
    }
    const auto result = api.increment(*key, -1);
    if (!result) {
        command.respond(resp_value::error("value is not an integer or out of range"));
        return;
    }
    command.respond(resp_value::integer(*result));
}

/** https://redis.io/docs/latest/commands/decrby/ */
void resp_api::onDecrBy(const resp_command_context& command) const {
    const auto key = command.getArgOrNull(1);
    if (!key) {
        command.respondErrorWrongArguments();
        return;
    }
    const auto amountStr = command.getArgOrNull(2);
    if (!amountStr) {
        command.respondErrorWrongArguments();
        return;
    }
    const auto amount = api_impl::parseIntStrict(*amountStr);
    if (!amount) {
        command.respondErrorNoInteger();
        return;
    }
    const auto result = api.increment(*key, -*amount);
    if (!result) {
        command.respondErrorNoInteger();
        return;
    }
    command.respond(resp_value::integer(*result));
}

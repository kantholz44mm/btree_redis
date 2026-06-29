#include "resp_api.h"

#include <iostream>
#include <chrono>
#include <format>
#include <boost/algorithm/string/case_conv.hpp>

#include "../resp/resp_command_context.h"
#include "../resp/resp_connection.h"
#include "arg_parsing.h"

// ReSharper disable CppMemberFunctionMayBeStatic

resp_api::resp_api(const api_impl& api) : api(api) {
    commandLogging = getenv("REDIS_SERVER_COMMAND_LOGGING") != nullptr;
}

void resp_api::processCommand(const resp_command_context& command) const {
    if (commandLogging) {
        logCommand(command);
    }

    const auto rootCommand = command.getArgOrNull(0);
    if (!rootCommand) {
        command.respondErrorWrongArguments();
        return;
    }
    const auto& commandEntry = commandMap.find(boost::algorithm::to_lower_copy(*rootCommand));
    if (commandEntry == commandMap.end()) {
        std::cout << "Unknown command received: ";
        command.respond(resp_value::error(std::format("ERR unknown command '{}'", *rootCommand)));
        return;
    }
    const auto& commandFn = commandEntry->second;
    commandFn(*this, command);
}

/** https://redis.io/docs/latest/commands/info/ */
void resp_api::onInfo(const resp_command_context& command) const {
    std::stringstream str;
    str << "# Server\r\n";
    str << "redis_version:8.6.2\r\n";
    str << "redis_mode:standalone\r\n";
    str << "\r\n";
    str << "# Memory\r\n";
    str << "used_memory_dataset:" << api.getMemory() << "\r\n";
    command.respond(resp_value::bulk_string(str.str()));
}

/** https://redis.io/docs/latest/commands/ping/ */
void resp_api::onPing(const resp_command_context& command) const {
    command.respond(resp_value::simple_string("PONG"));
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
    const auto amount = arg_parsing::parseIntStrict(*amountStr);
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
    const auto amount = arg_parsing::parseIntStrict(*amountStr);
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

/** https://redis.io/docs/latest/commands/mget/ */
void resp_api::onMGet(const resp_command_context& command) const {
    const auto keys = command.getArgs(1);
    auto result = api.mget(keys);
    auto responseArr = std::vector<resp_value>();
    responseArr.reserve(result.size());
    std::ranges::transform(result, std::back_inserter(responseArr), [](const std::shared_ptr<std::string>& val) {
        return resp_value::bulk_string(val);
    });
    command.respond(resp_value::array(responseArr));
}

/** https://redis.io/docs/latest/commands/mset/ */
void resp_api::onMSet(const resp_command_context& command) const {
    const auto kvPairs = command.getArgs(1);
    api.mset(kvPairs);
    command.respondOk();
}

/** https://redis.io/docs/latest/commands/zadd/ */
void resp_api::onZAdd(const resp_command_context& command) const {
    const auto key = command.getArgOrNull(1);
    if (!key) {
        command.respondErrorWrongArguments();
        return;
    }
    if (*key != "$ROOT$") {
        command.respond(resp_value::error("For simplicity, only $ROOT$ is allowed for the key to operate on the entire BTree-Implementation. This is all that's needed for this specific benchmarking use-case"));
        return;
    }
    // option flags NX, XX, GT, LT, CH, INCR are skipped here, this is not to spec but unused anyway
    const auto result = api.zadd(command.getArgs(2));
    command.respond(resp_value::integer(result));
}

/** https://redis.io/docs/latest/commands/zrange/ */
void resp_api::onZRange(const resp_command_context& command) const {
    const auto key = command.getArgOrNull(1);
    auto minArg = command.getArgOrNull(2);
    const auto limitArg = command.getArgOrNull(7);
    if (!key || !minArg || !limitArg) {
        command.respondErrorWrongArguments();
        return;
    }
    if (*key != "$ROOT$") {
        command.respond(resp_value::error("For simplicity, only $ROOT$ is allowed for the key to operate on the entire BTree-Implementation. This is all that's needed for this specific benchmarking use-case"));
        return;
    }
    if (!command.argIs(4, "bylex")) {
        command.respond(resp_value::error("ZRANGE command is only supported with BYLEX"));
        return;
    }
    if (!command.argIs(3, "+")) {
        command.respond(resp_value::error("ZRANGE command is only supported with a max of + (infinite)"));
        return;
    }
    if (!command.argIs(5, "limit")) {
        command.respond(resp_value::error("ZRANGE command requires the LIMIT part"));
        return;
    }
    if (!command.argIs(6, "0")) {
        command.respond(resp_value::error("ZRANGE command requires the LIMIT part"));
        return;
    }
    auto limit = arg_parsing::parseIntStrict(*limitArg);
    if (!limit) {
        command.respondErrorNoInteger();
        return;
    }
    if (minArg->size() < 0 || (*minArg)[0] != '[') {
        command.respond(resp_value::error("ZRANGE min value must start with ["));
        return;
    }
    auto min = minArg->substr(1);
    auto result = api.zrange(min, *limit);
    auto responseArr = std::vector<resp_value>();
    responseArr.reserve(result.size());
    std::ranges::transform(result, std::back_inserter(responseArr), [](const std::shared_ptr<std::string>& val) {
        return resp_value::bulk_string(val);
    });
    command.respond(resp_value::array(responseArr));
}

/** https://redis.io/docs/latest/commands/flushall/ */
void resp_api::onFlushAll(const resp_command_context& command) const {
    api.flushAll();
    command.respondOk();
}

void resp_api::logCommand(const resp_command_context& command) const {
    auto args = std::span(command.getCommand());
    size_t truncated = 0;
    if (args.size() > 5) {
        args = args.subspan(0,5);
        truncated = command.getCommand().size() - args.size();
    }
    auto now = std::chrono::system_clock::now();
    std::cout << std::format("{0:%F_%T} Received command: ", now);
    for (const auto& val : args) {
        if (val.isBulkString()) {
            std::cout << '"' << *val.getAsString() << "\"  ";
        }
    }
    if (truncated != 0) {
        std::cout << "... +" << truncated;
    }
    std::cout << std::endl;
}

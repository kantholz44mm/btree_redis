#pragma once

#include "api_impl.h"

class resp_command_context;

class resp_api {
public:
    explicit resp_api(const api_impl& api);

    void processCommand(const resp_command_context& command) const;

    void onInfo(const resp_command_context& command) const;
    void onPing(const resp_command_context& command) const;

    void onGet(const resp_command_context& command) const;
    void onSet(const resp_command_context& command) const;
    void onDel(const resp_command_context& command) const;
    void onExists(const resp_command_context& command) const;
    void onIncr(const resp_command_context& command) const;
    void onIncrBy(const resp_command_context& command) const;
    void onDecr(const resp_command_context& command) const;
    void onDecrBy(const resp_command_context& command) const;
    void onMGet(const resp_command_context& command) const;
    void onMSet(const resp_command_context& command) const;

    void onZAdd(const resp_command_context& command) const;
    void onZRange(const resp_command_context& command) const;

    void onFlushAll(const resp_command_context& command) const;

private:
    const std::unordered_map<std::string, std::function<void(const resp_api& api, const resp_command_context& command)>> commandMap{
        {"ping", &resp_api::onPing},
        {"info", &resp_api::onInfo},

        {"get", &resp_api::onGet},
        {"set", &resp_api::onSet},
        {"del", &resp_api::onDel},
        {"exists", &resp_api::onExists},
        {"incr", &resp_api::onIncr},
        {"incrby", &resp_api::onIncrBy},
        {"decr", &resp_api::onDecr},
        {"decrby", &resp_api::onDecrBy},
        {"mget", &resp_api::onMGet},
        {"mset", &resp_api::onMSet},

        {"zadd", &resp_api::onZAdd},
        {"zrange", &resp_api::onZRange},

        {"flushall", &resp_api::onFlushAll},
    };

    void logCommand(const resp_command_context& command) const;
    const api_impl& api;
    bool commandLogging;
};

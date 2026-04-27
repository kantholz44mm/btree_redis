#pragma once

#include "api_impl.h"

class resp_command_context;

class resp_api {
public:
    explicit resp_api(const api_impl& api);

    void processCommand(const resp_command_context& command) const;

    void onInfo(const resp_command_context& command) const;
    void onPing(const resp_command_context& command) const;
    void onDocs(const resp_command_context& command) const;

    void onGet(const resp_command_context& command) const;
    void onSet(const resp_command_context& command) const;
    void onDel(const resp_command_context& command) const;
    void onExists(const resp_command_context& command) const;
    void onIncr(const resp_command_context& command) const;
    void onIncrBy(const resp_command_context& command) const;
    void onDecr(const resp_command_context& command) const;
    void onDecrBy(const resp_command_context& command) const;

private:
    const api_impl& api;
};

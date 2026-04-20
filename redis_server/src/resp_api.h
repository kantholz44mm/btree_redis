#pragma once
#include "resp/resp_connection.h"


class resp_api {
public:
    void processCommand(const resp_command_context& command);

    void onInfo(const resp_command_context& command);
    void onPing(const resp_command_context& command);
    void onDocs(const resp_command_context& command);
};

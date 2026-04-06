#pragma once
#include "resp_connection.h"


class resp_api {
public:
    void processCommand(const std::vector<std::string>& command, resp_connection& connection);

    void onInfo(resp_connection& connection);
};

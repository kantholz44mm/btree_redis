#pragma once
#include "resp/resp_connection.h"


class resp_api {
public:
    void processCommand(const std::vector<std::shared_ptr<std::string>>& command, resp_sender connection);

    void onInfo(resp_sender connection);
};

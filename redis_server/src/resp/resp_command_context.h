#pragma once

#include <vector>
#include <memory>
#include <string>

class resp_connection;
class resp_value;

class resp_command_context {
public:
    resp_command_context(const std::vector<resp_value>& command, resp_connection& connection);

    void respond(const resp_value& value) const;

    bool argIs(size_t arg, const char* str) const;

    [[nodiscard]] std::vector<resp_value> getCommand() const;

private:
    const std::vector<resp_value>& command;
    resp_connection& connection;
};

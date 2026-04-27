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
    /** Convenience function for responding with an OK simple string for successful commands with no return */
    void respondOk() const;
    /** Convenience function for responding with an error for command argument mismatch */
    void respondErrorWrongArguments() const;
    /** Convenience function for responding with an error for integer parsing errors */
    void respondErrorNoInteger() const;

    bool argIs(size_t arg, const char* str) const;
    [[nodiscard]] std::shared_ptr<std::string> getArgOrNull(size_t arg) const;

    [[nodiscard]] const std::vector<resp_value>& getCommand() const;
    std::ranges::subrange<std::vector<resp_value>::const_iterator> varArgs(size_t start) const;

private:
    const std::vector<resp_value>& command;
    resp_connection& connection;
};

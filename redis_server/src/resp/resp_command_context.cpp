#include "resp_command_context.h"

#include <boost/algorithm/string.hpp>
#include "resp_connection.h"

resp_command_context::resp_command_context(const std::vector<resp_value>& command,
                                           resp_connection& connection) :
        command(command), connection(connection) {}

void resp_command_context::respond(const resp_value& value) const {
    connection.send(value);
}

void resp_command_context::respondOk() const {
    static const auto ok = resp_value::simple_string("OK");
    respond(ok);
}

void resp_command_context::respondErrorWrongArguments() const {
    static const auto err = resp_value::error("wrong number of arguments for command");
    respond(err);
}

void resp_command_context::respondErrorNoInteger() const {
    static const auto err = resp_value::error("value is not an integer or out of range");
    respond(err);
}

bool resp_command_context::argIs(const size_t arg, const char* str) const {
    return command.size() > arg && command[arg].isBulkString() && boost::iequals(*command[arg].getAsString(), str);
}

std::shared_ptr<std::string> resp_command_context::getArgOrNull(const size_t arg) const {
    if (command.size() > arg && command[arg].isBulkString()) {
        return command[arg].getAsString();
    }
    return {nullptr};
}

const std::vector<resp_value>& resp_command_context::getCommand() const {
    return command;
}

std::ranges::subrange<std::vector<resp_value>::const_iterator> resp_command_context::varArgs(const size_t start) const {
    return std::ranges::subrange(command.begin() + start, command.end());
}

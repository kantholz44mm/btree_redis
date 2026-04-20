#include "resp_command_context.h"

#include <boost/algorithm/string.hpp>
#include "resp_connection.h"

resp_command_context::resp_command_context(const std::vector<std::shared_ptr<std::string>>& command,
                                           resp_connection& connection) :
        command(command), connection(connection) {}

void resp_command_context::respond(const resp_value& value) const {
    connection.send(value);
}

bool resp_command_context::isAt(const size_t arg, const char* str) const {
    return command.size() > arg && boost::iequals(*command[arg], str);
}

std::vector<std::shared_ptr<std::string>> resp_command_context::getCommand() const {
    return command;
}

#include "resp_command_context.h"

#include <boost/algorithm/string.hpp>
#include "resp_connection.h"

resp_command_context::resp_command_context(const std::vector<resp_value>& command,
                                           resp_connection& connection) :
        command(command), connection(connection) {}

void resp_command_context::respond(const resp_value& value) const {
    connection.send(value);
}

bool resp_command_context::argIs(const size_t arg, const char* str) const {
    return command.size() > arg && command[arg].isBulkString() && boost::iequals(**command[arg].getAsString(), str);
}

std::vector<resp_value> resp_command_context::getCommand() const {
    return command;
}

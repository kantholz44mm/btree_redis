#include <iostream>

#include <boost/asio.hpp>

#include "api/resp_api.h"
#include "api/api_impl.h"
#include "resp/resp_connection.h"

#include "btree2020.hpp"

using boost::asio::ip::tcp;

DataStructureWrapper btree(true);
api_impl apiImpl{btree};
resp_api respApi{apiImpl};

int main() {
    boost::asio::io_context io_context;
    int port = 3000;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
    std::cout << "RESP server started on port " << port << ", waiting for connection" << std::endl;
    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "Client connected" << std::endl;
        resp_connection connection(socket, [](const resp_command_context& command) {
            respApi.processCommand(command);
        });
        connection.run();
        std::cout << "Client disconnected" << std::endl;
    }
    return 0;
}

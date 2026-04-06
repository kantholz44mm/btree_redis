#include <iostream>

#include <boost/asio.hpp>

#include "resp/resp_api.h"
#include "resp/resp_connection.h"

using boost::asio::ip::tcp;


resp_api respApi;

int main() {
    boost::asio::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 3000));
    std::cout << "RESP server started, waiting for connection" << std::endl;
    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "Client connected" << std::endl;
        resp_connection connection(socket, [](const std::vector<std::string>& command, resp_connection& con) {
            respApi.processCommand(command, con);
        });
        connection.run();
        std::cout << "Client disconnected" << std::endl;
    }
    return 0;
}

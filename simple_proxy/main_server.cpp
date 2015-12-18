//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include <sys/fcntl.h>
#include "main_server.hpp"

main_server::main_server(int port) : port(port) {
    server_socket = socket(AF_INET, SOCK_STREAM, NULL);

    int flags;
    if (-1 == (flags = fcntl(server_socket, F_GETFL, 0))) {
        flags = 0;
    }
    if (fcntl(server_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::exception();
    }

    sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;

    int bnd = bind(server_socket, (struct sockaddr*)&server, sizeof(server));
    if (bnd == -1) {
        throw std::exception();
    }
    if (listen(server_socket, SOMAXCONN) == -1) {
        throw std::exception();
    }
}
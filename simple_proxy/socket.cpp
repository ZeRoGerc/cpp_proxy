//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include "socket.hpp"

#include <iostream>
#include <sys/socket.h>

socket::socket(int descriptor) {
    sockaddr client_addr;
    socklen_t client_size = sizeof(sockaddr);
    client_socket = accept(descriptor, &client_addr, &client_size);

    if (client_socket == -1) {
        throw new std::exception();
    }
    
    int flags;
    if (-1 == (flags = fcntl(client_socket, F_GETFL, 0))) {
        flags = 0;
    }
    if (fcntl(client_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << std::strerror(errno);
        throw std::exception();
    }
}

socket::socket(std::string const& ip, size_t port) {
    client_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        throw new std::exception();
    }

    int flags;
    if (-1 == (flags = fcntl(client_socket, F_GETFL, 0))) {
        flags = 0;
    }
    if (fcntl(client_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::exception();
    }

    const int set = 1;
    setsockopt(client_socket, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));

    sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(ip.c_str());
    serv.sin_port = htons(port);

    connect(client_socket, (struct sockaddr*)&serv, sizeof(serv));
}

socket::~socket() {
    close(client_socket);
}
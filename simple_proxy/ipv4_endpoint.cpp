//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include "ipv4_endpoint.hpp"

#include <iostream>
#include <sys/socket.h>

ipv4_endpoint::ipv4_endpoint(int descriptor) {
    sockaddr client_addr;
    socklen_t client_size = sizeof(sockaddr);
    client_socket = accept(descriptor, (sockaddr*) &client_addr, &client_size);

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

ipv4_endpoint::ipv4_endpoint(std::string ip, size_t port) {
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
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

ipv4_endpoint::~ipv4_endpoint() {
    close(client_socket);
}

void ipv4_endpoint::send(char* buffer) {
    ::send(get_socket(), buffer, strlen(buffer), 0); // Only in debug
}
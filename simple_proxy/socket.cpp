//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include "socket.hpp"
#include "custom_exception.hpp"

#include <iostream>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <cmath>
#include <string>

void set_socket_properties(int client_socket) {
    if (client_socket == -1) {
        std::string message{"fail to create socket: "};
        message.append(std::strerror(errno));
        throw custom_exception(message);
    }
    
    int flags;
    if (-1 == (flags = fcntl(client_socket, F_GETFL, 0))) {
        flags = 0;
    }
    if (fcntl(client_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::string message{"fail to create socket: "};
        message.append(std::strerror(errno));
        throw custom_exception(message);
    }
    
    const int set = 1;
    setsockopt(client_socket, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set));
}

socket::socket(int descriptor) {
    sockaddr client_addr;
    socklen_t client_size = sizeof(sockaddr);
    client_socket = accept(descriptor, &client_addr, &client_size);
    set_socket_properties(client_socket);
}

socket::socket(std::string const& ip, size_t port) {
    client_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    set_socket_properties(client_socket);
    
    sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(ip.c_str());
    serv.sin_port = htons(port);

    connect(client_socket, (struct sockaddr*)&serv, sizeof(serv));
}

socket::~socket() {
    close(client_socket);
}
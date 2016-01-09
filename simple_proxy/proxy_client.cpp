//
// Created by Vladislav Sazanovich on 03.01.16.
//

#include "proxy_client.h"
#include "http_parse.hpp"

proxy_client::proxy_client(std::string const& ip, std::string const& host, size_t port)
        : client_socket(ip, port), host(host) {}


proxy_client::proxy_client(int descriptor)
        : client_socket(descriptor) {}

size_t proxy_client::send(std::string const& request) {
    if (request.size() == 0) return 0;  
    ssize_t len = ::send(get_socket(), request.c_str(), request.size(), 0);

    if (len == -1) len = 0;
    return static_cast<size_t>(len);
}

std::string proxy_client::read(size_t len) {
    char* buffer = new char[len];
    ssize_t new_len = ::recv(get_socket(), buffer, len, 0);
    if (new_len == -1) {
        delete [] buffer;
        std::cout << errno << ' ' << std::strerror(errno) << std::endl;
        throw std::exception();
    }
    
    std::string result = std::string(buffer, static_cast<unsigned long long>(new_len));
    delete [] buffer;
    return result.substr(0, new_len);
}
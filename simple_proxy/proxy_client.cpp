//
// Created by Vladislav Sazanovich on 03.01.16.
//

#include "proxy_client.h"

proxy_client::proxy_client(std::string const& ip, std::string const& host, size_t port)
: client_socket(ip, port), host(host) {std::cout << host << std::endl;}


proxy_client::proxy_client(int descriptor)
        : client_socket(descriptor) {}

size_t proxy_client::send(std::string const& request) {
    if (request.size() == 0) return 0;  
    ssize_t len = ::send(get_socket(), request.c_str(), request.size(), 0);

    if (len == -1) len = 0;
    return static_cast<size_t>(len);
}

std::string proxy_client::read(size_t len) {
    std::vector<int> buf(len);
    ssize_t new_len = ::recv(get_socket(), buf.data(), len, 0);
    if (new_len == -1) {
        return "";
    }
    
    return std::string(buffer.begin(), buffer().begin() + new_len);
}

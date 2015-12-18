//
// Created by Vladislav Sazanovich on 06.12.15.
//
#include "ipv4_endpoint.hpp"
#include "tcp_connection.hpp"
#include "tcp_server.hpp"

tcp_server::tcp_server(tcp_connection* connection, std::string host, size_t port)
: connection(connection), end_point(http_parse::get_ip_by_host(host, port), port), host(host) {
}

size_t tcp_server::send(std::string request) {
    size_t need = request.size();
    size_t len = ::send(get_socket(), request.c_str(), request.size(), 0);
    std::cout << "$$$$$$$$ " << need - len << '\n';
    return len;
}

std::string tcp_server::read(size_t len) {
    char* buffer = new char[len];
    ssize_t new_len = ::recv(get_socket(), buffer, len, 0);
    if (new_len == -1 && errno != 0x23) {
        std::cout << std::strerror(errno);
        throw std::exception();
    }
    
    std::string result = std::string(buffer, new_len);
    std::cerr << " $_$ " <<  len << " " << new_len <<  " " << result.size() << "\n";
    delete [] buffer;
    return result.substr(0, new_len);
}

void tcp_server::disconnect() {
    connection->server = nullptr;
    delete this;
}

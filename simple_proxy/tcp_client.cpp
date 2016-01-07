//
// Created by Vladislav Sazanovich on 06.12.15.
//
#include "ipv4_endpoint.hpp"
#include "tcp_connection.hpp"
#include "tcp_client.hpp"

tcp_client::tcp_client(int descriptor) : end_point(descriptor) {
}

size_t tcp_client::send(std::string responce) {
    size_t need = responce.size();
    size_t len = ::send(get_socket(), responce.c_str(), responce.size(), 0);
    std::cout << "$$$$$$$$" << need - len << '\n';
    return len;
}

std::string tcp_client::read(size_t len) {
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

//
//  tcp_server.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 28.11.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef tcp_server_hpp
#define tcp_server_hpp

#include <stdio.h>

struct tcp_connection;
struct ipv4_endpoint;

struct tcp_server {
public:
    tcp_server(tcp_connection* connection, std::string host, size_t port = 80);
    
    int get_socket() {
        return end_point.get_socket();
    }

    size_t send(std::string request);

    std::string read(size_t len);

    void disconnect();
    
    std::string const& get_host() {
        return host;
    }

private:
    tcp_connection* connection;
    ipv4_endpoint end_point;
    std::string host;
};

#endif /* tcp_server_hpp */

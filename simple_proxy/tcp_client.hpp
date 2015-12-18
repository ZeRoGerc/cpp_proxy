//
//  tcp_client.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 28.11.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef tcp_client_hpp
#define tcp_client_hpp

#include <stdio.h>
#include <string>

struct tcp_connection;
struct ipv4_endpoint;

struct tcp_client {
public:
    tcp_client(tcp_connection* connection, int descriptor);
    
    int get_socket() {
        return this->end_point.get_socket();
    }

    size_t send(std::string responce);

    std::string read(size_t len);

    void disconnect();
    
private:
    tcp_connection* connection;
    ipv4_endpoint end_point;
};

#endif /* tcp_client_hpp */

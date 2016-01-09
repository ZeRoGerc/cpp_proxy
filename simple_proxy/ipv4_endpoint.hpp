//
//  client.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 28.11.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef ipv4_endpoint_hpp
#define ipv4_endpoint_hpp

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <cmath>
#include <string>

struct ipv4_endpoint {
public:
    
    ipv4_endpoint(ipv4_endpoint const& other) = delete;
    ipv4_endpoint& operator=(ipv4_endpoint const& other) = delete;

    ipv4_endpoint(int descriptor);
    
    ipv4_endpoint(std::string const& ip, size_t port);

    ~ipv4_endpoint();

    void send(char* buffer);
    
    size_t get_socket() const{
        return client_socket;
    }
    
private:
    int client_socket;
};

#endif /* client_hpp */

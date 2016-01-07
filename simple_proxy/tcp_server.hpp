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
    tcp_server(std::string host, size_t port = 80);
    
    int get_socket() {
        return end_point.get_socket();
    }

    size_t send(std::string request);

    std::string read(size_t len);

    std::string const& get_host() {
        return host;
    }

    void set_read_event(event_registration&& event) {
        event_read = std::move(event);
    }

    void set_write_event(event_registration&& event) {
        event_write = std::move(event);
    }

private:
    ipv4_endpoint end_point;
    std::string host;

    event_registration event_read;
    event_registration event_write;
};

#endif /* tcp_server_hpp */

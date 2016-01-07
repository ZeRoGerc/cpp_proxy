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
    tcp_client(int descriptor);
    
    int get_socket() {
        return this->end_point.get_socket();
    }

    size_t send(std::string responce);

    std::string read(size_t len);

    void set_read_event(event_registration&& event) {
        event_read = std::move(event);
    }

    void set_write_event(event_registration&& event) {
        event_write = std::move(event);
    }

private:
    ipv4_endpoint end_point;

    event_registration event_read;
    event_registration event_write;
};

#endif /* tcp_client_hpp */

//
//  http_parser.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 05.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef http_parser_hpp
#define http_parser_hpp

#include <stdio.h>
#include <sstream>

struct http_parse {
public:
    std::string static get_ip_by_host(std::string const& host, size_t port) {
        struct addrinfo hints, *res, *res0;
        int error;
        int s;
        
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = PF_INET;
        hints.ai_socktype = SOCK_STREAM;
        
        char* char_port = new char[15];
        snprintf(char_port, 15, "%d", static_cast<int>(port));
        
        error = getaddrinfo(host.c_str(), char_port, &hints, &res0);
        delete char_port;
        
        if (error) {
            throw std::exception();
        }
        
        s = -1;
        for (res = res0; res; res = res->ai_next) {
            s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (s < 0) {
                continue;
            }
            
            if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
                close(s);
                s = -1;
                continue;
            }
            
            break;  /* okay we got one */
        }
        if (s < 0) {
            throw std::exception();
        }
        
        if (res->ai_family == AF_INET) {
            struct sockaddr_in  *sockaddr_ipv4;
            sockaddr_ipv4 = (struct sockaddr_in *) res->ai_addr;
            std::string ip = std::string(inet_ntoa(sockaddr_ipv4->sin_addr));
            freeaddrinfo(res0);
            return ip;
        } else {
            throw std::exception();
        }
    }
};

#endif /* http_parser_hpp */

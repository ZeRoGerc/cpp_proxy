//
//  main_server.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 28.11.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef main_server_hpp
#define main_server_hpp

#include <stdio.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <exception>

struct main_server {
public:
    main_server(int port);
    
    main_server& operator=(main_server const&) = delete;
    main_server(main_server const&) = delete;
    
    ~main_server() {
        close(server_socket);
    }
    
    int get_socket() {
        return server_socket;
    }
private:
    int server_socket;
    int port;
};

#endif /* main_server_hpp */

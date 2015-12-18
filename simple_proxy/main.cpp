//
//  main.cpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 28.11.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#include <iostream>
#include "ipv4_endpoint.hpp"
#include "main_server.hpp"
#include "ipv4_endpoint.hpp"
#include "tcp_connection.hpp"
#include "event_queue.hpp"
#include "proxy.hpp"

int main(int argc, const char * argv[]) {
    main_server server = main_server(2538);
    event_queue kq = event_queue(server.get_socket());
    proxy proxy_server = proxy(&kq, server.get_socket());
    
    proxy_server.main_loop();
}

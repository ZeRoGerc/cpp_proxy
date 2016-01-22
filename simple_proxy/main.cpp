//
//  main.cpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 28.11.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#include <iostream>
#include <thread>
#include <signal.h>

#include "socket.hpp"
#include "tcp_connection.hpp"
#include "event_queue.hpp"
#include "proxy.hpp"

int main(int argc, const char * argv[]) {
    event_queue kq;
    proxy proxy_server{&kq};

    proxy_server.main_loop();
}

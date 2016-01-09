//
//  proxy.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 28.11.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef proxy_hpp
#define proxy_hpp

#include <stdio.h>
#include <utility>
#include "event_queue.hpp"
#include "tcp_connection.hpp"
#include "event_registration.h"

struct tcp_connection;

struct proxy {
public:
    proxy(event_queue* queue, int descriptor);
    proxy(const proxy&) = delete;
    void main_loop();


    tasks_poll poll;
private:
    event_queue* queue;
    int descriptor;
    event_registration reg;
    
    std::set<tcp_connection*> deleted;
};

#endif /* proxy_hpp */

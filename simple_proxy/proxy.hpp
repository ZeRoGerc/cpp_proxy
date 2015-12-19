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
#include "event_queue.hpp"
#include "tcp_connection.hpp"

struct proxy {
public:
    proxy(event_queue* queue, int descriptor);
    proxy(const proxy&) = delete;
    void main_loop();
    
    tasks_pull _pull;
private:
    handler connect_handler;
    event_queue* queue;
    int descriptor;
};

#endif /* proxy_hpp */

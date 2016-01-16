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
#include <vector>
#include <utility>
#include "event_queue.hpp"
#include "tcp_connection.hpp"
#include "event_registration.h"
#include "lru_cache.hpp"

struct tcp_connection;

struct proxy {
public:
    proxy(event_queue* queue, int descriptor);
    
    proxy(proxy const&) = delete;
    proxy& operator=(proxy const&) = delete;
    
    proxy(proxy&&) = delete;
    proxy& operator=(proxy&&) = delete;
    
    void main_loop();
    
private:
    event_queue* queue;
    int descriptor;
    
    event_registration reg;
    
    std::set<std::unique_ptr<tcp_connection>> connections;
    std::vector<decltype(connections.begin())> deleted;
    
    lru_cache<std::string, std::string> responce_cache;
};

#endif /* proxy_hpp */

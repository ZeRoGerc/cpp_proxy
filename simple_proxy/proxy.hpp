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

    proxy(proxy const&) = delete;
    proxy& operator=(proxy const&) = delete;

    void main_loop();

    inline void add_background_task(task t) {
        poll.push(t);
    }

    inline task get_background_task() {
        return poll.pop();
    }

private:
    event_queue* queue;
    int descriptor;

    event_registration reg;

    tasks_poll poll;
    std::set<tcp_connection*> deleted;
};

#endif /* proxy_hpp */

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

    handler connect_handle = [&](struct kevent& event) {
        tcp_connection* conn = new tcp_connection(queue, descriptor);
        
        conn->set_client_handler([conn](struct kevent& event) {
            conn->handle_client_read(event);
        });

        queue->add_event(conn->get_client_socket(), EVFILT_READ, conn->get_client_handler());
    };

    void main_loop();
    
private:
    event_queue* queue;
    int descriptor;
};

#endif /* proxy_hpp */

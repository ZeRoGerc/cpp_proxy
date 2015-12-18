//
//  event_queue.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 28.11.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef event_queue_hpp
#define event_queue_hpp

#include <stdio.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <sys/socket.h>
#include <functional>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

typedef std::function<void(struct kevent&)> handler;

int get_id(struct kevent event);

struct event_queue {
public:
    event_queue(int main_socket);

    int delete_event(size_t ident, uint16_t filter);

    void add_event(int sock, uint16_t filter, handler* hand);

    void add_signal_listener(int signal_id);

    int occured();

    void execute();
    
private:
//    static const int MAX_CONN = 1024;
    struct kevent temp_event;
    struct kevent evlist[SOMAXCONN];
    std::vector<bool> is_used = std::vector<bool>(SOMAXCONN);
    int kq;
    int main_socket;
};

#endif /* event_queue_hpp */

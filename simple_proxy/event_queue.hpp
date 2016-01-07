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
#include <set>
#include "tasks_poll.hpp"

typedef std::function<void(struct kevent&)> handler;

struct event_queue {
public:
    static const int USER_EVENT_ID = 1242323;
    
    event_queue(int main_socket);

    void delete_event(size_t ident, int16_t filter);

    void add_event(size_t ident, int16_t filter, handler* hand);
    
    void execute_in_main(task t) {
        main_thread_tasks.push_back(t);
    }
    
    int occurred();

    void execute();
    
private:
//    static const int MAX_CONN = 1024;
    struct kevent evlist[SOMAXCONN];
    std::vector<bool> is_used = std::vector<bool>(SOMAXCONN);
    int kq;
    int main_socket;
    int pipe_in;
    int pipe_out;

    std::mutex mutex;
    std::set<size_t> deleted_events;
    
    event_registration main_events_registrator;
    std::vector<task> main_thread_tasks;

    void event(size_t ident, int16_t filter, uint16_t flags, uint32_t fflags, int64_t data, handler* hand);
};

#endif /* event_queue_hpp */

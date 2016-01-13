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
#include <map>
#include <array>
#include "tasks_poll.hpp"

typedef std::function<void(struct kevent&)> handler;

struct event_queue {
public:
    static const int USER_EVENT_ID = 1242323;
    
    event_queue(int main_socket);

    void delete_event(size_t ident, int16_t filter);

    void add_event(size_t ident, int16_t filter, handler hand);
    
    void execute_in_main(task t);
    
    int occurred();

    void execute(int amount);
    
private:
    struct kevent evlist[1024];
    std::vector<bool> is_used = std::vector<bool>(SOMAXCONN);
    int kq;
    int main_socket;
    int pipe_in;
    int pipe_out;

    std::mutex mutex;
    std::set< std::pair<size_t, int16_t> > deleted_events;
    
    using map_type = std::map<size_t, handler>;
    std::array<map_type, 2> handlers = { {map_type{}, map_type{}} };
    
    handler main_thread_events_handler;
    std::vector<task> main_thread_tasks;

    void event(size_t ident, int16_t filter, uint16_t flags, uint32_t fflags, int64_t data, handler hand);
    
    inline size_t event_type(int16_t filter) const {
        switch (filter) {
            case EVFILT_READ:
                return 0;
            case EVFILT_WRITE:
                return 1;
            default:
                throw std::exception();
        }
    }
};

#endif /* event_queue_hpp */

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
#include <thread>
#include <queue>

using handler = std::function<void(struct kevent&)>;
using task = std::function<void()>;

struct background_tasks_handler {
    background_tasks_handler(background_tasks_handler const&) = delete;
    background_tasks_handler& operator=(background_tasks_handler const&) = delete;
    
    background_tasks_handler(background_tasks_handler&&) = default;
    background_tasks_handler& operator=(background_tasks_handler&&) = default;

    background_tasks_handler();
    ~background_tasks_handler();
    
    void push(task t);
    
    void stop();
    
private:
    static const size_t THREADS_AMOUNT = 4;
    
    void execute();
    
    std::queue<task> poll;
    std::condition_variable condition;
    std::mutex mutex;
    std::atomic<bool> work;
    std::vector<std::thread> threads;
};

struct event_queue {
public:
    event_queue();
    
    ~event_queue();

    void delete_event(size_t ident, int16_t filter);

    void add_event(size_t ident, int16_t filter, handler* hand);
    
    void event(size_t ident, int16_t filter, uint16_t flags, uint32_t fflags, int64_t data, handler* hand);
    
    void execute_in_main(task t);
    
    void execute_in_background(task t);
    
    int occurred();

    void execute(int amount);
    
    void stop_resolve();
private:
    struct kevent evlist[1024];
    int kq;
    int pipe_in;
    int pipe_out;

    std::mutex mutex;
    std::set< std::pair<size_t, int16_t> > deleted_events;
    
    handler main_thread_events_handler;
    std::vector<task> main_thread_tasks;
    background_tasks_handler background_tasks;
    
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

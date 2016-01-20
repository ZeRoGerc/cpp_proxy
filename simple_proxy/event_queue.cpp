//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include <sys/socket.h>
#include <assert.h>
#include "event_queue.hpp"

event_queue::event_queue(int main_socket) : main_socket(main_socket) {
    kq = kqueue();
    int fds[2];
    if (pipe(fds) == -1) {
        throw std::exception();
    }
    pipe_in = fds[1];
    pipe_out = fds[0];
    
    main_thread_events_handler = handler {
        [this](struct kevent& event) {
            assert(main_thread_tasks.size() != 0);
            char* buffer = new char(1);
            read(pipe_out, buffer, 1);
            delete [] buffer;
            if (main_thread_tasks.size() != 0)
                main_thread_tasks.back()();
                main_thread_tasks.pop_back();
        }
    };
    
    add_event(pipe_out, EVFILT_READ, &main_thread_events_handler);
}

void event_queue::delete_event(size_t ident, int16_t filter) {
    
    if (deleted_events.find(std::make_pair(ident, filter)) == deleted_events.end()) {
        std::cout << "delete " << ident << ' ' << filter << std::endl;
        event(ident, filter, EV_DELETE, NULL, NULL, nullptr);
    } else {
        std::cout << "already deleted " << ident << ' ' << filter << std::endl;
    }
}

void event_queue::add_event(size_t ident, int16_t filter, handler* hand) {
    std::cout << "add " << ident << ' ' << filter << std::endl;
    event(ident, filter, EV_ADD, NULL, NULL, hand);
}

void event_queue::execute_in_main(task t) {
    std::lock_guard<std::mutex> locker{mutex};
    main_thread_tasks.push_back(t);
    write(pipe_in, "T", 1);
}

void event_queue::execute_in_background(task t) {
    background_tasks.push(t);
}


task event_queue::get_background_task() {
    return background_tasks.pop();
}

int event_queue::occurred() {
    return kevent(kq, NULL, 0, evlist, SOMAXCONN, NULL);
}

void event_queue::execute(int amount) {
    deleted_events.clear();
    
//    std::cerr << "AMOUNT " << amount << "\n";

    for (int i = 0; i < amount; i++) {
//        std::cerr << "EVENT " << evlist[i].filter << ' ' << evlist[i].ident << "\n";
        
        if (deleted_events.size() == 0 || deleted_events.find(std::make_pair(evlist[i].ident, evlist[i].filter)) == deleted_events.end()) {
            handler* hand = static_cast<handler*>(evlist[i].udata);
            hand->operator()(evlist[i]);
        }
    }
}

void event_queue::event(size_t ident, int16_t filter, uint16_t flags, uint32_t fflags, int64_t data, handler* hand) {
    struct kevent temp_event;
    
    EV_SET(&temp_event, ident, filter, flags, fflags, data, static_cast<void*>(hand));

    if (kevent(kq, &temp_event, 1, NULL, 0, NULL) == -1) {
        std::cout << std::strerror(errno);
        throw std::exception();
    }
    
    if (flags & EV_DELETE) {
        deleted_events.insert(std::make_pair(ident, filter));
    }
}
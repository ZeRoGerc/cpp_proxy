//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include <sys/socket.h>
#include "event_queue.hpp"

event_queue::event_queue(int main_socket) : main_socket(main_socket) {
    kq = kqueue();
    int fds[2];
    if (pipe(fds) == -1) {
        throw std::exception();
    }
    pipe_in = fds[1];
    pipe_out = fds[0];
}

void event_queue::delete_event(size_t ident, int16_t filter) {
    std::lock_guard<std::mutex> locker{mutex};

    if (deleted_events.find(ident) == deleted_events.end()) {
        event(ident, filter, EV_DELETE, NULL, NULL, nullptr);
        deleted_events.insert(ident);
    }
}

void event_queue::add_event(size_t ident, int16_t filter, handler* hand) {
    std::lock_guard<std::mutex> locker{mutex};
    event(ident, filter, EV_ADD, NULL, NULL, hand);
}

int event_queue::occurred() {
    std::lock_guard<std::mutex> locker{mutex};

    deleted_events.clear();
    return kevent(kq, NULL, 0, evlist, SOMAXCONN, NULL);
}

void event_queue::execute() {
    int amount = kevent(kq, NULL, 0, evlist, SOMAXCONN, NULL);

//    std::cerr << "AMOUNT " << amount << "\n";

    std::unique_lock<std::mutex> locker{mutex, std::defer_lock};
    
    for (int i = 0; i < amount; i++) {
//        std::cerr << "EVENT " << evlist[i].filter << "\n";

        locker.lock();
        if (deleted_events.find(evlist[i].ident) == deleted_events.end()) {
            locker.unlock();
            handler *hand = static_cast<handler *>(evlist[i].udata);
            hand->operator()(evlist[i]);
        } else {
            locker.unlock();
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
}
//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include <sys/socket.h>
#include "event_queue.hpp"

int get_id(struct kevent event) {
    return static_cast<int>(event.ident);
}

event_queue::event_queue(int main_socket) : main_socket(main_socket) {
    kq = kqueue();
}

int event_queue::delete_event(size_t ident, uint16_t filter) {
    for (int i = 0; i < SOMAXCONN; i++) {
        if (evlist[i].ident == ident) {
            is_used[i] = true;
        }
    }
    EV_SET(&temp_event, ident, filter, EV_DELETE, NULL, NULL, NULL);
    return kevent(kq, &temp_event, 1, NULL, NULL, NULL);
}

void event_queue::add_event(int sock, uint16_t filter, handler* hand) {
    EV_SET(&temp_event, sock, filter, EV_ADD, NULL, NULL, static_cast<void*>(hand));
    if (kevent(kq, &temp_event, 1, NULL, NULL, NULL) == -1) {
        std::cout << std::strerror(errno);
        throw new std::exception();
    }
}

void event_queue::add_signal_listener(int signal_id) {
    struct kevent kev;
    EV_SET(&kev, signal_id, EVFILT_USER, EV_ADD|EV_CLEAR, 0, 0, NULL);
    kevent(kq, &kev, 1, NULL, 0, NULL);
}

int event_queue::occured() {
//        std::cerr << "HERE OCCURED\n";
    return kevent(kq, NULL, 0, evlist, SOMAXCONN, NULL);
}

void event_queue::execute() {
    int amount = kevent(kq, NULL, 0, evlist, SOMAXCONN, NULL);

    for (int i = 0; i < SOMAXCONN; i++) {
        is_used[i] = false;
    }

    for (int i = 0; i < amount; i++) {
        if (is_used[i]) continue;

//            std::cerr << "filter " << evlist[i].filter << '\n';
        handler* hand = static_cast<handler*>(evlist[i].udata);
        hand->operator()(evlist[i]);
    }
}
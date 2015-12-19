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
    struct kevent temp_event;
    EV_SET(&temp_event, ident, filter, EV_DELETE, NULL, NULL, NULL);
    return kevent(kq, &temp_event, 1, NULL, NULL, NULL);
}

void event_queue::add_event(int sock, uint16_t filter, handler* hand) {
    struct kevent temp_event;
    
    EV_SET(&temp_event, sock, filter, EV_ADD, NULL, NULL, static_cast<void*>(hand));
    
    if (kevent(kq, &temp_event, 1, NULL, NULL, NULL) == -1) {
        std::cout << std::strerror(errno);
        //TODO can't throw exceptions from background thread
        throw std::exception();
    }
}

void event_queue::trigger_user_event(handler hand) {
    struct kevent temp_event;
    
    user_event_handler = hand;
    EV_SET(&temp_event, USER_EVENT_ID, EVFILT_USER, EV_ADD|EV_CLEAR, 0, 0, NULL);
    if (kevent(kq, &temp_event, 1, NULL, 0, NULL) == -1) {
        std::cout << std::strerror(errno);
        //TODO can't throw exceptions from background thread
        throw std::exception();
    }
    
    std::cerr << "TRIGGER ADDED\n";
    struct kevent kev;
    
    EV_SET(&kev, USER_EVENT_ID, EVFILT_USER, 0, NOTE_TRIGGER, 0, static_cast<void*>(&user_event_handler));
    if (kevent(kq, &kev, 1, NULL, 0, NULL) == -1) {
        std::cerr << std::strerror(errno);
        throw std::exception();
    }
}

void event_queue::delete_trigger_event() {
    struct kevent temp_event;
    EV_SET(&temp_event, USER_EVENT_ID, EVFILT_USER, EV_DELETE, 0, 0, NULL);
    if (kevent(kq, &temp_event, 1, NULL, 0, NULL) == -1) {
        std::cout << std::strerror(errno);
        //TODO can't throw exceptions from background thread
        throw std::exception();
    }
    
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

    std::cerr << "AMOUNT " << amount << "\n";
    for (int i = 0; i < amount; i++) {
        std::cerr << "EVENT " << evlist[i].filter << "\n";
        if (is_used[i]) continue;

//            std::cerr << "filter " << evlist[i].filter << '\n';
        handler* hand = static_cast<handler*>(evlist[i].udata);
        hand->operator()(evlist[i]);
//        if (evlist[i].filter == EVFILT_USER) {
//            std::cerr << "sad(\n";
//            delete hand;
//        }
    }
}
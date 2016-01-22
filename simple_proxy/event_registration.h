//
// Created by Vladislav Sazanovich on 07.01.16.
//

#ifndef SIMPLE_PROXY_EVENT_REGISTRATION_H
#define SIMPLE_PROXY_EVENT_REGISTRATION_H

#include "event_queue.hpp"

struct event_registration
{
    event_registration();

    event_registration(event_queue* queue, int ident, int16_t filter, handler h, bool listen=false);
    
    event_registration(event_queue* queue, int ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, handler h, bool listen=false);

    event_registration(event_registration const&) = delete;
    event_registration& operator=(event_registration const&) = delete;

    event_registration(event_registration&& other);
    event_registration& operator=(event_registration&& other);

    ~event_registration() {
        stop_listen();
    }

    void stop_listen() {
        if (is_listened && is_valid()) {
            queue->event(static_cast<size_t>(ident), filter, EV_DELETE | flags, fflags, data, &handler_);
            is_listened = false;
        }
    }

    void resume_listen() {
        if (!is_listened && is_valid()) {
            queue->event(static_cast<size_t>(ident), filter, EV_ADD | flags, fflags, data, &handler_);
            is_listened = true;
        }
    }
    
    void refresh() {
        stop_listen();
        resume_listen();
    }

    void change_function(handler&& hand) {
        handler_ = std::move(hand);
        bool temp = is_listened;
        stop_listen();
        if (temp && is_valid())
            resume_listen();
    }
    
    inline bool is_valid() const {
        return ident != -1;
    }
    
    void invalidate() {
        stop_listen();
        ident = -1;
    }

private:
    event_queue* queue;
    handler handler_;
    int ident = -1;
    int16_t filter = NULL;
    uint16_t flags = NULL;
    uint32_t fflags = NULL;
    intptr_t data = NULL;

    bool is_listened = false;
};



#endif //SIMPLE_PROXY_EVENT_REGISTRATION_H

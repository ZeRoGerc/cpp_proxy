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

    inline void stop_listen() {
        if (is_listened && is_valid()) {
            queue->event(static_cast<size_t>(ident), filter, EV_DELETE | flags, fflags, data, &handler_);
            is_listened = false;
        }
    }

    inline void resume_listen() {
        if (!is_listened && is_valid()) {
            queue->event(static_cast<size_t>(ident), filter, EV_ADD | flags, fflags, data, &handler_);
            is_listened = true;
        }
    }
    
    inline void refresh() {
        stop_listen();
        resume_listen();
    }

    void change_function(handler&& hand);
    
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
    int16_t filter = 0;
    uint16_t flags = 0;
    uint32_t fflags = 0;
    intptr_t data = 0;

    bool is_listened = false;
};



#endif //SIMPLE_PROXY_EVENT_REGISTRATION_H

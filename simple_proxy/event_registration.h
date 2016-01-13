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

    event_registration(event_registration const&) = delete;
    event_registration& operator=(event_registration const&) = delete;

    event_registration(event_registration&& other);
    event_registration& operator=(event_registration&& other);

    ~event_registration() {
        stop_listen();
    }

    void stop_listen() {
        if (is_listened && is_valid()) {
            queue->delete_event(static_cast<size_t>(ident), filter);
            is_listened = false;
        }
    }

    void resume_listen() {
        if (!is_listened && is_valid()) {
            queue->add_event(static_cast<size_t>(ident), filter, handler_);
            is_listened = true;
        }
    }

    void change_function(handler&& hand) {
        handler_ = std::move(hand);
        if (is_listened && is_valid())
            queue->add_event(static_cast<size_t>(ident), filter, handler_);
    }
    
    inline bool is_valid() const {
        return ident != -1;
    }

private:
    event_queue* queue;
    handler handler_;
    int ident = -1;
    int16_t filter;

    bool is_listened = false;
};



#endif //SIMPLE_PROXY_EVENT_REGISTRATION_H

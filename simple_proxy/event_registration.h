//
// Created by Vladislav Sazanovich on 03.01.16.
//

#ifndef SIMPLE_PROXY_EVENT_REGISTRATION_H
#define SIMPLE_PROXY_EVENT_REGISTRATION_H


#include "event_queue.hpp"

struct event_registration
{
    event_registration();

    event_registration(event_queue* queue, size_t ident, int16_t filter, handler h, bool listen=true);

    event_registration(event_registration const&) = delete;
    event_registration& operator=(event_registration const&) = delete;

    event_registration(event_registration&& other)
    {
        bool previous = other.is_listened;
        
        queue = other.queue;
        ident = other.ident;
        filter = other.filter;
        handler_ = std::move(other.handler_);
        is_listened = other.is_listened;
        
        other.is_listened = false;
        
        if (previous) {
            queue->add_event(ident, filter, &handler_);
        }
    }

    event_registration& operator=(event_registration&& other)
    {
        // mustn't listen events while move
        stop_listen();
        bool previous = other.is_listened;
        other.stop_listen();
        
        queue = other.queue;
        ident = other.ident;
        filter = other.filter;
        handler_ = std::move(other.handler_);
        is_listened = other.is_listened;
        
        if (previous) {
            queue->add_event(ident, filter, &handler_);
        }
        
        return *this;
    }

    ~event_registration() {
        stop_listen();
    }

    void stop_listen() {
        if (is_listened && ident != -1) {
            queue->delete_event(ident, filter);
            is_listened = false;
        }
    }

    void resume_listen() {
        if (!is_listened && ident != -1) {
            queue->add_event(ident, filter, &handler_);
            is_listened = true;
        }
    }

    void change_function(handler hand) {
        handler_ = std::move(hand);
        if (is_listened)
            queue->add_event(ident, filter, &handler_);
    }

private:
    event_queue* queue;
    handler handler_;
    size_t ident = -1;
    int16_t filter;

    bool is_listened=false;
};


#endif //SIMPLE_PROXY_EVENT_REGISTRATION_H

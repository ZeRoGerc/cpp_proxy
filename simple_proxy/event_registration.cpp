//
// Created by Vladislav Sazanovich on 07.01.16.
//

#include "event_registration.h"

event_registration::event_registration() {};

event_registration::event_registration(event_queue* queue, int ident, int16_t filter, handler h, bool listen)
        : queue(queue), ident(ident), filter(filter), handler_(std::move(h)), is_listened(listen)
{
    if (is_listened) {
        queue->add_event(static_cast<size_t>(ident), filter, &handler_);
    }
}

event_registration::event_registration(event_registration&& other)
{
    bool previous = other.is_listened;
    
    queue = other.queue;
    ident = other.ident;
    filter = other.filter;
    handler_ = std::move(other.handler_);
    is_listened = other.is_listened;
    
    other.is_listened = false;
    
    if (previous) {
        queue->add_event(static_cast<size_t>(ident), filter, &handler_);
    }
}

event_registration& event_registration::operator=(event_registration&& other)
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
        queue->add_event(static_cast<size_t>(ident), filter, &handler_);
    }
    
    return *this;
}
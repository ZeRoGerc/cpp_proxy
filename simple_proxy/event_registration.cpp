//
// Created by Vladislav Sazanovich on 07.01.16.
//

#include "event_registration.h"
#include <assert.h>

event_registration::event_registration() {};

event_registration::event_registration(event_queue* queue, int ident, int16_t filter, handler h, bool listen)
    : queue(queue), ident(ident), filter(filter), handler_(std::move(h)), is_listened(listen)
{
    if (is_listened) {
        queue->event(static_cast<size_t>(ident), filter, EV_ADD | flags, fflags , data, &handler_);
    }
}

event_registration::event_registration(event_queue* queue, int ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, handler h, bool listen)
    : queue(queue), ident(ident), filter(filter), flags(flags), fflags(fflags), data(data), handler_(std::move(h)), is_listened(listen)
{
    if (is_listened) {
        queue->event(static_cast<size_t>(ident), filter, EV_ADD | flags, fflags , data, &handler_);
    }
}

event_registration::event_registration(event_registration&& other)
{
    queue = other.queue;
    ident = other.ident;
    filter = other.filter;
    flags = other.flags;
    fflags = other.fflags;
    data = other.data;
    handler_ = std::move(other.handler_);
    is_listened = false;
    
    // make other invalid
    other.ident = -1;
    
    assert(ident != -1);
    
    if (other.is_listened) {
        resume_listen();
    }
}

event_registration& event_registration::operator=(event_registration&& other)
{
    // unsubscribe from previous event
    stop_listen();
    
    queue = other.queue;
    ident = other.ident;
    filter = other.filter;
    flags = other.flags;
    fflags = other.fflags;
    data = other.data;
    handler_ = std::move(other.handler_);
    is_listened = false;
    
    //make other invalid
    other.ident = -1;
    
    assert(ident != -1);
    
    if (other.is_listened) {
        resume_listen();
    }
    
    return *this;
}
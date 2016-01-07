//
// Created by Vladislav Sazanovich on 03.01.16.
//

#include "event_registration.h"

event_registration::event_registration() {};

event_registration::event_registration(event_queue* queue, size_t ident, int16_t filter, handler h, bool listen)
        : queue(queue), ident(ident), filter(filter), handler_(std::move(h)), is_listened(listen)
{
    queue->add_event(ident, filter, &handler_);
}
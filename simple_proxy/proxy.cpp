//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include "event_queue.hpp"
#include "proxy.hpp"

proxy::proxy(event_queue* queue, int descriptor) : queue(queue), descriptor(descriptor) {
    queue->add_event(descriptor, EVFILT_READ, &connect_handle);
}

void proxy::main_loop() {
    while (true) {
        if (queue->occured() > 0) {
            queue->execute();
        }
    }
}
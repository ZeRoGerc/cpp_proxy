//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include "event_queue.hpp"
#include "proxy.hpp"
#include "listener.hpp"

proxy::proxy(event_queue* queue, int descriptor)
    : queue(queue)
    , descriptor(descriptor)
    , reg(queue, descriptor, EVFILT_READ, [this, queue, descriptor](struct kevent& event) {
        tcp_connection* conn = new tcp_connection(queue, descriptor);
    
        resolver callback = [this](task t) {
            std::cerr << "PUSH\n";
            this->poll.push(t);
        };
    
        conn->set_callback(callback);
    
        conn->start();
    })
{}

void proxy::main_loop() {
    while (true) {
        if (queue->occurred() > 0) {
            queue->execute();
        }
    }
}
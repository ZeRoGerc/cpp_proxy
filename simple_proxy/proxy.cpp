//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include "event_queue.hpp"
#include "proxy.hpp"
#include "listener.hpp"

proxy::proxy(event_queue* queue, int descriptor) : queue(queue), descriptor(descriptor) {
    connect_handler = [&](struct kevent& event) {
        tcp_connection* conn = new tcp_connection(this->queue, this->descriptor);
        
        resolver callback = [&](task _task) {
            std::cerr << "PUSH\n";
            _pull.push(_task);
        };
        
        conn->set_callback(callback);
        
        conn->set_client_handler([conn](struct kevent& event) {
            conn->handle_client_read(event);
        });
        
        this->queue->add_event(conn->get_client_socket(), EVFILT_READ, conn->get_client_handler());
    };
    
    queue->add_event(descriptor, EVFILT_READ, &connect_handler);
}

void proxy::main_loop() {
    while (true) {
        if (queue->occured() > 0) {
            queue->execute();
        }
    }
}
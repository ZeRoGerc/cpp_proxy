//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include "event_queue.hpp"
#include "proxy.hpp"
#include "listener.hpp"

proxy::proxy(event_queue* queue, int descriptor)
: queue(queue)
, descriptor(descriptor)
, reg(
      queue,
      descriptor,
      EVFILT_READ,
      [this, queue, descriptor](struct kevent& event) {
          tcp_connection* conn = new tcp_connection(queue, descriptor);
          
          resolver callback = [this](task t) {
              add_background_task(t);
          };
          
          std::function<void()> deleter = [this, conn]() {
              deleted.push_back(conn);
          };
          
          conn->set_callback(callback);
          conn->set_deleter(deleter);
          
          connections.insert(conn);
          
          conn->start();
      },
      true
      )
{}

void proxy::main_loop() {
    while (true) {
        if (int amount = queue->occurred()) {
            if (deleted.size() > 0) {
                //delete all disconnected connections
                for (tcp_connection* connection: deleted) {
                    connections.erase(connection);
                    delete connection;
                }
                deleted.clear();
            }
            queue->execute(amount);
        }
    }
}
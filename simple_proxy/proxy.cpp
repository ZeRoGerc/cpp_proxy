//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include "event_queue.hpp"
#include "proxy.hpp"
#include "listener.hpp"
#include "tcp_connection.hpp"
#include <memory>

proxy::proxy(event_queue* queue, int descriptor)
: queue(queue)
, descriptor(descriptor)
, reg(
      queue,
      descriptor,
      EVFILT_READ,
      [this, queue, descriptor](struct kevent& event) {
//          auto conn = std::make_unique<tcp_connection>(queue, descriptor);
          
          //memory leak
          auto iter = connections.insert(std::unique_ptr<tcp_connection>(new tcp_connection(queue, &responce_cache, descriptor))).first;
          
          std::function<void()> deleter = [this, iter]() {
              deleted.push_back(iter);
          };
          
          (*iter)->set_deleter(deleter);
          (*iter)->start();
      },
      true
      )
{}

void proxy::main_loop() {
    while (true) {
        for (auto& conn_iter: deleted) {
            connections.erase(conn_iter);
        }
        deleted.clear();
        if (int amount = queue->occurred()) {
            queue->execute(amount);
        }
    }
}
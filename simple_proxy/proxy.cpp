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
          auto iter = connections.insert(std::unique_ptr<tcp_connection>(new tcp_connection(queue, &responce_cache, descriptor))).first;
          
          std::function<void()> deleter = [this, iter]() {
              deleted.push_back(iter);
          };
          
          (*iter)->set_deleter(deleter);
          (*iter)->start();
      },
      true
      )
, sigint(
         queue,
         SIGINT,
         EVFILT_SIGNAL,
         [this](struct kevent event) {
             std::cout << "SIGINT";
             hard_stop();
         },
         true
         )
{}

proxy::~proxy() {
    reg.stop_listen();
    sigint.stop_listen();
    queue->stop_resolve();
    
}

void proxy::main_loop() {
    try {
        while (work) {
            for (auto& conn_iter: deleted) {
                connections.erase(conn_iter);
            }
            deleted.clear();
            
            std::cout << "CONNECTIONS " << connections.size() << std::endl;
            if (int amount = queue->occurred()) {
                queue->execute(amount);
            }
            
            if (soft_exit && connections.size() == 0) {
                return;
            }
        }
    } catch(...) {
        hard_stop();
    }
}


void proxy::hard_stop() {
    queue->stop_resolve();
    work = false;
}


void proxy::soft_stop() {
    soft_exit = true;
    reg.stop_listen();
    
    //finish working with existing connections
    main_loop();
}
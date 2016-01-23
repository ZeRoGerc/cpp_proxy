//
// Created by Vladislav Sazanovich on 06.12.15.
//

#include "event_queue.hpp"
#include "proxy.hpp"
#include "tcp_connection.hpp"
#include <memory>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <exception>
#include <sys/fcntl.h>

main_server::main_server(int port) : port(port) {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    const int set = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &set, sizeof(set)) == -1) {
        throw custom_exception("fail to reuse port");
    };
    
    sockaddr_in server;
    
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = INADDR_ANY;
    
    int flags;
    if (-1 == (flags = fcntl(server_socket, F_GETFL, 0))) {
        flags = 0;
    }
    if (fcntl(server_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw custom_exception("fail to create connect socket");
    }
    
    int bnd = bind(server_socket, (struct sockaddr*)&server, sizeof(server));
    if (bnd == -1) {
        throw custom_exception("fail to bind connect socket");
    }
    if (listen(server_socket, SOMAXCONN) == -1) {
        throw custom_exception("fail to listen connect socket");
    }
}

proxy::proxy(event_queue* queue)
: queue(queue)
, connect_server(main_server{2532})
, reg(
      queue,
      connect_server.get_socket(),
      EVFILT_READ,
      [this, queue](struct kevent& event) {
          try {
              auto temp = std::unique_ptr<tcp_connection>(new tcp_connection(queue, &responce_cache, connect_server.get_socket()));
              
              auto iter = connections.insert(std::move(temp)).first;
              
              std::function<void()> deleter = [this, iter]() {
                  deleted.push_back(iter);
              };
              
              (*iter)->set_deleter(deleter);
              (*iter)->start();
          } catch (std::exception const& e) {
              std::cerr << e.what() << std::endl;
          }
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
}
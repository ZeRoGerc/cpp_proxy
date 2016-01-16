//
//  tcp_connection.cpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 06.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#include <stdio.h>
#include "socket.hpp"
#include "tcp_connection.hpp"

const std::string buffer::chunked_end{"\r\n0\r\n\r\n"};
const int tcp_connection::CHUNK_SIZE = 1024;
const int tcp_connection::BUFFER_SIZE = 16384;

buffer::buffer(std::string chunk, int amount_of_data) {
    available_data = amount_of_data;
    readed = 0;
    tail = std::string();
    data = std::string();
    append(chunk);
}

void buffer::append(std::string chunk) {
    assert(available_data != 0);
    
    if (available_data != -1 && chunk.size() > available_data) {
        chunk = chunk.substr(0, static_cast<unsigned>(available_data));
    }
    
    if (chunk.size() > chunked_end.size()) {
        tail = chunk.substr(chunk.size() - chunked_end.size());
    } else {
        tail += chunk;
        if (tail.size() > chunked_end.size()) {
            tail = tail.substr(tail.size() - chunked_end.size());
        }
    }
    
    if (available_data != -1) {
        available_data -= chunk.size();
    }
    data += chunk;
    
    if (tail == chunked_end && available_data == -1) {
        available_data = 0;
    }
}

std::string buffer::get(size_t amount) const {
    //amount == -1: return all data by default
    if (amount == -1 || amount >= (data.size() - readed)) {
        return data.substr(readed);
    }
    
    return data.substr(readed, readed + amount);
}

void buffer::pop_front(size_t amount) {
    assert(amount <= (data.size() - readed));
    readed += amount;
}

void buffer::clear() {
    data.clear();
    readed = 0;
    available_data = 0;
    tail.clear();
}

std::string buffer::get_all_data() {
    return data;
}

size_t buffer::size() const {
    return data.size() - readed;
}

tcp_connection::tcp_connection(event_queue* q, cache_type* cache, int descriptor)
    : queue(q), cache(cache), client(new proxy_client(descriptor)), server(nullptr) {}

void tcp_connection::start() {
    set_read_function(
                      client,
                      handler {
                          [this](struct kevent& event) {
                              handle_client_disconnect(event);
                          }
                      }
                      );
    switch_state(State::RECEIVE_CLIENT);
}

bool tcp_connection::init_server(std::string const& ip, std::string const& host, size_t port) {
    //check if new server different to previous
    if (server && !(server->get_host() == host)) {
        safe_server_disconnect();
    }

    if (!server) {
        server.reset(new proxy_client(ip, host, port));
    }
    
    set_read_function(
                      server,
                      handler {
                          [this](struct kevent& event) {
                              handle_server_disconnect(event);
                          }
                      }
                      );
    return true;
}

tcp_connection::~tcp_connection() {}

void tcp_connection::safe_server_disconnect() {
    server.reset(nullptr);
}

void tcp_connection::safe_client_disconnect() {
    client.reset(nullptr);
}

void tcp_connection::safe_disconnect() {
    if (state == State::RESOLVE) {
        //while resolving in progress connection can't die
        safe_client_disconnect();
        safe_server_disconnect();
    } else {
        switch_state(State::DELETED);
    }
}

void tcp_connection::get_client_body(struct kevent &event) {
    if (handle_client_disconnect(event) || body_buffer.amount_of_available_data() == 0)
        return;

    std::string chunk = client->read(std::min(CHUNK_SIZE, body_buffer.amount_of_available_data()));
    
    
    std::cerr << "client body send " << client->get_socket() << ' ' << chunk << std::endl;
    
    body_buffer.append(chunk);
}

void tcp_connection::get_client_header(struct kevent &event) {
    if (handle_client_disconnect(event)) {
//        std::cout << "read client disconnect";
        return;
    }

    std::string chunk = client->read(CHUNK_SIZE);

    std::cerr << "client header send " << client->get_socket() << ' '  << chunk << std::endl;
    header.append(chunk);

    if (header.get_state() == http_header::State::COMPLETE) {
        switch_state(State::RESOLVE);
        
        task resolve {
                [this]() {
                    /*
                     during resolve connection couldn't die
                     but if we need to determine if connection is in valid state after resolve
                     we could check state of client
                     if client doesn't exist it means that connection die
                     */
                    std::string host = header.retrieve_host();
                    size_t port = header.retrieve_port();
                    // since we pass data as header + body
                    size_t content_len = header.get_content_length() + header.size();

                    try {
                        std::string ip = http_parse::get_ip_by_host(host, port);
                        queue->execute_in_main(task{[this, host, ip, port, content_len](){
//                            std::cout << "MAIN TASK connection state " << int(this->state) << std::endl;
                            state = State::UNDEFINED;
                            
                            if (!client) {
                                //if state is invalid just delete
                                switch_state(State::DELETED);
                                return;
                            }
                            init_server(ip, host, port);
                            
                            //here we have valid server and valid client
                            body_buffer = buffer(header.get_string_representation(), static_cast<int>(content_len));
                            
                            switch_state(State::SEND_SERVER);
                        }});
                    } catch (std::exception const& e) {
                        //send bad request in main thread
                        queue->execute_in_main(
                                               task{[this]()
                                                   {
//                                                       std::cout << "MAIN TASK(exception) connection state " << int(this->state) << std::endl;
                                                       state = State::UNDEFINED;
                                                       safe_disconnect();
                                                   }
                                               });
                    }
                }
        };

        //execute in background
        queue->execute_in_background(resolve);
    }
}

void tcp_connection::handle_client_write(struct kevent& event) {
    if (handle_client_disconnect(event))
        return;

    
    size_t len = client->send(body_buffer.get());
    assert(len != -1);
    
    
    std::cerr << "client receive " << client->get_socket() << ' ' << body_buffer.get() << std::endl;
    
    body_buffer.pop_front(len);
    
    //if server finish sending and client receive all available data
    if (body_buffer.size() == 0 && body_buffer.amount_of_available_data() == 0) {
        client->stop_write();
        switch_state(State::RECEIVE_CLIENT); //start new request
    }
}

void tcp_connection::get_server_body(struct kevent &event) {
    if (handle_server_disconnect(event) || body_buffer.amount_of_available_data() == 0)
        return;

    
    std::string chunk = server->read(CHUNK_SIZE);
    
    std::cout << "server body send " << server->get_socket() << ' ' << chunk << std::endl;
    
    body_buffer.append(chunk);
}

void tcp_connection::get_server_header(struct kevent &event) {
    if (handle_server_disconnect(event))
        return;

    std::string chunk = server->read(CHUNK_SIZE);

    std::cout << "server header send " << server->get_socket() << ' ' << chunk << std::endl;
    header.append(chunk);

    if (header.get_state() == http_header::State::COMPLETE) {
        
        if (header.get_type() == http_header::Type::CHUNKED) {
            body_buffer = buffer(header.get_string_representation(), -1);
            
        } else {
            body_buffer = buffer(header.get_string_representation(), static_cast<int>(header.get_content_length() + header.size()));
        }

        switch_state(State::SEND_CLIENT);
    }
}

void tcp_connection::handle_server_write(struct kevent& event) {
    if (handle_server_disconnect(event))
        return;
    
    
    size_t len = server->send(body_buffer.get());
    assert(len != -1);
    
    std::cout << "server receive " << body_buffer.get(len) << std::endl;
    body_buffer.pop_front(len);
    
    //If we have already received all data from client and send it
    if (body_buffer.size() == 0 && body_buffer.amount_of_available_data() == 0) {
        server->stop_write();
        switch_state(State::RECEIVE_SERVER);
    }
}

bool tcp_connection::handle_server_disconnect(struct kevent& event) {
    if (state == State::DELETED) {
        return true;
    }
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        safe_server_disconnect();
        return true;
    }
    return false;
}

bool tcp_connection::handle_client_disconnect(struct kevent& event) {
    if (state == State::DELETED) {
        return true;
    }
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        safe_disconnect();
        return true;
    }
    return false;
}

void tcp_connection::switch_state(State new_state) {
    if (state == State::DELETED) {
        throw std::exception();
    }
    state = new_state;
    std::cout << "switch_state: ";
    switch (new_state) {
        case State::RECEIVE_CLIENT:
            std::cout << "RECEIVE_CLIENT ";
            break;
        case State::RESOLVE:
            std::cout << "RESOLVE ";
            break;
        case State::SEND_SERVER:
            std::cout << "SEND_SERVER ";
            break;
        case State::RECEIVE_SERVER:
            std::cout << "RECEIVE_SERVER ";
            break;
        case State::SEND_CLIENT:
            std::cout << "SEND_CLIENT ";
            break;
        case State::DELETED:
            std::cout << "DELETED ";
            break;
        default:
            std::cout << "UNDEFINED ";
            break;
    }
    
    if (client) std::cout << "client: " << client->get_socket() << ' ';
    if (server) std::cout << "server: " << server->get_socket() << ' ';
    std::cout << std::endl;
    
    switch (new_state) {
        case State::RECEIVE_CLIENT:
            header.clear();
            body_buffer.clear();
            
            set_read_function(
                              client,
                              handler{
                                  [this](struct kevent &event) {
                                      get_client_header(event);
                                  }
                              }
                              );
            break;
        case State::RESOLVE:
            set_read_function(
                              client,
                              handler {
                                  [this](struct kevent& event) {
                                      handle_client_disconnect(event);
                                  }
                              }
                              );
            if (server) {
                set_read_function(
                                  server,
                                  handler {
                                      [this](struct kevent& event) {
                                          handle_server_disconnect(event);
                                      }
                                  }
                                  );
            }
            break;
        case State::SEND_SERVER:
            set_write_function(
                               server,
                               handler{
                                   [this](struct kevent &event) {
                                       handle_server_write(event);
                                   }
                               }
                               );
            
            set_read_function(
                              client,
                              handler{
                                      [this](struct kevent &event) {
                                          get_client_body(event);
                                      }
                              }
            );
            break;
        case State::RECEIVE_SERVER:
            header.clear();
            body_buffer.clear();
            
            set_read_function(
                              server,
                              handler{
                                  [this](struct kevent &event) {
                                      get_server_header(event);
                                  }
                              }
                              );
            break;
        case State::SEND_CLIENT:
            set_write_function(
                               client,
                               [this](struct kevent &event) {
                                   handle_client_write(event);
                               }
                               );
            
            set_read_function(
                              server,
                              [this](struct kevent &event) {
                                  get_server_body(event);
                              }
                              );
            break;
        default:
            safe_client_disconnect();
            safe_server_disconnect();
            deleter();
    }
}

void tcp_connection::set_read_function(std::unique_ptr<proxy_client>& object, handler hand) {
    assert(object);
    if (object->get_event_read().is_valid()) {
        object->get_event_read().change_function(std::move(hand));
    } else {
        object->set_read_event(
                           event_registration{
                               queue,
                               object->get_socket(),
                               EVFILT_READ,
                               std::move(hand),
                           }
                           );
    }
    object->resume_read();
}

void tcp_connection::set_write_function(std::unique_ptr<proxy_client>& object, handler hand) {
    assert(object);
    if (object->get_event_write().is_valid()) {
        object->get_event_write().change_function(std::move(hand));
    } else {
        object->set_write_event(
                               event_registration{
                                   queue,
                                   object->get_socket(),
                                   EVFILT_WRITE,
                                   std::move(hand)
                               }
                               );
    }
    object->resume_write();
}

void tcp_connection::set_deleter(std::function<void()> del) {
    this->deleter = del;
}
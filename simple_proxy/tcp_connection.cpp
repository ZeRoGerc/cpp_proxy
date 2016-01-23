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

const std::string BAD_REQUEST = "HTTP/1.1 400 Bad Request\r\nServer: proxy\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 164\r\nConnection: close\r\n\r\n<html>\r\n<head><title>400 Bad Request</title></head>\r\n<body bgcolor=\"white\">\r\n<center><h1>400 Bad Request</h1></center>\r\n<hr><center>proxy</center>\r\n</body>\r\n</html>";

const std::string NOT_FOUND = "HTTP/1.1 404 Not Found\r\nServer: proxy\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: 160\r\nConnection: close\r\n\r\n<html>\r\n<head><title>404 Not Found</title></head>\r\n<body bgcolor=\"white\">\r\n<center><h1>404 Not Found</h1></center>\r\n<hr><center>proxy</center>\r\n</body>\r\n</html>";

const std::string buffer::chunked_end{"0\r\n\r\n"};
const int tcp_connection::CHUNK_SIZE = 1024;

std::string get_field(std::string const& data, std::string const& field) {
    size_t pos = data.find(field);
    if (pos == std::string::npos) {
        return "";
    }
    pos = pos + field.size() + 1; //+ 1 - skip :
    
    while (data[pos] == ' ') pos++;
    
    std::string result{};
    while (data[pos] != ' ' && data[pos] != '\n' && data[pos] != '\r') {
        result += data[pos];
        pos++;
    }
    return result;
}

buffer::buffer(std::string chunk) {
    available_data = 0;
    readed = 0;
    tail = std::string();
    data = chunk;
}

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
    : queue(q), cache(cache), client(new proxy_client(descriptor)), server(nullptr)
{
    client_timer = std::move(
                             event_registration {
                                 queue,
                                 client->get_socket(),
                                 EVFILT_TIMER,
                                 NULL,
                                 NOTE_SECONDS,
                                 600,
                                 handler{
                                     [this](struct kevent& event) {
                                         std::cout << "TIMER";
                                         safe_disconnect();
                                     }
                                 },
                                 true
                             }
    );
}

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

    try {
        server.reset(new proxy_client(ip, host, port));
        set_read_function(
                          server,
                          handler {
                              [this](struct kevent& event) {
                                  handle_server_disconnect(event);
                              }
                          }
                          );
        return true;
    } catch (...) {
        server.reset(nullptr);
        return false;
    }
}

tcp_connection::~tcp_connection() {
    server.reset(nullptr);
    client.reset(nullptr);
}

void tcp_connection::safe_server_disconnect() {
    server.reset(nullptr);
}

void tcp_connection::safe_client_disconnect() {
    client_timer.invalidate();
    client.reset(nullptr);
}

void tcp_connection::safe_disconnect() {
    safe_client_disconnect();
    safe_server_disconnect();
    
    if (state != State::RESOLVE) { // while resolving in progress connection can't die
        deleter();
    }
    deleted = true;
}

void tcp_connection::disconnect() {
    safe_client_disconnect();
    safe_server_disconnect();
    
    deleter();
}

void tcp_connection::get_client_body(struct kevent &event) {
    if (handle_client_disconnect(event) || body_buffer.amount_of_available_data() == 0)
        return;

    std::string chunk = client->read(CHUNK_SIZE);
   
//    std::cerr << "client body send " << client->get_socket() << ' ' << chunk << std::endl;
    
    body_buffer.append(chunk);
}

void tcp_connection::get_client_header(struct kevent &event) {
    if (handle_client_disconnect(event)) {
//        std::cout << "read client disconnect";
        return;
    }

    std::string chunk = client->read(CHUNK_SIZE);

//    std::cerr << "client header send " << client->get_socket() << ' '  << chunk << std::endl;
    header.append(chunk);

    if (header.get_state() == http_header::State::COMPLETE) {
        switch_state(State::RESOLVE);
        
        current_url = header.get_url();
        
        if (header.has_field("If-Match")
            || header.has_field("If-Modified-Since")
            || header.has_field("If-None-Match")
            || header.has_field("If-Range")
            || header.has_field("If-Unmodified-Since")) {
            //client not interested in caching
            current_url = "";
        }
        
        if (cache->is_cached(current_url)) {
            header.add_line("If-None-Match", get_field(cache->get(current_url), "ETag"));
        }
        
        task resolve {
                [this]() {
                    /*
                     during resolve connection couldn't die
                     but if we need to determine if connection is in valid state after resolve
                     we could check state of client
                     if client doesn't exist it means that connection die
                     */

                    try {
                        std::string host = header.retrieve_host();
                        size_t port = header.retrieve_port();
                        // since we pass data as header + body
                        size_t content_len = header.get_content_length() + header.size();
                        std::string ip = http_header::get_ip_by_host(host, port);
                        queue->execute_in_main(task{[this, host, ip, port, content_len](){
                            if (deleted) {
                                //if state is invalid just delete
                                disconnect();
                                return;
                            }
//                            std::cout << "RESOLVED " << client_s << std::endl;
                            bool is_ok = init_server(ip, host, port);
                            if (!is_ok) {
                                body_buffer = buffer(NOT_FOUND, static_cast<int>(NOT_FOUND.size()));
                                switch_state(State::SEND_CLIENT);
                                return;
                            }
                            //here we have valid server and valid client
                            body_buffer = buffer(header.get_string_representation(), static_cast<int>(content_len));
                            
                            switch_state(State::SEND_SERVER);
                        }});
                        
                    } catch (...) {
                        queue->execute_in_main(
                                               task{[this]()
                                                   {
                                                       body_buffer = buffer(BAD_REQUEST, static_cast<int>(BAD_REQUEST.size()));
                                                       switch_state(State::SEND_CLIENT);
                                                       return;
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
    
//    std::cerr << "client receive " << client->get_socket() << ' ' << body_buffer.get() << std::endl;
    
    body_buffer.pop_front(len);
    
    //if server finish sending and client receive all available data
    if (body_buffer.size() == 0 && body_buffer.amount_of_available_data() == 0) {
        client->stop_write();
        if (current_url.size() != 0) {
            //cache responce
            cache->append(current_url, body_buffer.get_all_data());
        }
        switch_state(State::RECEIVE_CLIENT); //start new request
    }
}

void tcp_connection::get_server_body(struct kevent &event) {
    if (handle_server_disconnect(event) || body_buffer.amount_of_available_data() == 0)
        return;
    
    std::string chunk = server->read(CHUNK_SIZE);
    
//    std::cout << "server body send " << server->get_socket() << ' ' << chunk << std::endl;
    
    body_buffer.append(chunk);
}

void tcp_connection::get_server_header(struct kevent &event) {
    if (handle_server_disconnect(event))
        return;

    std::string chunk = server->read(CHUNK_SIZE);

//    std::cout << "server header send " << server->get_socket() << ' ' << chunk << std::endl;
    header.append(chunk);

    if (header.get_state() == http_header::State::COMPLETE) {
        /*
         Parse answer from server
         */
        
        if (cache->is_cached(current_url) && header.find_in_head("304")) {
            body_buffer = buffer(cache->get(current_url));
            switch_state(State::SEND_CLIENT);
            return;
        }
        
        if (header.get_field("Cache-Control") == "private"
            || header.get_field("Cache-Control") == "no-store"
            || header.get_field("ETag") == "") {
            //no caching
            current_url.clear();
        }
        
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
    
//    std::cout << "server receive " << server->get_socket() << ' ' << body_buffer.get(len) << std::endl;
    body_buffer.pop_front(len);
    
    //If we have already received all data from client and send it
    if (body_buffer.size() == 0 && body_buffer.amount_of_available_data() == 0) {
        server->stop_write();
        switch_state(State::RECEIVE_SERVER);
    }
}

bool tcp_connection::handle_server_disconnect(struct kevent& event) {
    if (deleted) {
        return true;
    }
    client_timer.refresh();
    
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        safe_server_disconnect();
        return true;
    }
    return false;
}

bool tcp_connection::handle_client_disconnect(struct kevent& event) {
    if (deleted) {
        return true;
    }
    client_timer.refresh();
    
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        safe_disconnect();
        return true;
    }
    return false;
}

void tcp_connection::switch_state(State new_state) {
    state = new_state;
//    std::cout << "switch_state: ";
//    switch (new_state) {
//        case State::RECEIVE_CLIENT:
//            std::cout << "RECEIVE_CLIENT ";
//            break;
//        case State::RESOLVE:
//            std::cout << "RESOLVE ";
//            break;
//        case State::SEND_SERVER:
//            std::cout << "SEND_SERVER ";
//            break;
//        case State::RECEIVE_SERVER:
//            std::cout << "RECEIVE_SERVER ";
//            break;
//        case State::SEND_CLIENT:
//            std::cout << "SEND_CLIENT ";
//            break;
//    }
//    
//    if (client) std::cout << "client: " << client->get_socket() << ' ';
//    if (server) std::cout << "server: " << server->get_socket() << ' ';
//    std::cout << std::endl;
    
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
    }
}

void tcp_connection::set_read_function(std::unique_ptr<proxy_client>& object, handler hand) {
    if (!object) return;
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
    if (!object) return;
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
    deleter = del;
}
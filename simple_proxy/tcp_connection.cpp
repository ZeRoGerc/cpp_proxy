//
//  tcp_connection.cpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 06.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#include <stdio.h>
#include "ipv4_endpoint.hpp"
#include "tcp_connection.hpp"
#include "tcp_client.hpp"
#include "tcp_server.hpp"


tcp_connection::tcp_connection(event_queue* q, int descriptor)
    : queue(q), client(new proxy_client(descriptor)), server() {}

void tcp_connection::start() {
    client->set_write_event(event_registration{
        queue,
        get_client_socket(),
        EVFILT_WRITE,
        handler{[this](struct kevent& event){
            this->handle_client_disconnect(event);
        }}
    });
    
    switch_state(State::RECEIVE_CLIENT);
}

bool tcp_connection::init_server() {
    try {
        size_t port = this->header.retrieve_port();

        //check if new server different to previous
        std::string host = this->header.retrieve_host();
        if (server && !(server->get_host() == host)) {
            safe_server_disconnect();
        }

        if (!server) {
            server.reset(new proxy_client(host, port));
        }
        
        server->set_write_event(event_registration{
            queue,
            get_server_socket(),
            EVFILT_WRITE,
            handler{[this](struct kevent& event){
                this->handle_server_disconnect(event);
            }}
        });
        
    } catch (std::exception const& e) {
        std::cerr << std::strerror(errno) << '\n';
        std::cerr << "get neither POST nor GET request\n";
        safe_disconnect();
        return false;
    }
    return true;
}

tcp_connection::~tcp_connection() {}

void tcp_connection::safe_server_disconnect() {
    if (!server) return;
    server.reset(nullptr);
}

void tcp_connection::safe_client_disconnect() {
    if (!client) return;
    client.reset(nullptr);
}

void tcp_connection::safe_disconnect() {
    delete this;
}

size_t tcp_connection::get_client_body(struct kevent &event, size_t residue) {
    if (handle_client_disconnect(event))
        return residue;

    if (residue == 0) {
        switch_state(State::SEND_SERVER);
    } else {
        std::string chunk = client->read(std::min(1024, static_cast<int>(residue)));
        body_buffer += chunk;
        return chunk.size();
    }

    return residue;
}

void tcp_connection::get_client_header(struct kevent &event) {
    if (handle_client_disconnect(event))
        return;

    std::string chunk = client->read(1024);

    std::cerr << "#############################################\n" << chunk << "\n###################################\n";
    header.append(chunk);

    if (header.get_state() == http_header::State::COMPLETE) {
        client->stop_read(); //wait while server creating
        if (server) server->stop_listen();
        
        task resolve{
                [this]() {
                    if (!this->init_server()) return;
                    size_t content_len = header.get_content_length();
                    content_len += header.size();

                    body_buffer = header.get_string_representation();

                    if (content_len != header.size()) {
                        client->get_event_read().change_function(
                             handler(
                                     [this, content_len](struct kevent &event) mutable {
                                         content_len -= get_client_body(event, content_len);
                                     }
                              )
                             );
                        
                        this->client->resume_read();
                    } else {
                        this->switch_state(tcp_connection::State::SEND_SERVER);
                    }

                    server->get_event_write().change_function(
                            handler(
                                    [this](struct kevent &event) {
                                        handle_server_write(event);
                                    }
                            )
                    );

                    this->server->resume_listen();
                }
        };

        callback(resolve);
    }
}

void tcp_connection::handle_client_write(struct kevent& event) {
    if (handle_client_disconnect(event))
        return;

    size_t len = client->send(body_buffer);
    if (body_buffer.size() == len && state == State::SEND_CLIENT) {
        switch_state(State::RECEIVE_CLIENT);
    } else {
        body_buffer = body_buffer.substr(len);
    }
}

size_t tcp_connection::get_server_content_body(struct kevent &event, size_t residue) {
    if (handle_server_disconnect(event))
        return residue;

    if (residue == 0) {
        switch_state(State::SEND_CLIENT);
    } else {
        std::string chunk = server->read(std::min(1024, static_cast<int>(residue)));
        body_buffer += chunk;
        return chunk.size();
    }

    return residue;
}

std::string tcp_connection::get_server_chunked_body(struct kevent &event, std::string tail) {
    static std::string const chunked_end{"\r\n0\r\n\r\n"};

    if (handle_server_disconnect(event))
        return chunked_end;
    
    std::string chunk = server->read(1024);
    if (tail.size() == 0) {
        tail += body_buffer;
    }
    
    body_buffer += chunk;
    tail += chunk;
    
    if (tail.size() > chunked_end.size()) {
        tail = tail.substr(tail.size() - chunked_end.size());
    }

    if (tail == chunked_end) {
        switch_state(State::SEND_CLIENT);
    }

    return tail;


}

void tcp_connection::get_server_header(struct kevent &event) {
    if (handle_server_disconnect(event))
        return;

    std::string chunk = server->read(1024);

    std::cerr << "#############################################\n" << chunk << "\n###################################\n";
    header.append(chunk);

    if (header.get_state() == http_header::State::COMPLETE) {
        body_buffer = header.get_string_representation();

        if (header.get_type() == http_header::Type::CHUNKED) {
            std::string tail{};
            server->get_event_read().change_function(
                    handler{
                            [this, tail](struct kevent &event) mutable {
                                this->get_server_chunked_body(event, tail);
                            }
                    }
            );
        } else {
            size_t content_len = header.get_content_length();
            content_len += header.size();


            server->get_event_read().change_function(
                    handler{
                            [this, content_len](struct kevent &event) mutable {
                                content_len -= this->get_server_content_body(event, content_len);
                            }
                    }
            );
        }

        client->get_event_write().change_function(
                handler{
                        [this](struct kevent &event) {
                            this->handle_client_write(event);
                        }
                }
        );
    }
}

void tcp_connection::handle_server_write(struct kevent& event) {
    if (handle_server_disconnect(event))
        return;

    //If we have already received all data from client and send it
    if (state == State::SEND_SERVER && body_buffer.size() == 0) {
        switch_state(State::RECEIVE_SERVER);
    } else {
        size_t sent = server->send(body_buffer);
        body_buffer = body_buffer.substr(sent);
    }
}

bool tcp_connection::handle_server_disconnect(struct kevent& event) {
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        std::cerr << "server disconnected" << std::endl;
        safe_server_disconnect();
        return true;
    }
    return false;
}

bool tcp_connection::handle_client_disconnect(struct kevent& event) {
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        std::cerr << "client disconnected" << std::endl;
        safe_disconnect();
        return true;
    }
    return false;
}

void tcp_connection::switch_state(State new_state) {
    state = new_state;
    switch (new_state) {
        case State::RECEIVE_CLIENT:
            header.clear();
            body_buffer.clear();
            client->set_read_event(event_registration{
                    queue,
                    get_client_socket(),
                    EVFILT_READ,
                    handler{[this](struct kevent &event) {
                        this->get_client_header(event);
                    }}
            });
            break;
        case State::SEND_SERVER:
            client->stop_read();
            break;
        case State::RECEIVE_SERVER:
            header.clear();
            body_buffer.clear();
            server->set_read_event(event_registration{
                    queue,
                    get_server_socket(),
                    EVFILT_READ,
                    handler{[this](struct kevent &event) {
                        this->get_server_header(event);
                    }}
            });
            break;
        case State::SEND_CLIENT:
            server->stop_write();
            break;
    }
}

void tcp_connection::set_callback(resolver _callback) {
    this->callback = _callback;
}
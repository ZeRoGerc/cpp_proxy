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


tcp_connection::tcp_connection(event_queue* q, int descriptor) {
    queue = q;
    client = new tcp_client(this, descriptor);
    server = nullptr;
}


void tcp_connection::init_server(event_queue* q, http_data const& request) {
    queue = q;
    
    size_t port = request.get_port();
    
    //check if new server different to previous
    std::string host = request.get_host();
    if (server && !(server->get_host() == host)) {
        safe_server_disconnect();
    }
    
    if (server == nullptr) {
        server = new tcp_server(this, host, port);
    }
}


tcp_connection::~tcp_connection() {
    if (client) {
        safe_client_disconnect();
    }
    if (server) {
        safe_server_disconnect();
    }
}


void tcp_connection::safe_server_disconnect() {
    if (server == nullptr) return;
    
    queue->delete_event(get_server_socket(), EVFILT_READ);
    queue->delete_event(get_server_socket(), EVFILT_WRITE);
    
    server->disconnect();
}


void tcp_connection::safe_client_disconnect() {
    if (client == nullptr) return;

    
    queue->delete_event(get_client_socket(), EVFILT_READ);
    client->disconnect();
}

void tcp_connection::receive_from_client(struct kevent& event) {
    http_data request{std::string()};
    
    while (true) {
        request.append(client->read(BUFFER_SIZE));
        if (request.get_state() == http_data::State::COMPLETE) {
            break;
        }
    }
    std::cout << request.get_header();
    
    try {
        init_server(queue, request);
        
        bool keep_alive = request.is_keep_alive();
        
        server_write = [this, request, keep_alive](struct kevent& event) {
            this->handle_server_write(event, request);
        };
        
        queue->add_event(get_server_socket(), EVFILT_WRITE, &server_write);
    } catch (std::exception e) {
        std::cout << std::strerror(errno) << '\n';
        std::cerr << "get neither POST nor GET request\n";
        delete this;
    }
}


void tcp_connection::handle_client_read(struct kevent& event) {
    if (event.flags & EV_EOF) {
        std::cerr << "client read disconnected " << event.ident << '\n';
        delete this;
    } else {
        receive_from_client(event);
    }
}

void tcp_connection::handle_client_write(struct kevent& event, std::string response) {
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        printf("client write disconnected\n");
        client->disconnect();
    } else {
        std::cerr << "########################################\n";
        std::cerr << response << "\n##########################################\n";
        
        size_t sent = client->send(response);
        
        //check if client receive all data
        if (sent < response.size()) {
            std::cerr << "CLIENT RECEIVED NOT ALL AVAILABLE DATA!!!\n";
            response = response.substr(sent);
            //just overwrite lambda, write event is listening by kqueue
            client_write = [this, response](struct kevent& event) {
                this->handle_client_write(event, response);
            };
        } else {
            //dont have to listen client write
            queue->delete_event(get_client_socket(), EVFILT_WRITE);
        }
    }
}


void tcp_connection::handle_server_read(struct kevent& event, http_data response) {
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        std::cerr << "server read disconnected " << event.ident << '\n';
        if (response.is_keep_alive()) {
            queue->delete_event(get_server_socket(), EVFILT_READ);
        } else {
            server->disconnect();
        }
    } else {
        std::string buffer = server->read(event.data);
        
        response.append(buffer);
        
        if (response.get_state() != http_data::State::COMPLETE) {
            server_read = [this, response](struct kevent& event) {
                this->handle_server_read(event, response);
            };
        } else {
            //don't have to listen anymore
            queue->delete_event(get_server_socket(), EVFILT_READ);
            
            std::string to_sent = response.get_header() + response.get_body();
            
            client_write = [this, to_sent](struct kevent& event) {
                this->handle_client_write(event, to_sent);
            };
            
            queue->add_event(get_client_socket(), EVFILT_WRITE, &client_write);
        }
    }
}


void tcp_connection::handle_server_write(struct kevent& event, http_data request) {
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        printf("server write disconnected\n");
        if (request.is_keep_alive()) {
            queue->delete_event(get_server_socket(), EVFILT_WRITE);
        } else {
            server->disconnect();
        }
    } else {
        server->send(request.get_header() + request.get_body());
        
        //Dont have to listen EVFILT_WRITE from server anymore
        queue->delete_event(get_server_socket(), EVFILT_WRITE);
        
        server_read = [this](struct kevent& event) {
            this->handle_server_read(event, http_data{std::string(), http_data::Type::RESPONCE});
        };
        
        queue->add_event(get_server_socket(), EVFILT_READ, &server_read);
    }
}


void tcp_connection::set_client_handler(handler handler) {
    client_read = handler;
}


handler* tcp_connection::get_client_handler() {
    return &client_read;
}


int tcp_connection::get_client_socket() {
    assert(client != nullptr);
    return client->get_socket();
}


int tcp_connection::get_server_socket() {
    assert(server != nullptr);
    return server->get_socket();
}
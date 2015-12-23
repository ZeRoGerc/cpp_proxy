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


void tcp_connection::start() {
    client_read = [this](struct kevent& event) {
        this->handle_client_read(event);
    };
    
    queue->add_event(get_client_socket(), EVFILT_READ, &client_read);
}

void tcp_connection::init_server(event_queue* q) {
    queue = q;
    
    size_t port = this->buffer.get_port();
    
    //check if new server different to previous
    std::string host = this->buffer.get_host();
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

void tcp_connection::safe_disconnect() {
    delete this;
}

void tcp_connection::get_http_request(struct kevent& event) {
    std::string chunk = client->read(BUFFER_SIZE);
    
    //For sure, that two threads can not write to request in one time
    queue->delete_event(get_client_socket(), EVFILT_READ);
    
    task get_http_info = [this, chunk]() {
        std::cerr << "#############################################\n" << chunk << "\n###################################\n";
        this->buffer.append(chunk);
        if (this->buffer.get_state() == http_data::State::COMPLETE) {
            std::cerr << "ZASHOL\n";
            this->finish_server_initialization(this->queue);
        } else {
            this->queue->add_event(this->get_client_socket(), EVFILT_READ, &this->client_read);
        }
    };
    
    callback(get_http_info);
}


void tcp_connection::finish_server_initialization(event_queue* queue) {  
    try {
        init_server(queue);
        
        server_write = [this](struct kevent& event) {
            this->handle_server_write(event);
        };
        
        queue->add_event(get_server_socket(), EVFILT_WRITE, &server_write);
        
    } catch (std::exception const& e) {
        std::cerr << std::strerror(errno) << '\n';
        std::cerr << "get neither POST nor GET request\n";
        delete this;
    }
}


void tcp_connection::handle_client_read(struct kevent& event) {
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        std::cerr << "client read disconnected " << event.ident << '\n';
        safe_disconnect();
    } else {
        get_http_request(event);
    }
}

void tcp_connection::handle_client_write(struct kevent& event, std::string response) {
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        printf("client write disconnected\n");
        safe_disconnect();
    } else {
        size_t sent = client->send(response);
        
        //check if client receive all data
        //TODO resolver maybe
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
            
            //Could read now again
            this->buffer.clear_data();
            queue->add_event(get_client_socket(), EVFILT_READ, &client_read);
        }
    }
}


void tcp_connection::handle_server_read(struct kevent& event) {
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        std::cerr << "server read disconnected " << event.ident << '\n';
        safe_server_disconnect();
    } else {
        std::string chunk = server->read(event.data);
        
        //For sure, that two threads can not write to request in one time
        queue->delete_event(get_server_socket(), EVFILT_READ);
        
        //Receive not all data
        if (chunk.size() < event.data) {
            std::cerr << "SERVER SEND NOT ALL AVAILABLE DATA\n";
            
            task append {
                [this, chunk] {
                    this->buffer.append(chunk);
                    this->queue->add_event(this->get_server_socket(), EVFILT_READ, &this->server_read);
                }
            };
            callback(append);
            return;
        }
        
        //If we read all available data we need another kind of task
        task append {
            [this, chunk] {
                this->buffer.append(chunk);
                if (this->buffer.get_state() == http_data::State::COMPLETE) {
                    //don't have to listen anymore
                    
                    std::string to_sent = this->buffer.get_header() + this->buffer.get_body();
                    this->buffer.clear_data();
                    
                    client_write = [this, to_sent](struct kevent& event) {
                        this->handle_client_write(event, to_sent);
                    };
                    
                    this->queue->add_event(this->get_client_socket(), EVFILT_WRITE, &this->client_write);
                }
                
                //Just read another chunk of data
                this->queue->add_event(this->get_server_socket(), EVFILT_READ, &this->server_read);
            }
        };
        callback(append);
    }
}


void tcp_connection::handle_server_write(struct kevent& event) {
    if ((event.flags & EV_EOF) && (event.data == 0)) {
        printf("server write disconnected\n");
        safe_server_disconnect();
    } else {
        size_t need = this->buffer.get_header().size() + this->buffer.get_body().size();
        size_t amount = server->send(this->buffer.get_header() + this->buffer.get_body());
        assert(amount == need);
        
        
        //Dont have to listen EVFILT_WRITE from server anymore
        queue->delete_event(get_server_socket(), EVFILT_WRITE);
        
        buffer = http_data(std::string{}, http_data::Type::RESPONCE);
        
        server_read = [this](struct kevent& event) {
            this->handle_server_read(event);
        };
        
        queue->add_event(get_server_socket(), EVFILT_READ, &server_read);
    }
}


int tcp_connection::get_client_socket() {
    assert(client != nullptr);
    return client->get_socket();
}


int tcp_connection::get_server_socket() {
    assert(server != nullptr);
    return server->get_socket();
}

void tcp_connection::set_callback(resolver _callback) {
    this->callback = _callback;
}
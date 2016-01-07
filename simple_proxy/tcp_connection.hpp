//
//  tcp_pair.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 28.11.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef tcp_connection_hpp
#define tcp_connection_hpp

#include <stdio.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "event_queue.hpp"
#include "http_parse.hpp"
#include "http_data.hpp"
#include "tasks_poll.hpp"
#include "http_header.hpp"
#include "proxy.hpp"
#include "proxy_client.h"


struct tcp_connection {
private:
    static const size_t CHUNK_SIZE = 1024;
    static const size_t BUFFER_SIZE = 16384;

    enum class State {UNDEFINED, RECEIVE_CLIENT, SEND_SERVER, RECEIVE_SERVER, SEND_CLIENT};

    State state = State::UNDEFINED;
    
    std::unique_ptr<proxy_client> client;
    std::unique_ptr<proxy_client> server;
    event_queue* queue;
    
    resolver callback;

    http_header header;
    std::string body_buffer;

    bool init_server();
    void switch_state(State new_state);

    size_t get_client_body(struct kevent &event, size_t residue);
    void get_client_header(struct kevent &event);

    size_t get_server_content_body(struct kevent &event, size_t residue);
    std::string get_server_chunked_body(struct kevent &event, std::string tail);
    void get_server_header(struct kevent &event);

    void handle_client_write(struct kevent& event);
    void handle_server_write(struct kevent& event);

    bool handle_server_disconnect(struct kevent& event);
    bool handle_client_disconnect(struct kevent& event);

public:
    //Don't forget to set callback after constructor
    tcp_connection(event_queue* queue, int descriptor);
    
    ~tcp_connection();

    size_t get_client_socket() {
        assert(client);
        return client->get_socket();
    }

    size_t get_server_socket() {
        assert(server);
        return server->get_socket();
    }
    
    void set_callback(resolver callback);
    
    void safe_server_disconnect();
    
    void safe_client_disconnect();
    
    void safe_disconnect();

    void start();
};

#endif /* tcp_pair_hpp */

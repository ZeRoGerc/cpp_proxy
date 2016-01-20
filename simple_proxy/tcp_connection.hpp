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
#include "tasks_poll.hpp"
#include "http_header.hpp"
#include "proxy.hpp"
#include "proxy_client.h"
#include "lru_cache.hpp"

struct buffer {
private:
    std::string data;
    
    /*
     amount of data that server or client could add to body_buffer
     0 means server or client finish reading data
     -1 means chunked encoding(infinity)
     */
    int available_data = 0;
    
    /*
     pointer to the last readed char
     */
    mutable size_t readed = 0;
    
    /*
     stores last bytes to determine end of chunked data
     */
    std::string tail;
    
    static const std::string chunked_end;
    
public:
    buffer() {};
    buffer(std::string);
    buffer(std::string, int amount_of_data);
    
    buffer(buffer const&) = delete;
    buffer& operator=(buffer const&) = delete;
    
    buffer(buffer&&) = default;
    buffer& operator=(buffer&&) = default;
    
    int amount_of_available_data() const {
        return available_data;
    }
    
    void append(std::string chunk);
    
    std::string get(size_t amount = -1) const;
    
    void pop_front(size_t amount);
    
    void clear();
    
    std::string get_all_data();
    
    size_t size() const;
};


struct tcp_connection {
private:
    using cache_type = lru_cache<std::string, std::string>;
    static const int CHUNK_SIZE;
    static const int BUFFER_SIZE;

    enum class State {UNDEFINED, RECEIVE_CLIENT, RESOLVE, SEND_SERVER, RECEIVE_SERVER, SEND_CLIENT, DELETED};

    State state = State::UNDEFINED;
    
    std::unique_ptr<proxy_client> client;
    std::unique_ptr<proxy_client> server;
    event_queue* queue;
    cache_type* cache;
    
    event_registration client_timer;
    
    /*
     funcition used for executeing tasks in background thread
     */
    resolver callback;
    
    /*
     callback to proxy server
     invoked when connection died
     */
    std::function<void()> deleter;

    /*
     header used for storing http request or responce
     from server or client
     */
    http_header header;
    
    /*
     buffer used for simultaneous transfer of data
     between server and client
     */
    buffer body_buffer;
    
    std::string current_url;
    
    bool init_server(std::string const& ip, std::string const& host, size_t port);
    //always be sure to call this ONLY in main thread
    void switch_state(State new_state);

    void get_client_body(struct kevent &event);
    void get_client_header(struct kevent &event);

    void get_server_body(struct kevent &event);
    void get_server_header(struct kevent &event);

    void handle_client_write(struct kevent& event);
    void handle_server_write(struct kevent& event);

    bool handle_server_disconnect(struct kevent& event);
    bool handle_client_disconnect(struct kevent& event);
    
    //event_registration is_listen is always false after finishing
    void set_read_function(std::unique_ptr<proxy_client>&, handler);
    void set_write_function(std::unique_ptr<proxy_client>&, handler);

public:
    //Don't forget to set callback and deleter after constructor
    tcp_connection(event_queue* queue, cache_type* cache, int descriptor);
    
    ~tcp_connection();

    int get_client_socket() {
        assert(client);
        return client->get_socket();
    }

    int get_server_socket() {
        assert(server);
        return server->get_socket();
    }
    
    void set_callback(resolver callback);
    
    void set_deleter(std::function<void()> del);
    
    void safe_server_disconnect();
    
    void safe_client_disconnect();
    
    void safe_disconnect();

    void start();
};

#endif /* tcp_pair_hpp */

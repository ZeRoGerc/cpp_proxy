//
// Created by Vladislav Sazanovich on 03.01.16.
//

#ifndef SIMPLE_PROXY_PROXY_CLIENT_H
#define SIMPLE_PROXY_PROXY_CLIENT_H


#include <iosfwd>
#include <assert.h>
#include "socket.hpp"
#include "event_registration.h"

struct proxy_client {
public:
    proxy_client(std::string const& ip, std::string const& host = "localhost", size_t port = 80);

    proxy_client(int descriptor);

    proxy_client(proxy_client const&) = delete;
    proxy_client& operator=(proxy_client const&) = delete;

    int get_socket() const {
        return client_socket.value();
    }

    size_t send(std::string const& request);

    std::string read(size_t len);

    std::string const& get_host() const {
        return host;
    }

    void set_read_event(event_registration&& event) {
        event_read = std::move(event);
        resume_read();
    }

    void set_write_event(event_registration&& event) {
        event_write = std::move(event);
        resume_write();
    }

    void stop_read() {
        event_read.stop_listen();
    }

    void stop_write() {
        event_write.stop_listen();
    }

    void resume_read() {
        event_read.resume_listen();
    }

    void resume_write() {
        event_write.resume_listen();
    }

    void stop_listen() {
        stop_read();
        stop_write();
    }

    void resume_listen() {
        resume_read();
        resume_write();
    }

    event_registration& get_event_read() {
        return event_read;
    }

    event_registration& get_event_write() {
        return event_write;
    }

private:
    struct socket client_socket;
    std::string host = "localhost";

    event_registration event_read;
    event_registration event_write;
};

#endif //SIMPLE_PROXY_PROXY_CLIENT_H

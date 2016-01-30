//
//  http_header.cpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 26.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#include "http_header.hpp"
#include "custom_exception.hpp"
#include <assert.h>
#include <iostream>

void http_header::append(std::string const& chunk) {
    static const std::string header_end{"\r\n\r\n"};
    
    data += chunk;
    
    if (data.find(header_end) != std::string::npos) {
        state = State::READED;
        init_properties();
    }
}

void http_header::init_properties() {
    static const std::string header_end{"\r\n\r\n"};
    type = Type::HEADER;
    
    size_t pos = data.find("\r\n");
    head = data.substr(0, pos);
    
    transform_to_relative();
    parse_is_chunked_encoding();
    parse_content_length();

    sz = data.find(header_end) + header_end.size();

    state = State::COMPLETE;
}

void http_header::parse_content_length() {
    static const std::string content_mark{"Content-Length:"};
    
    size_t pos = data.find(content_mark);
    if (pos == std::string::npos) {
        return;
    }
    
    assert(type != Type::CHUNKED);
    
    type = Type::CONTENT;
    
    content_length = 0;
    pos += content_mark.size();
    while (data[pos] == ' ') pos++;
    
    while (isdigit(data[pos])) {
        content_length *= 10;
        content_length += (data[pos] - '0');
        pos++;
    }
}

void http_header::parse_is_chunked_encoding() {
    static const std::string chunked_encoding_mark{"chunked"};
    
    if (data.find(chunked_encoding_mark) != std::string::npos) {
        type = Type::CHUNKED;
    }
}

std::string http_header::retrieve_host() const {
    static const std::string host_mark("\r\nHost:");
    
    size_t pos = data.find(std::string(host_mark));
    if (pos == std::string::npos) {
        return std::string("localhost");
    }
    
    std::string host;
    pos += host_mark.size();
    while (data[pos] == ' ') pos++;
    
    while (data[pos] != '\n' && data[pos] != '\r' && data[pos] != ':' && data[pos] != ' ') {
        host += data[pos++];
    }
    
    return host;
}


size_t http_header::retrieve_port() const {
    static const std::string host_mark("\r\nHost:");
    
    size_t pos = data.find(host_mark);
    if (pos == std::string::npos) {
        return 80;
    }
    
    pos += host_mark.size();
    while (data[pos] == ' ') pos++;
    
    while (data[pos] != '\n' && data[pos] != '\r' && data[pos] != ':') {
        pos++;
    }
    
    if (data[pos] != ':')
        return 80;
    pos++;
    
    size_t port = 0;
    while (isdigit(data[pos])) {
        port *= 10;
        port += (data[pos] - '0');
        pos++;
    }
    
    return port;
}


void http_header::transform_to_relative() {
    size_t pos = 0;
    //skip POST or GET
    while (data[pos] != ' ') pos++;
    pos++;
    
    if (data[pos] == '/')
        //it's already relative
        return;
    
    std::string host = retrieve_host();
    if (host == "localhost") {
        return;
    }
//    std::cout << "DATA " << data << std::endl;
    size_t host_pos = data.find(host);
    size_t end = host_pos;
    
    while (data[end] != '/') end++;
    
    data = data.substr(0, pos) + data.substr(end);
}


std::string http_header::get_field(std::string const& field) const {
    size_t pos = data.find(field);
    if (pos == std::string::npos) {
        return "";
    }
    pos = pos + field.size() + 1; //skip :
    
    while (data[pos] == ' ') pos++;
    
    std::string result{};
    while (data[pos] != ' ' && data[pos] != '\n' && data[pos] != '\r' && data[pos] != ',') {
        result += data[pos];
        pos++;
    }
    return result;
}

std::string http_header::get_url() const {
    size_t pos = data.find(' ');
    while (data[pos] == ' ') pos++;
    
    std::string result{retrieve_host()};
    while (data[pos] != ' ') {
        result += data[pos];
        pos++;
    }
    return result;
}


void http_header::add_line(std::string const& key, std::string const value) {
    data.substr(0, data.size() - 2) + key + ": " + value + "\n" + data.substr(data.size() - 2);
}

std::string  http_header::get_ip_by_host(std::string const& host, size_t port) {
    struct addrinfo hints, *res, *res0;
    int error;
    int s;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
        
    error = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res0);
    
    if (error) {
        std::string message{"getaddrinfo failed: "};
        message.append(std::strerror(error));
        throw custom_exception{message};
    }
    
    s = -1;
    for (res = res0; res; res = res->ai_next) {
        s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s < 0) {
            continue;
        }
        
        if (connect(s, res->ai_addr, res->ai_addrlen) < 0) {
            close(s);
            s = -1;
            continue;
        }
        
        break;  /* okay we got one */
    }
    if (s < 0) {
        throw custom_exception{"fail to find valid ip"};
    }
    
    if (res->ai_family == AF_INET) {
        struct sockaddr_in  *sockaddr_ipv4;
        sockaddr_ipv4 = (struct sockaddr_in *) res->ai_addr;
        std::string ip = std::string(inet_ntoa(sockaddr_ipv4->sin_addr));
        freeaddrinfo(res0);
        return ip;
    } else {
        throw custom_exception{"get ip by host failed: ai_family != AF_INET"};
    }
}

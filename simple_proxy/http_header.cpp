//
//  http_header.cpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 26.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#include "http_header.hpp"
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
    
    transform_to_relative();
    parse_is_chunked_encoding();
    parse_content_length();

    sz = data.find(header_end) + header_end.size();

    state = State::COMPLETE;
}

void http_header::parse_content_length() {
    static const std::string content_mark{"Content-Length: "};
    
    size_t pos = data.find(content_mark);
    if (pos == std::string::npos) {
        return;
    }
    
    assert(type != Type::CHUNKED);
    
    type = Type::CONTENT;
    
    content_length = 0;
    pos += content_mark.size();
    
    while (isdigit(data[pos])) {
        content_length *= 10;
        content_length += (data[pos] - '0');
        pos++;
    }
}

void http_header::parse_is_chunked_encoding() {
    static const std::string chunked_encoding_mark{"chunked"};
    
    if (data.find(chunked_encoding_mark) != std::string::npos) {
        if (type == Type::CONTENT) {
            // request must be either chunked or content
            throw std::exception();
        }
        type = Type::CHUNKED;
    }
}

std::string http_header::retrieve_host() const {
    static const std::string host_mark("Host: ");
    
    size_t pos = data.find(std::string(host_mark));
    if (pos == std::string::npos) {
        return std::string("localhost");
    }
    
    std::string host;
    pos += host_mark.size();
    while (data[pos] != '\n' && data[pos] != '\r' && data[pos] != ':') {
        host += data[pos++];
    }
    
    return host;
}


size_t http_header::retrieve_port() const {
    static const std::string host_mark("Host: ");
    
    size_t pos = data.find(host_mark);
    
    pos += host_mark.size();
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
    std::cout << "DATA " << data << std::endl;
    size_t host_pos = data.find(host);
    size_t end = host_pos;
    
    while (data[end] != '/') end++;
    
    data = data.substr(0, pos) + data.substr(end);
}
//
//  http_data.cpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 17.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#include "http_data.hpp"


http_data::http_data(std::string initial, Type t) {
    type = t;
    append(initial);
}

void http_data::append(std::string chunk) {
    static const std::string header_end{"\r\n\r\n"};
    static const std::string chunk_end{"\r\n0\r\n\r\n"};

    if (state == State::READING_HEADER) {
        header += chunk;
        
        size_t pos = header.find(header_end);
        if (pos != std::string::npos) {
            //give a tail to body because it's not a header
            body += header.substr(pos + header_end.size());
            
            //complement header
            header = header.substr(0, pos + header_end.size());
            initialize_properties();
            if (type != Type::CONTENT && type != Type::CHUNKED) {
                state = State::COMPLETE;
            } else {
                state = State::READING_BODY;
            }
        }
        chunk.clear();
    }
    
    if (state == State::READING_BODY) {
        if (type == Type::GET || type == Type::UNDEFINED) {
            //they mustn't have body
            throw std::exception();
        }
        
        body += chunk;
        
        if (type == Type::CONTENT) {
            if (body.size() >= content_length) {
                body = body.substr(0, content_length);
                state = State::COMPLETE;
            }
            return;
        }
        
        if (type == Type::CHUNKED) {
            size_t pos = body.find(chunk_end);
            if (pos != std::string::npos) {
                body = body.substr(0, pos + chunk_end.size());
                state = State::COMPLETE;
            }
            return;
        }
    }
}


void http_data::parse_content_length() {
    static const std::string content_mark{"Content-Length: "};
    
    size_t pos = header.find(content_mark);
    if (pos == std::string::npos) {
        return;
    }
    if (type == Type::CHUNKED) {
        // request must be either chunked or content
        throw std::exception();
    } else {
        type = Type::CONTENT;
    }
    
    content_length = 0;
    pos += content_mark.size();
    
    while (isdigit(header[pos])) {
        content_length *= 10;
        content_length += (header[pos] - '0');
        pos++;
    }
}


void http_data::parse_is_chunked_encoding() {
    static const std::string chunked_encoding_mark{"Transfer-Encoding: chunked"};
    
    if (header.find(chunked_encoding_mark) != std::string::npos) {
        if (type == Type::CONTENT) {
            // request must be either chunked or content
            throw std::exception();
        }
        type = Type::CHUNKED;
    }
}

std::string http_data::get_host() const {
    static const std::string host_mark("Host: ");
    
    size_t pos = header.find(std::string(host_mark));
    if (pos == std::string::npos) {
        throw std::exception();
    }
    
    std::string host;
    pos += host_mark.size();
    while (header[pos] != '\n' && header[pos] != '\r' && header[pos] != ':') {
        host += header[pos++];
    }
    
    return host;
}


size_t http_data::get_port() const {
    static const std::string host_mark("Host: ");
    
    size_t pos = header.find(host_mark);
    std::string header;
    pos += host_mark.size();
    while (header[pos] != '\n' && header[pos] != '\r' && header[pos] != ':') {
        pos++;
    }
    
    if (header[pos] != ':')
        return 80;
    pos++;
    
    size_t port = 0;
    while (isdigit(header[pos])) {
        port *= 10;
        port += (header[pos] - '0');
    }
    
    return port;
}



void http_data::initialize_properties() {
    if (type == Type::UNDEFINED && header.find("GET ") == std::string::npos && header.find("POST ") == std::string::npos) {
        return;
    }
   
    if (header.find("GET ") != std::string::npos) {
        type = Type::GET;
        transform_to_relative();
        return;
    }
    
    if (header.find("POST ") != std::string::npos) {
        transform_to_relative();
        parse_content_length();
        parse_is_chunked_encoding();
        return;
    }
    
    //WE GOT RESPONCE
    if (type == Type::RESPONCE) {
        parse_content_length();
        parse_is_chunked_encoding();
    }
}


void http_data::transform_to_relative() {
    size_t pos = 0;
    //skip POST or GET
    while (header[pos] != ' ') pos++;
    pos++;
    
    if (header[pos] == '/')
        //it's already relative
        return;
    
    std::string host = get_host();
    size_t host_pos = header.find(host);
    
    header = header.substr(0, pos) + header.substr(host_pos + host.length());
}
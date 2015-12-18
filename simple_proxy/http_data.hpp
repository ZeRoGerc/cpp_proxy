//
//  http_request.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 17.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef http_request_hpp
#define http_request_hpp

#include <stdio.h>
#include <string>

struct http_data {
public:
    enum class State {READING_HEADER, READING_BODY, COMPLETE};
    enum class Type {UNDEFINED, RESPONCE, GET, CHUNKED, CONTENT};
    
    http_data(std::string initial, Type type = Type::UNDEFINED);

    void append(std::string chunk);
    
    void clear_data() {
        header.clear();
        body.clear();
        type = Type::UNDEFINED;
        state = State::READING_HEADER;
    }
    
    bool is_keep_alive() const{
        return keep_alive;
    }
    
    size_t get_content_length() const{
        return content_length;
    }
    
    State get_state() const{
        return state;
    }
    
    Type get_type() const{
        return type;
    }
    
    std::string get_header() const {
        return header;
    }
    
    std::string get_body() const {
        return body;
    }
    
    std::string get_host() const;
    
    size_t get_port() const;
    
    
private:
    std::string header{};
    std::string body{};
    ssize_t content_length = 0;
    bool keep_alive = false;
    
    bool header_complete = false;
    bool is_complete = false;
    
    
    void transform_to_relative();
    
    void parse_keep_alive();
    void parse_content_length();
    void parse_is_chunked_encoding();
    void initialize_properties();
    
    State state = State::READING_HEADER;
    Type type = Type::UNDEFINED;
};

#endif /* http_request_hpp */

//
//  client.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 28.11.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef socket_hpp
#define socket_hpp

#include <string>

struct socket {
public:
    socket(socket const&) = delete;
    socket& operator=(socket const&) = delete;
    
    socket(socket&&) = delete;
    socket& operator=(socket&&) = delete;
    
    socket(int descriptor);
    
    socket(std::string const& ip, size_t port);

    ~socket();
    
    int value() const {
        return client_socket;
    }
    
private:
    int client_socket;
};

#endif /* client_hpp */

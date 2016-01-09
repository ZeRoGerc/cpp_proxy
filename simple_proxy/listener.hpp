//
//  listener.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 18.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef listener_hpp
#define listener_hpp

#include <stdio.h>
#include "proxy.hpp"

struct listener {
public:
    static void listen(proxy* proxy_server) {
        while (true) {
            std::cerr << "LISTEN_ENTER\n";
            
            task current_task = proxy_server->get_background_task();
            
            std::cerr << "LISTEN_EXECUTE\n";
            current_task();
        }
    }
};

#endif /* listener_hpp */

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
    static void listen(proxy* _proxy) {
        while (true) {
            std::cerr << "LISTEN1\n";
            tasks_pull* pull = &_proxy->_pull;
            std::unique_lock<std::mutex> _lock(pull->mutex);
            
            while (pull->size() == 0) {
                std::cerr << "LISTEN$\n";
                pull->condition.wait(_lock);
            }
            
            //We got one
            task current_task = pull->pop();
            _lock.unlock();
            
            std::cerr << "LISTEN3\n";
            current_task();
        }
    }
};

#endif /* listener_hpp */

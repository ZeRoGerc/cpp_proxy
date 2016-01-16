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
    static void listen(event_queue* eq) {
        while (true) {
            task current_task = eq->get_background_task();
            current_task();
        }
    }
};

#endif /* listener_hpp */

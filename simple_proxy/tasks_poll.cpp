//
//  tasks_poll.cpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 18.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#include "tasks_poll.hpp"

void tasks_poll::push(task task) {
    std::unique_lock<std::mutex> lock(mutex);
    _pull.push(task);
    
    condition.notify_one();
}

task tasks_poll::pop() {
    std::unique_lock<std::mutex> lock(mutex);
    
    while (_pull.size() == 0) {
        condition.wait(lock);
    }
    
    //We've got one
    task result = _pull.front();
    _pull.pop();
    
    lock.unlock();
    return result;
}
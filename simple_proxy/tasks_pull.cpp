//
//  tasks_pull.cpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 18.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#include "tasks_pull.hpp"

void tasks_pull::push(task task) {
    std::unique_lock<std::mutex> lock(mutex);
    _pull.push(task);
    
    condition.notify_one();
}

task tasks_pull::pop() {
    std::unique_lock<std::mutex> lock(mutex);
    
    while (_pull.size() == 0) {
        std::cerr << "LISTEN_WAIT\n";
        condition.wait(lock);
    }
    
    //We got one
    task result = _pull.front();
    _pull.pop();
    
    lock.unlock();
    return result;
}
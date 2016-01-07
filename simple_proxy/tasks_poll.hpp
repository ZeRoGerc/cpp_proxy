//
//  tasks_poll.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 18.12.15.
//  Copyright © 2015 ZeRoGerc. All rights reserved.
//

#ifndef tasks_pull_hpp
#define tasks_pull_hpp

#include <stdio.h>
#include <queue>
#include <string>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <iostream>

typedef std::function<void()> task;
typedef std::function<void(task)> resolver;

struct tasks_poll {
private:
    std::queue<task> _pull;
    std::condition_variable condition;
    std::mutex mutex;
public:
    tasks_poll() {};
    
    void push(task task);
    task pop();
    
    size_t size() {
        return _pull.size();
    }
};

#endif /* tasks_pull_hpp */

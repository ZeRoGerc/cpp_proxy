//
//  tasks_pull.hpp
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

typedef std::function<void()> task;
typedef std::function<void(task)> resolver;

struct tasks_pull {
private:
    std::queue<task> _pull;
public:
    tasks_pull() {};
    
    std::condition_variable condition;
    std::mutex mutex;
    void push(task task);
    task pop();
    
    size_t size() {
        return _pull.size();
    }
};

#endif /* tasks_pull_hpp */

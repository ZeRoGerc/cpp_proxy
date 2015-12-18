////
////  tasks_pull.hpp
////  simple_proxy
////
////  Created by Vladislav Sazanovich on 18.12.15.
////  Copyright © 2015 ZeRoGerc. All rights reserved.
////
//
//#ifndef tasks_pull_hpp
//#define tasks_pull_hpp
//
//#include <stdio.h>
//#include <queue>
//#include <string>
//#include <mutex>
//#include <functional>
//
//
//typedef std::function<void()> resolver;
//typedef std::function<void()> task;
//
//struct tasks_pull {
//private:
//    std::queue<task> queue;
//    std::condition_variable condition;
//    std::mutex mutex;
//public:
//    void push(task task);
//    task pop();
//};
//
//#endif /* tasks_pull_hpp */

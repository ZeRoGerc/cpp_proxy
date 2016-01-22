//
//  custom_exception.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 23.01.16.
//  Copyright © 2016 ZeRoGerc. All rights reserved.
//

#ifndef custom_exception_hpp
#define custom_exception_hpp

#include <exception>
#include <string>

struct custom_exception : std::exception {
    custom_exception(const std::string& str)
        : error_info(str)
    {}
    
    custom_exception(std::string&& msg)
        : error_info(std::move(msg))
    {}
    
    const char* what() const noexcept override {
        return error_info.c_str();
    }
    
private:
    std::string error_info;
};

#endif /* custom_exception_hpp */

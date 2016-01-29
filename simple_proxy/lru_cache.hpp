//
//  lru_cache.hpp
//  simple_proxy
//
//  Created by Vladislav Sazanovich on 15.01.16.
//  Copyright © 2016 ZeRoGerc. All rights reserved.
//

#ifndef lru_cache_hpp
#define lru_cache_hpp

#include <stdio.h>
#include <map>
#include <list>
#include <mutex>
#include <thread>

template<typename K, typename V>
struct lru_cache {
private:
    std::list< std::pair<K, V> > lst;
    std::map<K,decltype(lst.begin())> map;
    size_t max_size = 1;
    
    mutable std::mutex mutex;
public:
    lru_cache(size_t size): max_size(size) {}
    
    lru_cache(lru_cache const&) = delete;
    lru_cache& operator=(lru_cache const&) = delete;
    
    lru_cache(lru_cache&&) = default;
    lru_cache& operator=(lru_cache&&) = default;
    
    template<typename VV>
    void append(K const& key, VV&& value) {
        std::unique_lock<std::mutex> lock(mutex);
        
        if (map.find(key) != map.end()) {
            map[key]->second = std::forward<VV>(value);
            lst.splice(lst.begin(), lst, map[key]);
        } else {
            lst.emplace_front(key, std::forward<VV>(value));
        }
        
        map[key] = lst.begin();
        
        if (lst.size() > max_size) {
            map.erase(lst.back().first);
            lst.pop_back();
        }
    }
    
    inline bool is_cached(const K& key) const {
        std::unique_lock<std::mutex> lock(mutex);
        return map.find(key) != map.end();
    }
    
    const V& get(const K& key) const {
        std::unique_lock<std::mutex> lock(mutex);
        
        auto it = map.find(key);
        if (it == map.end()) {
            throw std::exception();
        } else {
            return it->second->second;
        }
    }
};

#endif /* lru_cache_hpp */

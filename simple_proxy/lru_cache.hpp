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

template<typename K, typename V>
struct lru_cache {
private:
    static const size_t SIZE = 100;
    std::list< std::pair<K, V> > lst;
    std::map<K,decltype(lst.begin())> map;
public:
    lru_cache() {}
    
    lru_cache(lru_cache const&) = delete;
    lru_cache& operator=(lru_cache const&) = delete;
    
    lru_cache(lru_cache&&) = default;
    lru_cache& operator=(lru_cache&&) = default;
    
    void append(K const& key, V&& value) {
        std::cout << "CACHED " << std::endl;
//        std::cout << key << std::endl;
//        std::cout << value << std::endl;
        
        if (is_cached(key)) {
            map[key]->second = std::forward<V>(value);
            lst.splice(lst.begin(), lst, map[key]);
        } else {
            lst.emplace_front(key, std::forward<V>(value));
        }
        
        map[key] = lst.begin();
        
        if (lst.size() > SIZE) {
            map.erase(lst.back().first);
            lst.pop_back();
        }
    }
    
    inline bool is_cached(const K& key) const {
        return map.find(key) != map.end();
    }
    
    const V& get(const K& key) const {
        auto it = map.find(key);
        std::cout << "FROM CACHE " << key << std::endl;
        if (it == map.end()) {
            throw std::exception();
        } else {
            return it->second->second;
        }
    }
};

#endif /* lru_cache_hpp */

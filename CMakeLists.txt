cmake_minimum_required(VERSION 3.2)
project(simple_proxy)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        simple_proxy/event_queue.hpp
        simple_proxy/event_queue.cpp
        simple_proxy/http_parse.hpp
        simple_proxy/socket.hpp
        simple_proxy/socket.cpp
        simple_proxy/main.cpp
        simple_proxy/main_server.hpp
        simple_proxy/main_server.cpp
        simple_proxy/proxy.hpp
        simple_proxy/proxy.cpp
        simple_proxy/tcp_connection.cpp
        simple_proxy/tcp_connection.hpp
        simple_proxy/http_header.hpp
        simple_proxy/http_header.cpp
        simple_proxy/listener.hpp
        simple_proxy/tasks_poll.cpp
        simple_proxy/tasks_poll.hpp
        simple_proxy/proxy_client.cpp
        simple_proxy/proxy_client.h
        simple_proxy/event_registration.cpp
        simple_proxy/event_registration.h
        simple_proxy/lru_cache.hpp)

add_executable(simple_proxy ${SOURCE_FILES} simple_proxy/event_queue.cpp simple_proxy/socket.cpp simple_proxy/main_server.cpp simple_proxy/proxy.cpp simple_proxy/proxy_client.cpp)
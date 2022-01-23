add_executable(hello-world hello-world.cpp)
add_executable(demo demo.c)
add_executable(libevent_tcp_server libevent_tcp_server.cpp wrap.cpp)
add_executable(bufferevent_server bufferevent_server.cpp)


target_link_libraries(hello-world event)
target_link_libraries(demo event)
target_link_libraries(libevent_tcp_server event)
target_link_libraries(bufferevent_server event)
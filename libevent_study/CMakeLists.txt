add_executable(hello-world hello-world.cpp)
add_executable(libevent_tcp_server libevent_tcp_server.cpp wrap.cpp)
add_executable(bufferevent_server bufferevent_server.cpp)
add_executable(bufferevent_client bufferevent_client.c)


target_link_libraries(hello-world event)
target_link_libraries(libevent_tcp_server event)
target_link_libraries(bufferevent_server event)
target_link_libraries(bufferevent_client event)
add_executable(tcp_client tcp_client.cpp)
add_executable(tcp_server tcp_server.cpp)
add_executable(tcp_process_server tcp_process_server.cpp wrap.cpp)
add_executable(tcp_pthread_server tcp_pthread_server.cpp wrap.cpp)



target_link_libraries(tcp_pthread_server pthread)

add_executable(epoll_server epoll_server.cpp wrap.cpp)
add_executable(epoll_server_edge_triggered epoll_server_edge_triggered.cpp wrap.cpp)

add_executable(pipe_epoll pipe_epoll.cpp)


add_executable(epoll_server_edge_triggered_thread_pool
        epoll_server_edge_triggered_thread_pool.cpp
        wrap.cpp

        )


target_link_libraries(epoll_server_edge_triggered_thread_pool pthread)

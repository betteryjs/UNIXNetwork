add_executable(epoll_webserver
        epoll_webserver.cpp
        wrap.cpp
        urlencode_urldecode.cpp
        )

add_executable(event_webserver
        event_webserver.cpp
        urlencode_urldecode.cpp

        )


target_link_libraries(event_webserver event)

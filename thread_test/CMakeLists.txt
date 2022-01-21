
set(CMAKE_CXX_STANDARD 20)

add_executable(test test.cpp thread_pool_yjs.cpp)

target_link_libraries(test pthread)
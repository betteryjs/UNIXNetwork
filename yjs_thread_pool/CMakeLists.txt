#add_executable(thread_pool_class thread_pool_class.cpp)
#
#
#target_link_libraries(thread_pool_class pthread)


add_executable(test_pools test_pool.cpp)

target_link_libraries(test_pools pthread)




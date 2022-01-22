add_executable(hello-world hello-world.c)
add_executable(demo demo.c)

target_link_libraries(hello-world event)
target_link_libraries(demo event)
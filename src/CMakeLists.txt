set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_executable(generate_srpc generate_srpc.cpp) 
target_link_libraries(generate_srpc PRIVATE srpc)

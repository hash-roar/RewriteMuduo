

set(CMAKE_CXX_STANDARD 17)
add_executable(test_server test/test_server.cc)
target_link_libraries(test_server PRIVATE rnet fmt)


set(CMAKE_CXX_STANDARD 17)
add_executable(test_server test/test_server.cc)
add_executable(test_full_server test/test_full_server.cc)
target_link_libraries(test_server PRIVATE rnet fmt)
target_link_libraries(test_full_server PRIVATE rnet fmt)
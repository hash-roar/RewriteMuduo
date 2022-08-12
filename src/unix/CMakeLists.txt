

set(CMAKE_CXX_STANDARD 17)
add_executable(test_duration test/test_duration.cc)
# add_executable(test_full_server test/test_full_server.cc)
# target_link_libraries(test_server PRIVATE rnet fmt)
target_link_libraries(test_duration PRIVATE rnet)
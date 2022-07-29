
set(CMAKE_CXX_STANDARD 17)
find_package(fmt)
add_executable(log_test test/test_log.cc)
add_executable(log_async_test test/test_asylogging.cc)
target_link_libraries(log_test PRIVATE rnet fmt)
target_link_libraries(log_async_test PRIVATE rnet)
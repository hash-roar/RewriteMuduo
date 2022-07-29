
set(CMAKE_CXX_STANDARD 17)
find_package(fmt)
add_executable(file_test test/file_test.cc)
target_link_libraries(file_test PRIVATE rnet)
target_link_libraries(file_test PRIVATE fmt)
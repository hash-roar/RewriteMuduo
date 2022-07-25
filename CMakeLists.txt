set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
cmake_minimum_required(VERSION 3.1)

project(rmuduo)
find_package(fmt)
add_executable(rmuduo main.cc)
add_link_options(rmuduo fmt)

add_subdirectory(src)
target_link_libraries(rmuduo PUBLIC fmt)
add_subdirectory(src/detail)

set(CXX_FLAGS
 -g
 # -DVALGRIND
 -DCHECK_PTHREAD_RETURN_VALUE
 -D_FILE_OFFSET_BITS=64
 -Wall
 -Wextra
 -Werror
 -Wconversion
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 -march=native
 # -MMD
 -std=c++17
 -rdynamic
)


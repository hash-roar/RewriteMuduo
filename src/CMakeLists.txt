
set(CMAKE_CXX_STANDARD 17)

find_package(pthread)
aux_source_directory( ./ SRC_DIR_SRCS)
aux_source_directory(./detail DETAIL_DIR_SRCS)
add_library(rnet ${SRC_DIR_SRCS} ${DETAIL_DIR_SRCS})

target_link_libraries(rnet  pthread)

add_subdirectory(detail)
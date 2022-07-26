
set(CMAKE_CXX_STANDARD 17)
aux_source_directory( ./ SRC_DIR_SRCS)
aux_source_directory(./detail DETAIL_DIR_SRCS)
add_library(rmuduo ${SRC_DIR_SRCS} ${DETAIL_DIR_SRCS})


add_subdirectory(detail)
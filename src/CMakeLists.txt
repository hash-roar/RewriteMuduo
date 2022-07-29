
set(CMAKE_CXX_STANDARD 17)

find_library(p pthread)
aux_source_directory( ./unix UNIX_SRCS)
aux_source_directory(./detail DETAIL_SRCS)
aux_source_directory(./file File_SRCS)
add_library(rnet ${FILE_SRCS} ${UNIX_SRCS})

target_link_libraries(rnet  p)
target_include_directories(rnet PRIVATE ${PROJECT_SOURCE_DIR}/src)

add_subdirectory(detail)
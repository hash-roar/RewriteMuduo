
set(CMAKE_CXX_STANDARD 17)

# find_package(pthread)
aux_source_directory( ./unix UNIX_SRCS)
aux_source_directory(./log LOG_SRCS)
aux_source_directory(./file FILE_SRCS)
aux_source_directory(./network NETWORK_SRCS)
add_library(rnet ${FILE_SRCS} ${UNIX_SRCS} ${LOG_SRCS}  ${NETWORK_SRCS})

target_link_libraries(rnet  pthread)
target_include_directories(rnet PUBLIC ${PROJECT_SOURCE_DIR}/src)
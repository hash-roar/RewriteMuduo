
aux_source_directory(./ HTTP_SRCS)
message(${HTTP_SRCS})
set(HTTP_SRCS HttpConnection.cc
              HttpServer.cc)
# string(REGEX REPLACE "\.//main.cc" "" OUTPUT ${HTTP_SRCS})
# message(${OUTPUT})
add_library(http ${HTTP_SRCS})
add_executable(http_server main.cc)
target_link_libraries(http_server http)
target_link_libraries(http_server fmt)
target_link_libraries(http_server rnet)
target_include_directories(http PUBLIC ${PROJECT_SOURCE_DIR}/http)
target_include_directories(http PUBLIC ${PROJECT_SOURCE_DIR}/src)


aux_source_directory(./ HTTP_SRCS)
message(${HTTP_SRCS})
string(REGEX REPLACE ".//main.cc" "" OUTPUT ${HTTP_SRCS})
add_library(http ${OUTPUT})
add_executable(http_server main.cc)
target_link_libraries(http_server http)
target_link_libraries(http_server fmt)
target_include_directories(http_server PUBLIC ${PROJECT_SOURCE_DIR}/http)

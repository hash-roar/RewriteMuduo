
set(CXX_STANDARD 17)
set(CXX_STANDARD_REQUIRED true)

aux_source_directory(./ NETWOEK_SRCS)

add_library(network ${NETWOEK_SRCS})
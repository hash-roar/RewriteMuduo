file(GLOB RNET_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*test.cpp")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
# build tests   
foreach(rnet_test_source)
    # Create a human readable name.
    get_filename_component(rnet_test_filename ${bustub_test_source} NAME)
    string(REPLACE ".cc" "" rnet_test_name ${bustub_test_filename})

    #  
    add_executable(${rnet_test_name} EXCLUDE_FROM_ALL ${rnet_test_source})
    target_link_libraries(${rnet_test_name} rnet)
    set_target_properties(${bustub_test_name}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
    COMMAND ${bustub_test_name}
)


endforeach(rnet_test_source)

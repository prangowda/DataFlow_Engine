add_subdirectory(core)
add_subdirectory(algorithms)
add_subdirectory(network)
add_subdirectory(threading)
add_subdirectory(utils)

# Main executable
add_executable(dataflow_app main.cpp)
target_link_libraries(dataflow_app 
    dataflow_core
    dataflow_algorithms
    dataflow_network
    dataflow_threading
    dataflow_utils
)

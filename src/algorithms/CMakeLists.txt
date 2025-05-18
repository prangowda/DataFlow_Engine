add_library(dataflow_algorithms
    ProcessingAlgorithm.cpp
)

target_include_directories(dataflow_algorithms PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dataflow_algorithms 
    dataflow_core
)

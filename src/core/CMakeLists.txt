add_library(dataflow_core
    DataflowEngine.cpp
    DataNode.cpp
    DataPacket.cpp
)

target_include_directories(dataflow_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dataflow_core 
    dataflow_threading
    dataflow_utils
)

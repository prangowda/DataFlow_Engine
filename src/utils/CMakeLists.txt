add_library(dataflow_utils
    ConfigManager.cpp
    Profiler.cpp
)

target_include_directories(dataflow_utils PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dataflow_utils 
    dataflow_threading
)

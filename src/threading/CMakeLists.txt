add_library(dataflow_threading
    ThreadPool.cpp
)

target_include_directories(dataflow_threading PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Link with threading library if needed
find_package(Threads REQUIRED)
target_link_libraries(dataflow_threading 
    Threads::Threads
)

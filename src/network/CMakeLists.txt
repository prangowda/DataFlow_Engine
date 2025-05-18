add_library(dataflow_network
    NetworkManager.cpp
)

target_include_directories(dataflow_network PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(dataflow_network 
    dataflow_core
    dataflow_threading
)

# Add platform-specific socket libraries
if(WIN32)
    target_link_libraries(dataflow_network ws2_32)
endif()

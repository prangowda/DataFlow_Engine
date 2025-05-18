# Getting Started with HighPerformanceDataFlow

This guide will help you quickly get up and running with the HighPerformanceDataFlow framework.

## Prerequisites

Before you begin, ensure you have the following installed:

- C++17 compatible compiler
  - GCC 7.0 or higher
  - Clang 5.0 or higher
  - MSVC 2019 or higher
- CMake 3.12 or higher
- Git (for cloning the repository)

## Building the Project <a name="building"></a>

### Clone the Repository

```bash
git clone https://github.com/yourusername/HighPerformanceDataFlow.git
cd HighPerformanceDataFlow
```

### Configure and Build

```bash
# Create a build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
cmake --build . --config Release
```

### Running Tests

```bash
# From the build directory
ctest -C Release
```

## Basic Usage <a name="basic-usage"></a>

Here's a simple example to get you started with the HighPerformanceDataFlow framework:

```cpp
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "core/DataflowEngine.hpp"
#include "core/DataNode.hpp"
#include "threading/ThreadPool.hpp"
#include "utils/Profiler.hpp"
#include "utils/ConfigManager.hpp"

using namespace dataflow;
using namespace std::chrono_literals;

int main() {
    // Initialize configuration
    auto config = std::make_shared<utils::ConfigManager>("config.json");
    
    // Initialize performance profiler
    auto profiler = std::make_shared<utils::Profiler>(config);
    profiler->start();
    
    // Create thread pool for parallel processing
    auto threadPool = std::make_shared<threading::ThreadPool>(4);
    
    // Create dataflow engine
    auto engine = std::make_shared<core::DataflowEngine>(threadPool, profiler);
    
    // Create source node that generates data
    auto source = engine->createNode<core::DataSourceNode>("data_source");
    source->setDataGenerator([]() {
        // Create a new data packet with random values
        auto packet = std::make_shared<core::DataPacket>();
        
        // Add some data to the packet
        std::vector<double> data(100);
        for (auto& val : data) {
            val = static_cast<double>(rand()) / RAND_MAX;
        }
        
        packet->setData("time_signal", data);
        return packet;
    });
    
    // Create processing node with FFT algorithm
    auto processor = engine->createNode<core::ProcessingNode>("fft_processor");
    processor->setAlgorithm(algorithms::AlgorithmFactory::create("FFT"));
    
    // Create sink node that consumes the processed data
    auto sink = engine->createNode<core::DataSinkNode>("data_sink");
    sink->setDataConsumer([](std::shared_ptr<core::DataPacket> packet) {
        auto spectrum = packet->getData<std::vector<double>>("magnitude_spectrum");
        if (spectrum) {
            std::cout << "Received spectrum with " << spectrum->size() << " points" << std::endl;
            // Process the spectrum data...
        }
    });
    
    // Connect the nodes to form a pipeline
    engine->connect(source, processor);
    engine->connect(processor, sink);
    
    // Start the processing
    engine->start();
    
    // Run for a few seconds
    std::cout << "Processing data for 5 seconds..." << std::endl;
    std::this_thread::sleep_for(5s);
    
    // Stop the engine
    engine->stop();
    
    // Display performance metrics
    profiler->generateReport("performance_report.json");
    
    return 0;
}
```

## Next Steps

- Read the [Architecture Overview](architecture.md) to understand the framework design
- Learn about [Component Reference](components/index.md) for details on each module
- Explore the [Configuration Guide](configuration.md) to customize your setup
- Check out [Advanced Usage](advanced_usage.md) for more complex scenarios

## Troubleshooting

### Common Build Issues

- **CMake cannot find compiler**: Ensure your compiler is in your PATH or specify it explicitly with `-DCMAKE_CXX_COMPILER`
- **Missing dependencies**: Check that your system has all required development libraries installed
- **Compilation errors**: Ensure you're using a C++17 compatible compiler

### Runtime Issues

- **Performance problems**: Review the [Performance Tuning](performance.md) guide
- **Memory usage**: Consider adjusting buffer sizes and cache parameters in your configuration
- **Thread-related issues**: Adjust thread count based on your system capabilities

# HighPerformanceDataFlow

A high-performance, multi-threaded data processing framework written in modern C++17. This system demonstrates advanced C++ concepts including lock-free data structures, parallel processing, RAII principles, and modern design patterns.

## Project Overview

HighPerformanceDataFlow is a flexible data processing pipeline system that enables efficient processing of data streams through a configurable network of processing nodes. The architecture follows a directed graph model where data flows from source nodes through processing nodes to sink nodes.

### Key Features

- **Concurrent Data Processing**: Uses modern C++17 threading model for parallel execution
- **Lock-Free Data Structures**: High-performance inter-thread communication
- **Pluggable Processing Algorithms**: Easily extend with custom algorithms
- **Network Communication Layer**: Built-in asynchronous I/O for distributed processing
- **Performance Metrics and Monitoring**: Real-time performance tracking
- **Type-Safe Interfaces**: Template-based, compile-time type checking

## Architecture

The system is organized into several key components:

### Core Components

- **DataflowEngine**: Central orchestrator that manages the flow of data
- **DataNode**: Base class for all data processing nodes
  - **DataSourceNode**: Generates or receives input data
  - **ProcessingNode**: Transforms data using algorithms
  - **DataSinkNode**: Consumes processed data
- **DataPacket**: Container for data flowing through the system

### Supporting Subsystems

- **Threading**: Thread pool implementation for parallel processing
- **Algorithms**: Pluggable processing strategies (FFT, Image Processing, ML)
- **Network**: Communication layer for distributed processing
- **Utils**: Configuration management and performance monitoring

## Building the Project

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2019+)
- CMake 3.12 or higher
- (Optional) Threading library

### Build Instructions

1. Clone the repository
2. Create a build directory:
   ```
   mkdir build && cd build
   ```
3. Configure with CMake:
   ```
   cmake ..
   ```
4. Build the project:
   ```
   cmake --build .
   ```

## Usage Example

```cpp
#include "core/DataflowEngine.hpp"
#include "core/DataNode.hpp"
#include "threading/ThreadPool.hpp"
#include "utils/Profiler.hpp"
#include "utils/ConfigManager.hpp"

using namespace dataflow;

int main() {
    // Initialize configuration
    auto config = std::make_shared<utils::ConfigManager>("config.json");
    
    // Initialize profiler and thread pool
    auto profiler = std::make_shared<utils::Profiler>(config);
    auto threadPool = std::make_shared<threading::ThreadPool>(8);
    
    // Create dataflow engine
    auto engine = std::make_shared<core::DataflowEngine>(threadPool, profiler);
    
    // Create nodes
    auto source = engine->createNode<core::DataSourceNode>("sensor_data");
    auto processor = engine->createNode<core::ProcessingNode>("processor");
    auto sink = engine->createNode<core::DataSinkNode>("output");
    
    // Connect nodes
    engine->connect(source, processor);
    engine->connect(processor, sink);
    
    // Start the engine
    engine->start();
    
    // Wait for completion or terminate on user input
    std::cin.get();
    
    // Stop the engine
    engine->stop();
    
    return 0;
}
```

## Advanced Features

### Custom Algorithms

You can easily extend the system with custom processing algorithms:

```cpp
class MyCustomAlgorithm : public algorithms::ProcessingAlgorithm {
public:
    std::shared_ptr<core::DataPacket> process(std::shared_ptr<core::DataPacket> input) override {
        // Custom processing logic
        auto output = std::make_shared<core::DataPacket>();
        // Process data...
        return output;
    }
    
    std::string getName() const override { return "MyAlgorithm"; }
    void configure(const std::unordered_map<std::string, std::string>& params) override { /* ... */ }
};

// Register with factory
algorithms::AlgorithmFactory::registerAlgorithm("MyAlgorithm", 
    [] { return std::make_shared<MyCustomAlgorithm>(); });
```

### Performance Monitoring

The built-in profiler allows tracking of various metrics:

```cpp
// Record a performance metric
profiler->recordValue("processing_time", 42.5);

// Use a scoped timer
{
    utils::ScopedTimer timer(*profiler, "operation_duration");
    // Code to measure...
}

// Generate a performance report
profiler->generateReport("performance_report.json");
```

## Design Patterns Used

- **Factory Pattern**: Algorithm creation
- **Observer Pattern**: Configuration and metrics monitoring
- **Strategy Pattern**: Pluggable processing algorithms
- **Composite Pattern**: Node connections
- **RAII**: Resource management

## Performance Considerations

- Lock-free data structures minimize contention
- Work-stealing thread pool for balanced load distribution
- Zero-copy data transfer where possible
- Type-erased containers with compile-time type safety

## License

This project is available under the MIT License - see the LICENSE file for details.

## Interview Application

This project demonstrates several advanced C++ concepts that are valuable in technical interviews:

1. **Modern C++ Features**: Smart pointers, type traits, lambda expressions
2. **Concurrent Programming**: Thread-safe design, atomics, condition variables
3. **Generic Programming**: Templates, type erasure, SFINAE
4. **Memory Management**: RAII, move semantics, custom memory handling
5. **Software Architecture**: Modular design, interface-based programming
6. **Performance Optimization**: Lock-free algorithms, efficient data structures

---

This project is intended as a showcase of advanced C++ programming techniques and not for production use without further development and testing.

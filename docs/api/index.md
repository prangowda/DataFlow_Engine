# API Reference

This section provides detailed documentation for the HighPerformanceDataFlow API. Each module's classes, methods, and properties are documented with usage examples.

## Modules

- [Core](core.md) - Core data processing components
- [Algorithms](algorithms.md) - Data processing algorithms
- [Threading](threading.md) - Thread pool and concurrency utilities
- [Network](network.md) - Network communication components
- [Utils](utils.md) - Configuration and profiling utilities

## Namespace Structure

All HighPerformanceDataFlow components are organized under the `dataflow` namespace, with specific modules in sub-namespaces:

```cpp
namespace dataflow {
    // Top-level components

    namespace core {
        // Core processing components
    }

    namespace algorithms {
        // Processing algorithms
    }

    namespace threading {
        // Thread pool and concurrency
    }

    namespace network {
        // Network communication
    }

    namespace utils {
        // Utility components
    }
}
```

## Common Patterns

Throughout the API, you'll encounter these common patterns:

### Error Handling

Most methods use exceptions for error handling:

```cpp
try {
    // API calls
} catch (const std::runtime_error& e) {
    // Handle runtime errors
} catch (const std::invalid_argument& e) {
    // Handle invalid arguments
} catch (const std::exception& e) {
    // Handle all other exceptions
}
```

### Resource Management

The API uses RAII principles with smart pointers:

```cpp
// Preferred usage
auto engine = std::make_shared<core::DataflowEngine>(threadPool, profiler);

// Avoid raw pointers
// DataflowEngine* engine = new DataflowEngine(); // Not recommended
```

### Thread Safety

Components are thread-safe unless explicitly documented otherwise:

```cpp
// This is safe to call from multiple threads
engine->scheduleTask([](){ /* task code */ });

// Thread-safe data packet modification
packet->setData("key", value);
```

## Version Compatibility

This documentation is for version 1.0.0 of the HighPerformanceDataFlow framework.

Future API changes will be documented in the [Changelog](../changelog.md).

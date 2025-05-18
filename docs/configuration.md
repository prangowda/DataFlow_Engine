# Configuration Guide

The HighPerformanceDataFlow framework provides extensive configuration options to customize its behavior for various use cases. This guide explains all available configuration options and how to use them effectively.

## Configuration Format

The system uses JSON for configuration. The default configuration file is `config.json` in the project root directory, but you can specify a different file when initializing the `ConfigManager`.

## Basic Configuration Example

```json
{
  "threadCount": 8,
  "host": "localhost",
  "port": 8080,
  "algorithm": "FFT",
  "enable_background_collection": true
}
```

## Loading Configuration

Load configuration when initializing the `ConfigManager`:

```cpp
// Load default config.json
auto config = std::make_shared<utils::ConfigManager>();

// Or specify a custom path
auto config = std::make_shared<utils::ConfigManager>("path/to/custom_config.json");
```

You can also modify configuration at runtime:

```cpp
// Set a new value
config->setValue<int>("threadCount", 16);

// Save the current configuration
config->saveToFile("updated_config.json");
```

## Configuration Options

### System Configuration

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `threadCount` | Integer | Hardware cores | Number of worker threads in the thread pool |
| `enable_background_collection` | Boolean | `false` | Enable background performance metrics collection |
| `background_collection_interval_ms` | Integer | `1000` | Interval for background metrics collection in milliseconds |
| `max_metric_history` | Integer | `1000` | Maximum number of historical values to store per metric |

### Network Configuration

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `host` | String | `"localhost"` | Hostname or IP address for the network server |
| `port` | Integer | `8080` | Port number for the network server |
| `timeout_ms` | Integer | `5000` | Network operation timeout in milliseconds |
| `buffer_size` | Integer | `8192` | Size of network buffer in bytes |
| `max_connections` | Integer | `100` | Maximum number of simultaneous connections |

### Algorithm Configuration

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `algorithm` | String | `"FFT"` | Default algorithm to use |

#### FFT Algorithm

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `algorithms.FFT.window_size` | Integer | `1024` | FFT window size |
| `algorithms.FFT.use_hanning` | Boolean | `true` | Whether to apply Hanning window |

#### Image Processing Algorithm

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `algorithms.ImageProcessing.filter_type` | Integer | `0` | Filter type (0=Blur, 1=Sharpen, 2=EdgeDetection) |
| `algorithms.ImageProcessing.kernel_size` | Integer | `3` | Size of kernel for filtering |

#### Machine Learning Algorithm

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `algorithms.MachineLearning.model_path` | String | `"models/default_model.bin"` | Path to the ML model file |
| `algorithms.MachineLearning.confidence_threshold` | Float | `0.75` | Confidence threshold for predictions |

### Performance Tuning

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `performance.batch_size` | Integer | `64` | Batch size for data processing |
| `performance.prefetch_count` | Integer | `2` | Number of batches to prefetch |
| `performance.cache_size_mb` | Integer | `256` | Size of data cache in megabytes |

### Logging Configuration

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `logging.level` | String | `"info"` | Log level (debug, info, warning, error) |
| `logging.file` | String | `"dataflow.log"` | Path to log file |
| `logging.console` | Boolean | `true` | Whether to output logs to console |

## Environment Variables

The system also supports overriding configuration options with environment variables. The format is:

```
DATAFLOW_UPPERCASE_OPTION_NAME
```

For example:
- `DATAFLOW_THREAD_COUNT` overrides `threadCount`
- `DATAFLOW_HOST` overrides `host`

Environment variables take precedence over values in the configuration file.

## Dynamic Configuration

The configuration can be updated at runtime using the `ConfigManager` API:

```cpp
// Update a configuration value
config->setValue<int>("threadCount", 16);

// Check if a key exists
if (config->hasKey("host")) {
    // Use the value
}

// Get a value with a default fallback
int port = config->getValue<int>("port", 9000);
```

### Configuration Change Callbacks

You can register callbacks to be notified when configuration values change:

```cpp
config->registerObserver("threadCount", [](const std::string& key) {
    std::cout << "Thread count changed!" << std::endl;
});
```

## Multiple Configurations

For complex applications, you may want to use multiple configuration files:

```cpp
// Load the base configuration
auto config = std::make_shared<utils::ConfigManager>("base_config.json");

// Load and merge environment-specific configuration
if (environment == "production") {
    config->loadFromFile("production_config.json");
} else if (environment == "development") {
    config->loadFromFile("development_config.json");
}
```

## Best Practices

1. **Default Values**: Always provide sensible defaults in your code for all configuration options
2. **Validation**: Validate configuration values before using them
3. **Documentation**: Document all configuration options used by your components
4. **Isolation**: Keep configuration for different components separate
5. **Version Control**: Track configuration files in version control, but use environment variables for secrets

## Troubleshooting

### Common Issues

- **Configuration file not found**: Ensure the file path is correct and accessible
- **Invalid JSON**: Validate your JSON syntax with a JSON linter
- **Type Mismatches**: Ensure values are of the expected type
- **Missing Values**: Check for required values and provide defaults for optional ones

### Configuration Debugging

Set the `DATAFLOW_DEBUG_CONFIG=1` environment variable to enable verbose logging of configuration loading and access.

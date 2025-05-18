# Core API Reference

The Core module contains the fundamental components of the HighPerformanceDataFlow framework.

## DataflowEngine

`DataflowEngine` is the central orchestrator that manages data flow between nodes.

### Class Definition

```cpp
namespace dataflow {
namespace core {

class DataflowEngine : public std::enable_shared_from_this<DataflowEngine> {
public:
    DataflowEngine(std::shared_ptr<threading::ThreadPool> threadPool,
                   std::shared_ptr<utils::Profiler> profiler);
    
    ~DataflowEngine();
    
    template<typename NodeType>
    std::shared_ptr<NodeType> createNode(const std::string& name);
    
    bool connect(std::shared_ptr<DataNode> source, std::shared_ptr<DataNode> target);
    bool disconnect(std::shared_ptr<DataNode> source, std::shared_ptr<DataNode> target);
    std::shared_ptr<DataNode> getNode(const std::string& name);
    void start();
    void stop();
    void scheduleTask(std::function<void()> task);
    void recordMetric(const std::string& name, double value);
    
    // ...
};

} // namespace core
} // namespace dataflow
```

### Constructor

```cpp
DataflowEngine(std::shared_ptr<threading::ThreadPool> threadPool,
               std::shared_ptr<utils::Profiler> profiler);
```

Creates a new DataflowEngine with the specified thread pool and profiler.

**Parameters:**
- `threadPool`: A shared pointer to a ThreadPool for executing tasks
- `profiler`: A shared pointer to a Profiler for performance monitoring

**Throws:**
- `std::invalid_argument`: If threadPool or profiler is null

### Methods

#### createNode

```cpp
template<typename NodeType>
std::shared_ptr<NodeType> createNode(const std::string& name);
```

Creates a new node of the specified type.

**Template Parameters:**
- `NodeType`: The type of node to create (must inherit from DataNode)

**Parameters:**
- `name`: A unique identifier for the node

**Returns:**
- A shared pointer to the newly created node

**Example:**
```cpp
auto source = engine->createNode<DataSourceNode>("sensor_data");
```

#### connect

```cpp
bool connect(std::shared_ptr<DataNode> source, std::shared_ptr<DataNode> target);
```

Connects two nodes, allowing data to flow from the source to the target.

**Parameters:**
- `source`: The node that produces data
- `target`: The node that consumes data

**Returns:**
- `true` if the connection was successful, `false` if it already exists or parameters are invalid

**Example:**
```cpp
engine->connect(source, processor);
```

#### disconnect

```cpp
bool disconnect(std::shared_ptr<DataNode> source, std::shared_ptr<DataNode> target);
```

Disconnects two previously connected nodes.

**Parameters:**
- `source`: The source node
- `target`: The target node

**Returns:**
- `true` if the disconnection was successful, `false` if the connection didn't exist

#### getNode

```cpp
std::shared_ptr<DataNode> getNode(const std::string& name);
```

Retrieves a node by its name.

**Parameters:**
- `name`: The name of the node to retrieve

**Returns:**
- A shared pointer to the node, or `nullptr` if not found

#### start

```cpp
void start();
```

Starts the dataflow engine, initiating data processing.

**Thread Safety:**
- Thread-safe, can be called from any thread

#### stop

```cpp
void stop();
```

Stops the dataflow engine, halting all processing.

**Thread Safety:**
- Thread-safe, can be called from any thread

#### scheduleTask

```cpp
void scheduleTask(std::function<void()> task);
```

Schedules a task to be executed by the thread pool.

**Parameters:**
- `task`: The function to execute

**Throws:**
- `std::runtime_error`: If the engine is not running

#### recordMetric

```cpp
void recordMetric(const std::string& name, double value);
```

Records a performance metric using the profiler.

**Parameters:**
- `name`: The name of the metric
- `value`: The value to record

## DataNode

`DataNode` is the abstract base class for all nodes in the dataflow system.

### Class Definition

```cpp
namespace dataflow {
namespace core {

class DataNode : public std::enable_shared_from_this<DataNode> {
public:
    DataNode(const std::string& name, std::shared_ptr<DataflowEngine> engine);
    virtual ~DataNode() = default;
    
    std::string getName() const;
    virtual bool processData(std::shared_ptr<DataPacket> packet) = 0;
    void sendToOutputs(std::shared_ptr<DataPacket> packet);
    bool addOutput(std::shared_ptr<DataNode> node);
    bool removeOutput(std::shared_ptr<DataNode> node);
    virtual void start() = 0;
    virtual void stop() = 0;
    bool isRunning() const;
    
    // ...
};

} // namespace core
} // namespace dataflow
```

### Constructor

```cpp
DataNode(const std::string& name, std::shared_ptr<DataflowEngine> engine);
```

Creates a new DataNode with the specified name and engine.

**Parameters:**
- `name`: A unique identifier for the node
- `engine`: A shared pointer to the DataflowEngine that manages this node

**Throws:**
- `std::invalid_argument`: If engine is null

### Methods

#### getName

```cpp
std::string getName() const;
```

Gets the name of this node.

**Returns:**
- The node's name

#### processData

```cpp
virtual bool processData(std::shared_ptr<DataPacket> packet) = 0;
```

Processes an incoming data packet. This is a pure virtual method that must be implemented by derived classes.

**Parameters:**
- `packet`: The data packet to process

**Returns:**
- `true` if processing was successful, `false` otherwise

#### sendToOutputs

```cpp
void sendToOutputs(std::shared_ptr<DataPacket> packet);
```

Sends a data packet to all connected output nodes.

**Parameters:**
- `packet`: The data packet to send

**Thread Safety:**
- Thread-safe, can be called from any thread

#### addOutput

```cpp
bool addOutput(std::shared_ptr<DataNode> node);
```

Adds a node as an output target.

**Parameters:**
- `node`: The target node

**Returns:**
- `true` if the connection was successful, `false` otherwise

#### removeOutput

```cpp
bool removeOutput(std::shared_ptr<DataNode> node);
```

Removes a node from output targets.

**Parameters:**
- `node`: The target node to remove

**Returns:**
- `true` if disconnection was successful, `false` otherwise

#### start

```cpp
virtual void start() = 0;
```

Starts the node's processing. This is a pure virtual method that must be implemented by derived classes.

#### stop

```cpp
virtual void stop() = 0;
```

Stops the node's processing. This is a pure virtual method that must be implemented by derived classes.

#### isRunning

```cpp
bool isRunning() const;
```

Checks if the node is currently running.

**Returns:**
- `true` if running, `false` if stopped

## DataSourceNode

`DataSourceNode` is a specialized node that generates or receives data.

### Class Definition

```cpp
namespace dataflow {
namespace core {

class DataSourceNode : public DataNode {
public:
    using DataNode::DataNode;
    
    bool processData(std::shared_ptr<DataPacket> packet) override;
    void start() override;
    void stop() override;
    
    void setDataGenerator(std::function<std::shared_ptr<DataPacket>()> generator);
    
    // ...
};

} // namespace core
} // namespace dataflow
```

### Methods

#### processData

```cpp
bool processData(std::shared_ptr<DataPacket> packet) override;
```

Processes an incoming data packet. For source nodes, this typically forwards external data.

**Parameters:**
- `packet`: The data packet to process

**Returns:**
- `true` if forwarding was successful, `false` otherwise

#### start

```cpp
void start() override;
```

Starts the data generation process.

#### stop

```cpp
void stop() override;
```

Stops the data generation process.

#### setDataGenerator

```cpp
void setDataGenerator(std::function<std::shared_ptr<DataPacket>()> generator);
```

Sets a function that generates data packets.

**Parameters:**
- `generator`: A function that returns data packets

**Example:**
```cpp
source->setDataGenerator([]() {
    auto packet = std::make_shared<DataPacket>();
    // Fill packet with data
    return packet;
});
```

## ProcessingNode

`ProcessingNode` is a specialized node that processes and transforms data.

### Class Definition

```cpp
namespace dataflow {
namespace core {

class ProcessingNode : public DataNode {
public:
    using DataNode::DataNode;
    
    bool processData(std::shared_ptr<DataPacket> packet) override;
    void start() override;
    void stop() override;
    
    void setAlgorithm(std::shared_ptr<algorithms::ProcessingAlgorithm> algorithm);
    
    // ...
};

} // namespace core
} // namespace dataflow
```

### Methods

#### processData

```cpp
bool processData(std::shared_ptr<DataPacket> packet) override;
```

Queues a data packet for processing.

**Parameters:**
- `packet`: The data packet to process

**Returns:**
- `true` if the packet was queued successfully, `false` otherwise

#### start

```cpp
void start() override;
```

Starts the processing thread.

#### stop

```cpp
void stop() override;
```

Stops the processing thread.

#### setAlgorithm

```cpp
void setAlgorithm(std::shared_ptr<algorithms::ProcessingAlgorithm> algorithm);
```

Sets the algorithm to use for data processing.

**Parameters:**
- `algorithm`: The algorithm to use

**Example:**
```cpp
processor->setAlgorithm(algorithms::AlgorithmFactory::create("FFT"));
```

## DataSinkNode

`DataSinkNode` is a specialized node that receives processed data and performs terminal operations.

### Class Definition

```cpp
namespace dataflow {
namespace core {

class DataSinkNode : public DataNode {
public:
    using DataNode::DataNode;
    
    bool processData(std::shared_ptr<DataPacket> packet) override;
    void start() override;
    void stop() override;
    
    void setDataConsumer(std::function<void(std::shared_ptr<DataPacket>)> consumer);
    
    // ...
};

} // namespace core
} // namespace dataflow
```

### Methods

#### processData

```cpp
bool processData(std::shared_ptr<DataPacket> packet) override;
```

Processes a data packet by passing it to the consumer function.

**Parameters:**
- `packet`: The data packet to process

**Returns:**
- `true` if processing was successful, `false` otherwise

#### start

```cpp
void start() override;
```

Starts the sink node.

#### stop

```cpp
void stop() override;
```

Stops the sink node.

#### setDataConsumer

```cpp
void setDataConsumer(std::function<void(std::shared_ptr<DataPacket>)> consumer);
```

Sets a function that consumes data packets.

**Parameters:**
- `consumer`: A function that processes data packets

**Example:**
```cpp
sink->setDataConsumer([](std::shared_ptr<DataPacket> packet) {
    // Process the final data
    auto result = packet->getData<std::vector<double>>("result");
    // Use the result...
});
```

## DataPacket

`DataPacket` is a container for data that flows through the system.

### Class Definition

```cpp
namespace dataflow {
namespace core {

class DataPacket {
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    
    explicit DataPacket(std::string id = "");
    
    const std::string& getId() const;
    
    template<typename T>
    void setData(const std::string& key, T&& value);
    
    template<typename T>
    std::optional<T> getData(const std::string& key) const;
    
    bool hasKey(const std::string& key) const;
    TimePoint getCreationTime() const;
    TimePoint getLastModifiedTime() const;
    std::vector<std::string> getKeys() const;
    std::shared_ptr<DataPacket> clone() const;
    
    static std::string generateUniqueId();
    
    // ...
};

} // namespace core
} // namespace dataflow
```

### Constructor

```cpp
explicit DataPacket(std::string id = "");
```

Creates a new DataPacket with the specified ID or generates a unique one.

**Parameters:**
- `id`: Optional unique identifier for this packet

### Methods

#### getId

```cpp
const std::string& getId() const;
```

Gets the unique ID of this packet.

**Returns:**
- The packet ID

#### setData

```cpp
template<typename T>
void setData(const std::string& key, T&& value);
```

Sets data of a specific type in the packet.

**Template Parameters:**
- `T`: Type of data

**Parameters:**
- `key`: Key for accessing this data
- `value`: The data value

**Thread Safety:**
- Thread-safe, can be called from any thread

**Example:**
```cpp
packet->setData("temperature", 25.5);
packet->setData("timestamp", std::chrono::system_clock::now());
packet->setData("data", std::vector<double>{1.0, 2.0, 3.0});
```

#### getData

```cpp
template<typename T>
std::optional<T> getData(const std::string& key) const;
```

Gets data of a specific type from the packet.

**Template Parameters:**
- `T`: Expected type of the data

**Parameters:**
- `key`: Key to look up

**Returns:**
- An optional containing the data value, or nullopt if not found or wrong type

**Example:**
```cpp
auto temperature = packet->getData<double>("temperature");
if (temperature) {
    std::cout << "Temperature: " << *temperature << "°C" << std::endl;
}
```

#### hasKey

```cpp
bool hasKey(const std::string& key) const;
```

Checks if the packet contains data with the specified key.

**Parameters:**
- `key`: Key to check

**Returns:**
- `true` if the key exists, `false` otherwise

#### getCreationTime

```cpp
TimePoint getCreationTime() const;
```

Gets the time the packet was created.

**Returns:**
- Creation timestamp

#### getLastModifiedTime

```cpp
TimePoint getLastModifiedTime() const;
```

Gets the time the packet was last modified.

**Returns:**
- Last modification timestamp

#### getKeys

```cpp
std::vector<std::string> getKeys() const;
```

Gets all keys in the packet.

**Returns:**
- Vector of all keys

#### clone

```cpp
std::shared_ptr<DataPacket> clone() const;
```

Creates a deep copy of this packet.

**Returns:**
- A new packet with copies of all data

#### generateUniqueId

```cpp
static std::string generateUniqueId();
```

Generates a unique ID for a packet.

**Returns:**
- A unique string ID

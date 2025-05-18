#pragma once

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <optional>
#include <atomic>
#include <functional>

#include "DataPacket.hpp"

namespace dataflow {

// Forward declaration
namespace core {
    class DataflowEngine;
}

namespace core {

/**
 * @class DataNode
 * @brief Base class for all nodes in the dataflow system
 * 
 * This abstract class defines the interface and common functionality for
 * all types of nodes in the dataflow processing system. Nodes can be
 * sources of data, processing nodes, or data sinks.
 */
class DataNode : public std::enable_shared_from_this<DataNode> {
public:
    /**
     * @brief Constructor
     * @param name Unique identifier for this node
     * @param engine Reference to the parent DataflowEngine
     */
    DataNode(const std::string& name, std::shared_ptr<DataflowEngine> engine);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~DataNode() = default;
    
    /**
     * @brief Get the name of this node
     * @return The node's name
     */
    std::string getName() const { return name_; }
    
    /**
     * @brief Process an incoming data packet
     * @param packet The data packet to process
     * @return true if processing was successful, false otherwise
     */
    virtual bool processData(std::shared_ptr<DataPacket> packet) = 0;
    
    /**
     * @brief Send a data packet to all connected output nodes
     * @param packet The data packet to send
     */
    void sendToOutputs(std::shared_ptr<DataPacket> packet);
    
    /**
     * @brief Add a node as an output target
     * @param node The target node
     * @return true if the connection was successful, false otherwise
     */
    bool addOutput(std::shared_ptr<DataNode> node);
    
    /**
     * @brief Remove a node from output targets
     * @param node The target node to remove
     * @return true if disconnection was successful, false otherwise
     */
    bool removeOutput(std::shared_ptr<DataNode> node);
    
    /**
     * @brief Start the node's processing
     */
    virtual void start() = 0;
    
    /**
     * @brief Stop the node's processing
     */
    virtual void stop() = 0;
    
    /**
     * @brief Check if the node is currently running
     * @return true if running, false if stopped
     */
    bool isRunning() const { return running_; }

protected:
    std::string name_;
    std::shared_ptr<DataflowEngine> engine_;
    std::vector<std::weak_ptr<DataNode>> outputs_;
    std::mutex outputsMutex_;
    std::atomic<bool> running_{false};
};

/**
 * @class DataSourceNode
 * @brief A node that generates data packets
 */
class DataSourceNode : public DataNode {
public:
    using DataNode::DataNode;
    
    bool processData(std::shared_ptr<DataPacket> packet) override;
    void start() override;
    void stop() override;
    
    /**
     * @brief Set the data generation function
     * @param generator Function that generates data packets
     */
    void setDataGenerator(std::function<std::shared_ptr<DataPacket>()> generator);

private:
    std::function<std::shared_ptr<DataPacket>()> generator_;
    std::atomic<bool> generatorActive_{false};
    std::mutex generatorMutex_;
    std::condition_variable generatorCV_;
    std::thread generatorThread_;
    
    void generatorLoop();
};

/**
 * @class ProcessingNode
 * @brief A node that processes data and transforms it
 */
class ProcessingNode : public DataNode {
public:
    using DataNode::DataNode;
    
    bool processData(std::shared_ptr<DataPacket> packet) override;
    void start() override;
    void stop() override;
    
    /**
     * @brief Set the processing algorithm
     * @param algorithm The algorithm to use for data processing
     */
    void setAlgorithm(std::shared_ptr<class algorithms::ProcessingAlgorithm> algorithm);

private:
    std::shared_ptr<class algorithms::ProcessingAlgorithm> algorithm_;
    std::deque<std::shared_ptr<DataPacket>> inputQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;
    std::thread processingThread_;
    std::atomic<bool> processingActive_{false};
    
    void processingLoop();
};

/**
 * @class DataSinkNode
 * @brief A node that receives data and performs a terminal operation
 */
class DataSinkNode : public DataNode {
public:
    using DataNode::DataNode;
    
    bool processData(std::shared_ptr<DataPacket> packet) override;
    void start() override;
    void stop() override;
    
    /**
     * @brief Set the data consumer function
     * @param consumer Function that consumes data packets
     */
    void setDataConsumer(std::function<void(std::shared_ptr<DataPacket>)> consumer);

private:
    std::function<void(std::shared_ptr<DataPacket>)> consumer_;
};

} // namespace core
} // namespace dataflow

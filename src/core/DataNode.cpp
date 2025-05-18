#include "../../include/core/DataNode.hpp"
#include "../../include/core/DataflowEngine.hpp"
#include "../../include/algorithms/ProcessingAlgorithm.hpp"

#include <iostream>
#include <algorithm>

namespace dataflow {
namespace core {

// Base DataNode implementation
DataNode::DataNode(const std::string& name, std::shared_ptr<DataflowEngine> engine)
    : name_(name), engine_(engine) {
    
    if (!engine_) {
        throw std::invalid_argument("DataflowEngine cannot be null");
    }
}

void DataNode::sendToOutputs(std::shared_ptr<DataPacket> packet) {
    if (!running_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(outputsMutex_);
    
    // Clean up expired weak pointers first
    outputs_.erase(
        std::remove_if(outputs_.begin(), outputs_.end(),
            [](const std::weak_ptr<DataNode>& weakNode) {
                return weakNode.expired();
            }
        ),
        outputs_.end()
    );
    
    // Send data to all outputs
    for (const auto& weakNode : outputs_) {
        if (auto node = weakNode.lock()) {
            // Use DataflowEngine to schedule async processing
            engine_->scheduleTask([node, packet]() {
                node->processData(packet);
            });
        }
    }
}

bool DataNode::addOutput(std::shared_ptr<DataNode> node) {
    if (!node) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(outputsMutex_);
    
    // Check if already exists
    for (const auto& weakNode : outputs_) {
        if (auto existingNode = weakNode.lock()) {
            if (existingNode == node) {
                return false; // Already connected
            }
        }
    }
    
    // Add new output
    outputs_.push_back(node);
    return true;
}

bool DataNode::removeOutput(std::shared_ptr<DataNode> node) {
    if (!node) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(outputsMutex_);
    
    // Find and remove the node
    size_t initialSize = outputs_.size();
    
    outputs_.erase(
        std::remove_if(outputs_.begin(), outputs_.end(),
            [&node](const std::weak_ptr<DataNode>& weakNode) {
                auto ptr = weakNode.lock();
                return !ptr || ptr == node;
            }
        ),
        outputs_.end()
    );
    
    // Return true if at least one was removed
    return outputs_.size() < initialSize;
}

// DataSourceNode implementation
bool DataSourceNode::processData(std::shared_ptr<DataPacket> packet) {
    // Source nodes typically don't process incoming data
    // but can be implemented to forward external data
    if (packet) {
        sendToOutputs(packet);
        return true;
    }
    return false;
}

void DataSourceNode::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return; // Already running
    }
    
    // Start the generator thread
    bool expectedGenerator = false;
    if (generator_ && !generatorActive_.compare_exchange_strong(expectedGenerator, true)) {
        // Start the generator thread
        generatorThread_ = std::thread(&DataSourceNode::generatorLoop, this);
    }
}

void DataSourceNode::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return; // Already stopped
    }
    
    // Stop the generator
    if (generatorActive_) {
        generatorActive_ = false;
        generatorCV_.notify_all();
        
        if (generatorThread_.joinable()) {
            generatorThread_.join();
        }
    }
}

void DataSourceNode::setDataGenerator(std::function<std::shared_ptr<DataPacket>()> generator) {
    std::lock_guard<std::mutex> lock(generatorMutex_);
    generator_ = std::move(generator);
}

void DataSourceNode::generatorLoop() {
    while (generatorActive_ && running_) {
        try {
            // Generate a data packet
            if (generator_) {
                auto packet = generator_();
                if (packet) {
                    sendToOutputs(packet);
                }
            }
            
            // Sleep for a short time between generations
            std::unique_lock<std::mutex> lock(generatorMutex_);
            generatorCV_.wait_for(lock, std::chrono::milliseconds(10), 
                                 [this] { return !generatorActive_ || !running_; });
        }
        catch (const std::exception& e) {
            std::cerr << "Error in DataSourceNode generator: " << e.what() << std::endl;
        }
    }
}

// ProcessingNode implementation
bool ProcessingNode::processData(std::shared_ptr<DataPacket> packet) {
    if (!packet) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        inputQueue_.push_back(packet);
    }
    
    queueCV_.notify_one();
    return true;
}

void ProcessingNode::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return; // Already running
    }
    
    // Start the processing thread
    bool expectedProcessing = false;
    if (!processingActive_.compare_exchange_strong(expectedProcessing, true)) {
        return; // Already started
    }
    
    processingThread_ = std::thread(&ProcessingNode::processingLoop, this);
}

void ProcessingNode::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return; // Already stopped
    }
    
    // Stop the processing
    if (processingActive_) {
        processingActive_ = false;
        queueCV_.notify_all();
        
        if (processingThread_.joinable()) {
            processingThread_.join();
        }
    }
}

void ProcessingNode::setAlgorithm(std::shared_ptr<algorithms::ProcessingAlgorithm> algorithm) {
    algorithm_ = algorithm;
}

void ProcessingNode::processingLoop() {
    while (processingActive_ && running_) {
        std::shared_ptr<DataPacket> packet;
        
        // Wait for data to process
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCV_.wait(lock, [this] { 
                return !inputQueue_.empty() || !processingActive_ || !running_; 
            });
            
            if (!processingActive_ || !running_) {
                break;
            }
            
            if (inputQueue_.empty()) {
                continue;
            }
            
            packet = inputQueue_.front();
            inputQueue_.pop_front();
        }
        
        try {
            // Process the data
            if (algorithm_ && packet) {
                auto startTime = std::chrono::high_resolution_clock::now();
                
                auto result = algorithm_->process(packet);
                
                // Record processing time
                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
                engine_->recordMetric("processing_time_" + name_, duration.count());
                
                // Send result to outputs
                if (result) {
                    sendToOutputs(result);
                }
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error in ProcessingNode: " << e.what() << std::endl;
        }
    }
}

// DataSinkNode implementation
bool DataSinkNode::processData(std::shared_ptr<DataPacket> packet) {
    if (!packet || !running_) {
        return false;
    }
    
    try {
        // Process the data
        if (consumer_) {
            consumer_(packet);
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error in DataSinkNode: " << e.what() << std::endl;
        return false;
    }
}

void DataSinkNode::start() {
    bool expected = false;
    running_.compare_exchange_strong(expected, true);
}

void DataSinkNode::stop() {
    bool expected = true;
    running_.compare_exchange_strong(expected, false);
}

void DataSinkNode::setDataConsumer(std::function<void(std::shared_ptr<DataPacket>)> consumer) {
    consumer_ = std::move(consumer);
}

} // namespace core
} // namespace dataflow

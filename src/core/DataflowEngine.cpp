#include "../../include/core/DataflowEngine.hpp"
#include "../../include/core/DataNode.hpp"
#include "../../include/threading/ThreadPool.hpp"
#include "../../include/utils/Profiler.hpp"

#include <iostream>
#include <algorithm>

namespace dataflow {
namespace core {

DataflowEngine::DataflowEngine(std::shared_ptr<threading::ThreadPool> threadPool,
                             std::shared_ptr<utils::Profiler> profiler)
    : threadPool_(threadPool), profiler_(profiler) {
    
    if (!threadPool_) {
        throw std::invalid_argument("ThreadPool cannot be null");
    }
    
    if (!profiler_) {
        throw std::invalid_argument("Profiler cannot be null");
    }
}

DataflowEngine::~DataflowEngine() {
    stop();
}

bool DataflowEngine::connect(std::shared_ptr<DataNode> source, std::shared_ptr<DataNode> target) {
    if (!source || !target) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    
    // Check if connection already exists
    auto it = std::find_if(connections_.begin(), connections_.end(),
        [&](const auto& conn) {
            return conn.first == source && conn.second == target;
        });
    
    if (it != connections_.end()) {
        return false; // Connection already exists
    }
    
    // Add the connection
    connections_.emplace_back(source, target);
    
    // Update the output node in the source
    source->addOutput(target);
    
    return true;
}

bool DataflowEngine::disconnect(std::shared_ptr<DataNode> source, std::shared_ptr<DataNode> target) {
    if (!source || !target) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    
    // Find and remove the connection
    auto it = std::find_if(connections_.begin(), connections_.end(),
        [&](const auto& conn) {
            return conn.first == source && conn.second == target;
        });
    
    if (it == connections_.end()) {
        return false; // Connection doesn't exist
    }
    
    connections_.erase(it);
    
    // Update the output node in the source
    source->removeOutput(target);
    
    return true;
}

std::shared_ptr<DataNode> DataflowEngine::getNode(const std::string& name) {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    
    auto it = nodes_.find(name);
    if (it != nodes_.end()) {
        return it->second;
    }
    
    return nullptr;
}

void DataflowEngine::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return; // Already running
    }
    
    // Start all nodes
    std::lock_guard<std::mutex> lock(nodesMutex_);
    for (const auto& pair : nodes_) {
        pair.second->start();
    }
}

void DataflowEngine::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return; // Already stopped
    }
    
    // Stop all nodes
    std::lock_guard<std::mutex> lock(nodesMutex_);
    for (const auto& pair : nodes_) {
        pair.second->stop();
    }
}

void DataflowEngine::scheduleTask(std::function<void()> task) {
    if (!running_) {
        throw std::runtime_error("Cannot schedule tasks when engine is not running");
    }
    
    threadPool_->enqueue(std::move(task));
}

void DataflowEngine::recordMetric(const std::string& name, double value) {
    profiler_->recordValue(name, value);
}

} // namespace core
} // namespace dataflow

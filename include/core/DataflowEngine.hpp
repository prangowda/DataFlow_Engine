#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <atomic>
#include <mutex>
#include <functional>

#include "DataNode.hpp"
#include "../threading/ThreadPool.hpp"
#include "../utils/Profiler.hpp"

namespace dataflow {
namespace core {

/**
 * @class DataflowEngine
 * @brief Core class that manages the dataflow processing system
 */
class DataflowEngine : public std::enable_shared_from_this<DataflowEngine> {
public:
    DataflowEngine(std::shared_ptr<threading::ThreadPool> threadPool,
                   std::shared_ptr<utils::Profiler> profiler);
    
    ~DataflowEngine();
    
    template<typename NodeType>
    std::shared_ptr<NodeType> createNode(const std::string& name) {
        auto node = std::make_shared<NodeType>(name, shared_from_this());
        
        std::lock_guard<std::mutex> lock(nodesMutex_);
        nodes_[name] = node;
        
        return node;
    }
    
    bool connect(std::shared_ptr<DataNode> source, std::shared_ptr<DataNode> target);
    bool disconnect(std::shared_ptr<DataNode> source, std::shared_ptr<DataNode> target);
    std::shared_ptr<DataNode> getNode(const std::string& name);
    void start();
    void stop();
    void scheduleTask(std::function<void()> task);
    void recordMetric(const std::string& name, double value);

private:
    std::shared_ptr<threading::ThreadPool> threadPool_;
    std::shared_ptr<utils::Profiler> profiler_;
    
    std::unordered_map<std::string, std::shared_ptr<DataNode>> nodes_;
    std::mutex nodesMutex_;
    
    std::vector<std::pair<std::shared_ptr<DataNode>, std::shared_ptr<DataNode>>> connections_;
    std::mutex connectionsMutex_;
    
    std::atomic<bool> running_{false};
};

} // namespace core
} // namespace dataflow

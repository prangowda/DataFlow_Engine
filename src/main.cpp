#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "core/DataflowEngine.hpp"
#include "core/DataNode.hpp"
#include "threading/ThreadPool.hpp"
#include "network/NetworkManager.hpp"
#include "algorithms/ProcessingAlgorithm.hpp"
#include "utils/Profiler.hpp"
#include "utils/ConfigManager.hpp"

using namespace dataflow;
using namespace std::chrono_literals;

int main(int argc, char* argv[]) {
    std::cout << "=======================================================" << std::endl;
    std::cout << "  High Performance DataFlow System - Starting Up" << std::endl;
    std::cout << "=======================================================" << std::endl;
    
    try {
        // Initialize configuration
        auto config = std::make_shared<utils::ConfigManager>("config.json");
        
        // Initialize profiler for performance metrics
        auto profiler = std::make_shared<utils::Profiler>(config);
        profiler->start();
        
        // Create thread pool for parallel processing
        auto threadPool = std::make_shared<threading::ThreadPool>(
            config->getValue<unsigned int>("threadCount", std::thread::hardware_concurrency())
        );
        
        // Initialize network manager
        auto networkManager = std::make_shared<network::NetworkManager>(
            config->getValue<std::string>("host", "localhost"),
            config->getValue<int>("port", 8080),
            threadPool
        );
        
        // Create data processing pipeline
        auto dataflowEngine = std::make_shared<core::DataflowEngine>(threadPool, profiler);
        
        // Add data source nodes
        auto sourceNode = dataflowEngine->createNode<core::DataSourceNode>("sensor_data");
        
        // Add processing nodes with algorithms
        auto processingNode = dataflowEngine->createNode<core::ProcessingNode>("data_processor");
        processingNode->setAlgorithm(
            algorithms::AlgorithmFactory::create(
                config->getValue<std::string>("algorithm", "FFT")
            )
        );
        
        // Add sink nodes for output
        auto sinkNode = dataflowEngine->createNode<core::DataSinkNode>("output");
        
        // Connect the nodes to form a pipeline
        dataflowEngine->connect(sourceNode, processingNode);
        dataflowEngine->connect(processingNode, sinkNode);
        
        // Start the dataflow engine
        dataflowEngine->start();
        
        // Start network server to receive data
        networkManager->startServer();
        
        std::cout << "System running... Press Enter to exit." << std::endl;
        std::cin.get();
        
        // Graceful shutdown
        dataflowEngine->stop();
        networkManager->stopServer();
        threadPool->shutdown();
        profiler->stop();
        
        // Report performance metrics
        profiler->generateReport("performance_report.json");
        
        std::cout << "System shutdown complete." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

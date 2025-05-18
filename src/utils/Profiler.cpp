#include "../../include/utils/Profiler.hpp"
#include "../../include/utils/ConfigManager.hpp"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <limits>
#include <cmath>

namespace dataflow {
namespace utils {

Profiler::Profiler(std::shared_ptr<ConfigManager> config)
    : config_(config) {
    
    if (!config_) {
        throw std::invalid_argument("ConfigManager cannot be null");
    }
}

Profiler::~Profiler() {
    stop();
}

void Profiler::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return; // Already running
    }
    
    // Start background collection if configured
    if (config_->getValue<bool>("enable_background_collection", false)) {
        backgroundThread_ = std::thread(&Profiler::backgroundCollection, this);
    }
}

void Profiler::stop() {
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false)) {
        return; // Already stopped
    }
    
    // Stop background thread
    if (backgroundThread_.joinable()) {
        backgroundThread_.join();
    }
}

void Profiler::createMetric(const std::string& name, const std::string& description, bool collectHistory) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    // Create or update metric
    auto& metric = metrics_[name];
    metric.description = description;
    metric.collectHistory = collectHistory;
    metric.currentValue = 0.0;
    metric.minValue = std::numeric_limits<double>::max();
    metric.maxValue = std::numeric_limits<double>::lowest();
    metric.sumValues = 0.0;
    metric.countValues = 0;
}

bool Profiler::recordValue(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto it = metrics_.find(name);
    if (it == metrics_.end()) {
        // Auto-create metric if it doesn't exist
        createMetric(name, "Auto-created metric", false);
        it = metrics_.find(name);
    }
    
    auto& metric = it->second;
    
    // Update statistics
    metric.currentValue = value;
    metric.minValue = std::min(metric.minValue, value);
    metric.maxValue = std::max(metric.maxValue, value);
    metric.sumValues += value;
    metric.countValues++;
    
    // Add to history if enabled
    if (metric.collectHistory) {
        metric.history.emplace_back(std::chrono::high_resolution_clock::now(), value);
        
        // Trim history if it gets too large
        size_t maxHistory = config_->getValue<size_t>("max_metric_history", 1000);
        if (metric.history.size() > maxHistory) {
            metric.history.erase(metric.history.begin(), 
                               metric.history.begin() + (metric.history.size() - maxHistory));
        }
    }
    
    // Call callbacks
    std::vector<std::function<void(const std::string&, double)>> callbacks = metric.callbacks;
    
    // Release lock before calling callbacks to avoid deadlocks
    metricsMutex_.unlock();
    
    for (const auto& callback : callbacks) {
        try {
            callback(name, value);
        } catch (const std::exception& e) {
            std::cerr << "Error in metric callback: " << e.what() << std::endl;
        }
    }
    
    // Re-acquire lock
    metricsMutex_.lock();
    
    return true;
}

uint64_t Profiler::startTimer(const std::string& name) {
    std::lock_guard<std::mutex> lock(timersMutex_);
    
    uint64_t timerId = nextTimerId_++;
    
    TimerInfo timer;
    timer.name = name;
    timer.startTime = std::chrono::high_resolution_clock::now();
    
    activeTimers_[timerId] = std::move(timer);
    
    return timerId;
}

int64_t Profiler::stopTimer(uint64_t timerId) {
    std::lock_guard<std::mutex> lock(timersMutex_);
    
    auto it = activeTimers_.find(timerId);
    if (it == activeTimers_.end()) {
        return -1; // Timer not found
    }
    
    const auto& timer = it->second;
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - timer.startTime);
    
    // Record the timing
    recordValue(timer.name + "_time", duration.count());
    
    // Remove the timer
    activeTimers_.erase(it);
    
    return duration.count();
}

bool Profiler::generateReport(const std::string& outputPath) {
    std::ofstream file(outputPath);
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n";
    file << "  \"timestamp\": " << std::chrono::system_clock::now().time_since_epoch().count() << ",\n";
    file << "  \"metrics\": {\n";
    
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    size_t metricCount = 0;
    for (const auto& pair : metrics_) {
        const auto& name = pair.first;
        const auto& metric = pair.second;
        
        file << "    \"" << name << "\": {\n";
        file << "      \"description\": \"" << metric.description << "\",\n";
        file << "      \"current\": " << metric.currentValue << ",\n";
        file << "      \"min\": " << metric.minValue << ",\n";
        file << "      \"max\": " << metric.maxValue << ",\n";
        
        // Calculate average
        double avg = 0.0;
        if (metric.countValues > 0) {
            avg = metric.sumValues / metric.countValues;
        }
        
        file << "      \"avg\": " << avg << ",\n";
        file << "      \"count\": " << metric.countValues << ",\n";
        
        // Include recent history if available
        if (metric.collectHistory && !metric.history.empty()) {
            file << "      \"recent_history\": [\n";
            
            // Get last 10 entries or fewer
            size_t historyStart = (metric.history.size() > 10) ? 
                                 (metric.history.size() - 10) : 0;
            
            for (size_t i = historyStart; i < metric.history.size(); ++i) {
                const auto& entry = metric.history[i];
                auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    entry.first.time_since_epoch()).count();
                
                file << "        {\"time\": " << timestamp 
                     << ", \"value\": " << entry.second << "}";
                
                if (i < metric.history.size() - 1) {
                    file << ",";
                }
                
                file << "\n";
            }
            
            file << "      ]\n";
        } else {
            file << "      \"recent_history\": []\n";
        }
        
        file << "    }";
        
        if (++metricCount < metrics_.size()) {
            file << ",";
        }
        
        file << "\n";
    }
    
    file << "  }\n";
    file << "}\n";
    
    return true;
}

double Profiler::getCurrentValue(const std::string& name) const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto it = metrics_.find(name);
    if (it == metrics_.end()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    return it->second.currentValue;
}

std::vector<std::pair<Profiler::TimePoint, double>> Profiler::getHistory(const std::string& name) const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto it = metrics_.find(name);
    if (it == metrics_.end() || !it->second.collectHistory) {
        return {};
    }
    
    return it->second.history;
}

std::tuple<double, double, double> Profiler::getStatistics(const std::string& name) const {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto it = metrics_.find(name);
    if (it == metrics_.end()) {
        return {std::numeric_limits<double>::quiet_NaN(),
               std::numeric_limits<double>::quiet_NaN(),
               std::numeric_limits<double>::quiet_NaN()};
    }
    
    const auto& metric = it->second;
    
    double avg = 0.0;
    if (metric.countValues > 0) {
        avg = metric.sumValues / metric.countValues;
    }
    
    return {metric.minValue, metric.maxValue, avg};
}

bool Profiler::registerCallback(const std::string& name, 
                             std::function<void(const std::string&, double)> callback) {
    std::lock_guard<std::mutex> lock(metricsMutex_);
    
    auto it = metrics_.find(name);
    if (it == metrics_.end()) {
        return false;
    }
    
    it->second.callbacks.push_back(std::move(callback));
    return true;
}

void Profiler::backgroundCollection() {
    const int intervalMs = config_->getValue<int>("background_collection_interval_ms", 1000);
    
    while (running_) {
        try {
            // Collect system metrics
            recordValue("system_cpu_usage", getCurrentCpuUsage());
            recordValue("system_memory_usage", getCurrentMemoryUsage());
            
            // Sleep for the configured interval
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        } catch (const std::exception& e) {
            std::cerr << "Error in background collection: " << e.what() << std::endl;
        }
    }
}

// Helper functions for system metrics (platform-specific, simplified here)
double getCurrentCpuUsage() {
    // In a real implementation, this would use platform-specific APIs
    // For simplicity, we return a random value between 0-100
    return rand() % 100;
}

double getCurrentMemoryUsage() {
    // In a real implementation, this would use platform-specific APIs
    // For simplicity, we return a random value between 0-16 GB
    return (rand() % 16000) / 1000.0;
}

} // namespace utils
} // namespace dataflow

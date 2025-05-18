#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <chrono>
#include <atomic>
#include <functional>

namespace dataflow {

// Forward declaration
namespace utils {
    class ConfigManager;
}

namespace utils {

/**
 * @class Profiler
 * @brief Performance measurement and metrics collection
 * 
 * Provides timing and performance statistics for system components.
 * Supports both instantaneous metrics and time-series data collection.
 */
class Profiler {
public:
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using DurationMicros = std::chrono::microseconds;
    
    /**
     * @brief Constructor
     * @param config Configuration settings
     */
    explicit Profiler(std::shared_ptr<ConfigManager> config);
    
    /**
     * @brief Destructor
     */
    ~Profiler();
    
    /**
     * @brief Start collecting metrics
     */
    void start();
    
    /**
     * @brief Stop collecting metrics
     */
    void stop();
    
    /**
     * @brief Create a new metric
     * @param name Metric name
     * @param description Metric description
     * @param collectHistory Whether to store historical values
     */
    void createMetric(const std::string& name, const std::string& description, bool collectHistory = false);
    
    /**
     * @brief Record a value for a metric
     * @param name Metric name
     * @param value Metric value
     * @return true if recorded successfully, false otherwise
     */
    bool recordValue(const std::string& name, double value);
    
    /**
     * @brief Start timing a section of code
     * @param name Timer name
     * @return A unique ID for this timing session
     */
    uint64_t startTimer(const std::string& name);
    
    /**
     * @brief Stop a previously started timer
     * @param timerId ID returned from startTimer
     * @return Duration in microseconds
     */
    int64_t stopTimer(uint64_t timerId);
    
    /**
     * @brief Generate a performance report
     * @param outputPath Path to write the report to
     * @return true if successful, false otherwise
     */
    bool generateReport(const std::string& outputPath);
    
    /**
     * @brief Get the current value of a metric
     * @param name Metric name
     * @return The current value, or NaN if not found
     */
    double getCurrentValue(const std::string& name) const;
    
    /**
     * @brief Get the historical values of a metric
     * @param name Metric name
     * @return Vector of historical values, empty if not found or history not enabled
     */
    std::vector<std::pair<TimePoint, double>> getHistory(const std::string& name) const;
    
    /**
     * @brief Get metric statistics (min, max, avg)
     * @param name Metric name
     * @return Tuple of min, max, avg values
     */
    std::tuple<double, double, double> getStatistics(const std::string& name) const;
    
    /**
     * @brief Register a callback for metric changes
     * @param name Metric name
     * @param callback Function to call when the metric changes
     * @return true if registered successfully, false otherwise
     */
    bool registerCallback(const std::string& name, 
                        std::function<void(const std::string&, double)> callback);

private:
    struct MetricInfo {
        std::string description;
        double currentValue;
        double minValue;
        double maxValue;
        double sumValues;
        uint64_t countValues;
        bool collectHistory;
        std::vector<std::pair<TimePoint, double>> history;
        std::vector<std::function<void(const std::string&, double)>> callbacks;
    };
    
    struct TimerInfo {
        std::string name;
        TimePoint startTime;
    };
    
    std::shared_ptr<ConfigManager> config_;
    std::unordered_map<std::string, MetricInfo> metrics_;
    std::unordered_map<uint64_t, TimerInfo> activeTimers_;
    std::atomic<uint64_t> nextTimerId_{0};
    
    mutable std::mutex metricsMutex_;
    mutable std::mutex timersMutex_;
    
    std::atomic<bool> running_{false};
    std::thread backgroundThread_;
    
    // Background thread for periodic collection
    void backgroundCollection();
};

// RAII helper class for timing code sections
class ScopedTimer {
public:
    ScopedTimer(Profiler& profiler, const std::string& name)
        : profiler_(profiler), name_(name) {
        timerId_ = profiler_.startTimer(name);
    }
    
    ~ScopedTimer() {
        profiler_.stopTimer(timerId_);
    }

private:
    Profiler& profiler_;
    std::string name_;
    uint64_t timerId_;
};

} // namespace utils
} // namespace dataflow
